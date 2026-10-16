/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2021, OPEN AI LAB
 * Author: haitao@openailab.com
 * Revised: lswang@openailab.com
 */

/*
 * ============================== 新手导读 ==============================
 * 调度器（scheduler_t）负责把图切分出来的各个子图按依赖顺序派给对应
 * 设备的执行器。本文件目前只有一个“同步调度器”（名字叫 "sync"），
 * 也是 create_context 时的默认调度器。四个接口：
 *
 *   prerun ：逐子图调用设备的 pre_run（绑定执行器、分配执行内存）；
 *   run    ：同步执行——维护一张“等待列表”，每轮挑出所有输入已就绪
 *            （input_ready_count == input_wait_count）的子图去执行，
 *            执行完更新下游子图的就绪计数，循环直到列表清空；
 *            最常见的单子图模型一轮循环即可跑完；
 *   wait   ：同步调度器的 run 本身阻塞，无需等待，恒返回 -1；
 *   postrun：逐子图调用设备的 post_run（释放执行期资源）。
 *
 * 子图的依赖关系靠“张量归属”表达：如果子图 A 的某个输出张量正好是
 * 子图 B 的输入，A 执行完就要把 B 的就绪计数加一。
 * ======================================================================
 */

#include "scheduler/scheduler.h"

#include "api/c_api.h"
#include "device/device.h"
#include "graph/graph.h"
#include "graph/subgraph.h"
#include "executer/executer.h"
#include "utility/sys_port.h"
#include "utility/vector.h"
#include "utility/log.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* prerun：为每个子图准备执行环境                                        */
/* ------------------------------------------------------------------ */

static int sched_prerun(ir_scheduler_t* scheduler, ir_graph_t* ir_graph)
{
    int subgraph_num = get_vector_num(ir_graph->subgraph_list);

    for (int i = 0; i < subgraph_num; i++)
    {
        struct subgraph* subgraph = get_ir_graph_subgraph(ir_graph, i);
        ir_device_t* device = subgraph->device;

        if (NULL == device || NULL == device->interface || NULL == device->interface->pre_run)
        {
            TLOG_ERR("Tengine Fatal: Subgraph(%d) has no runnable device.\n", subgraph->index);
            return -1;
        }

        /*
         * 给设备挑选运行选项：上下文里的 default_options 第一个字段是
         * 默认设备名（这里借 char** 读取，避免引入 cpu 头文件）。
         * 子图设备与默认设备一致时用 default_options，否则用 device_options。
         */
        struct context* ctx = ir_graph->attribute->context;
        void* opt = ctx->device_options;

        if (NULL != ctx->default_options)
        {
            char* default_name = *(char**)(ctx->default_options);

            if (0 == strcmp(device->name, default_name))
            {
                opt = ctx->default_options;
            }
        }

        int ret = device->interface->pre_run(device, subgraph, opt);
        if (0 != ret)
        {
            subgraph->status = GRAPH_STAT_ERROR;
            TLOG_ERR("Tengine Fatal: Pre-run subgraph(%d) on %s failed.\n", subgraph->index, device->name);
            return -1;
        }

        subgraph->status = GRAPH_STAT_READY;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* run：按依赖顺序同步执行所有子图                                       */
/* ------------------------------------------------------------------ */

static int sched_run(ir_scheduler_t* scheduler, ir_graph_t* ir_graph, int block)
{
    if (block == 0)
    {
        TLOG_DEBUG("sync scheduler does not support non block run\n");
        return -1;
    }

    /* 工作数据结构：等待列表（存子图指针）+ 每轮就绪子图的下标数组 */
    struct vector* wait_list = create_vector(sizeof(struct subgraph*), NULL);

    if (NULL == wait_list)
    {
        return -1;
    }

    int subgraph_num = get_vector_num(ir_graph->subgraph_list);

    /* 先把所有子图插入等待列表，并顺带校验每个子图都绑定了可用设备 */
    for (int i = 0; i < subgraph_num; i++)
    {
        struct subgraph* subgraph = get_ir_graph_subgraph(ir_graph, i);

        if (NULL == subgraph || NULL == subgraph->device || NULL == subgraph->device->interface ||
            NULL == subgraph->device->interface->run)
        {
            TLOG_ERR("Tengine Fatal: Subgraph(%d) has no runnable device.\n", i);
            release_vector(wait_list);
            return -1;
        }

        push_vector_data(wait_list, &subgraph);
    }

    int* ready_list = (int*)sys_malloc(sizeof(int) * subgraph_num);

    if (NULL == ready_list)
    {
        release_vector(wait_list);
        return -1;
    }

    int ret = 0;

    while (1)
    {
        int ready_num = 0;
        int wait_num = get_vector_num(wait_list);

        if (wait_num == 0)
            break;

        /* 挑出本轮所有“输入已就绪”的子图 */
        for (int i = 0; i < wait_num; i++)
        {
            struct subgraph* subgraph = *(struct subgraph**)get_vector_data(wait_list, i);

            if (subgraph->input_ready_count == subgraph->input_wait_count)
                ready_list[ready_num++] = i;
        }

        if (ready_num == 0)
        {
            TLOG_ERR("no subgraph is ready, but there are still %d subgraphs in wait_list\n", wait_num);
            ret = -1;
            break;
        }

        /* 执行本轮就绪的子图 */
        for (int i = 0; i < ready_num; i++)
        {
            struct subgraph* subgraph = *(struct subgraph**)get_vector_data(wait_list, ready_list[i]);
            ir_device_t* nn_dev = subgraph->device;

            subgraph->status = GRAPH_STAT_RUNNING;

            if (nn_dev->interface->run(nn_dev, subgraph) < 0)
            {
                TLOG_ERR("run subgraph %d error!\n", subgraph->index);
                subgraph->status = GRAPH_STAT_ERROR;
                ret = -1;
                break;
            }

            /*
             * 更新依赖计数：本子图的每个输出张量若正好是其他子图的
             * 输入，就把那个子图的就绪计数加一。
             */
            for (int j = 0; j < subgraph_num; j++)
            {
                struct subgraph* waiting_sub_graph = *(struct subgraph**)get_vector_data(ir_graph->subgraph_list, j);
                for (int k = 0; k < waiting_sub_graph->input_num; k++)
                {
                    uint16_t waiting_input_idx = waiting_sub_graph->input_tensor_list[k];
                    for (int m = 0; m < subgraph->output_num; m++)
                    {
                        int16_t current_output_idx = subgraph->output_tensor_list[m];
                        if (current_output_idx == waiting_input_idx)
                        {
                            waiting_sub_graph->input_ready_count++;
                        }
                    }
                }
            }

            subgraph->status = GRAPH_STAT_READY;
        }

        if (0 != ret)
        {
            break;
        }

        /* 从等待列表移除本轮已执行的子图：按下标从大到小删，避免移位错乱 */
        for (int i = ready_num - 1; i >= 0; i--)
        {
            remove_vector_via_index(wait_list, ready_list[i]);
        }
    }

    /* 无论成败都把就绪计数清零，保证下一轮 run 从干净状态开始 */
    for (int i = 0; i < subgraph_num; i++)
    {
        struct subgraph* subgraph = get_ir_graph_subgraph(ir_graph, i);
        subgraph->input_ready_count = 0;
    }

    sys_free(ready_list);
    release_vector(wait_list);

    return ret;
}

/* ------------------------------------------------------------------ */
/* wait / postrun                                                       */
/* ------------------------------------------------------------------ */

/* 同步调度器的 run 本身就是阻塞的，没有可等待的异步任务 */
static int sched_wait(ir_scheduler_t* scheduler, ir_graph_t* ir_graph)
{
    (void)scheduler;
    (void)ir_graph;

    return -1;
}

static int sched_postrun(ir_scheduler_t* scheduler, ir_graph_t* ir_graph)
{
    int subgraph_num = get_vector_num(ir_graph->subgraph_list);
    int has_error = 0;

    for (int i = 0; i < subgraph_num; i++)
    {
        struct subgraph* subgraph = get_ir_graph_subgraph(ir_graph, i);
        ir_device_t* nn_dev = subgraph->device;

        if (NULL == nn_dev || NULL == nn_dev->interface || NULL == nn_dev->interface->post_run)
        {
            TLOG_ERR("sched: subgraph %d has no runnable device\n", subgraph->index);
            subgraph->status = GRAPH_STAT_ERROR;
            has_error = 1;
            continue;
        }

        subgraph->status = GRAPH_STAT_DONE;

        if (nn_dev->interface->post_run(nn_dev, subgraph) < 0)
        {
            subgraph->status = GRAPH_STAT_ERROR;
            has_error = 1;
            TLOG_ERR("sched %d postrun failed\n", subgraph->index);
        }
    }

    if (has_error)
        return -1;
    else
        return 0;
}

/* ------------------------------------------------------------------ */
/* 调度器注册                                                           */
/* ------------------------------------------------------------------ */

/* 全局唯一一份同步调度器实例 */
static ir_scheduler_t sync_scheduler = {
    .name = "sync",
    .prerun = sched_prerun,
    .run = sched_run,
    .wait = sched_wait,
    .postrun = sched_postrun,
    .release = NULL,
};

ir_scheduler_t* find_default_scheduler(void)
{
    return &sync_scheduler;
}
