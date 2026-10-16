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
 * 本文件负责“计算图”（ir_graph_t）的整个生命周期管理：
 *
 *     创建图 -> 设置输入/输出节点 -> 推导各节点输出形状 -> 销毁图
 *
 * 需要先理解的三个核心概念：
 *   1. 图（graph）：一张有向图，描述整个网络从头到尾的计算流程；
 *   2. 节点（node）：图中的一个算子，例如卷积、池化（见 graph/node.c）；
 *   3. 张量（tensor）：节点之间流动的数据，例如特征图（见 graph/tensor.c）。
 *
 * 一个节点的输入张量，就是它上游节点的输出张量，整张图靠张量串联起来。
 * ======================================================================
 */

#include "graph/graph.h"

#include "defines.h"
#include "utility/sys_port.h"
#include "utility/vector.h"
#include "graph/tensor.h"
#include "graph/node.h"
#include "graph/subgraph.h"
#include "executer/executer.h"
#include "serializer/serializer.h"
#include "utility/utils.h"
#include "utility/log.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* 第一部分：图的创建与销毁                                             */
/* ------------------------------------------------------------------ */

ir_graph_t* create_ir_graph(struct context* context)
{
    /* 申请图结构体，内存不足时返回空 */
    ir_graph_t* ir_graph = (ir_graph_t*)sys_malloc(sizeof(ir_graph_t));
    if (NULL == ir_graph)
    {
        return NULL;
    }

    /* 申请图的属性结构体，并在 init_ir_graph 中完成全部成员的初始化 */
    ir_graph->attribute = (struct attribute*)sys_malloc(sizeof(struct attribute));

    init_ir_graph(ir_graph, context);

    return ir_graph;
}

void init_ir_graph(ir_graph_t* graph, struct context* context)
{
    /* 各类列表先置空，数量清零；真正的内存分配发生在加载模型时 */
    graph->tensor_list = NULL;
    graph->node_list = NULL;
    graph->input_nodes = NULL;
    graph->output_nodes = NULL;

    graph->tensor_num = 0;
    graph->node_num = 0;
    graph->input_num = 0;
    graph->output_num = 0;

    /* 子图列表：把整张图按执行设备切分后，子图指针存放在这个动态数组里 */
    graph->subgraph_list = create_vector(sizeof(struct subgraph*), NULL);

    /* 默认数据布局为 NCHW，默认模型格式为 Tengine 自有格式 */
    graph->graph_layout = TENGINE_LAYOUT_NCHW;
    graph->model_layout = TENGINE_LAYOUT_NCHW;
    graph->model_format = MODEL_FORMAT_TENGINE;

    /* 序列化器（负责读写模型文件）和执行设备，此时尚未绑定 */
    graph->serializer = NULL;
    graph->serializer_privacy = NULL;

    graph->device = NULL;
    graph->device_privacy = NULL;

    graph->status = GRAPH_STAT_CREATED;

    init_attribute(graph->attribute, context);
}

/*
 * 销毁图并释放全部内存。
 * 注意销毁顺序：先子图，再模型数据（序列化器）、张量、节点，最后才是图自身。
 */
void destroy_ir_graph(ir_graph_t* graph)
{
    //!< 1. 销毁所有子图
    if (NULL != graph->subgraph_list)
    {
        const int subgraph_num = get_vector_num(graph->subgraph_list);

        for (int i = 0; i < subgraph_num; i++)
        {
            struct subgraph* subgraph = *(struct subgraph**)get_vector_data(graph->subgraph_list, i);
            release_ir_subgraph(graph, subgraph);
        }

        release_vector(graph->subgraph_list);
    }

    //!< 2. 通知序列化器释放模型数据
    struct serializer* serializer = graph->serializer;
    if (NULL != serializer && serializer->unload_graph)
    {
        serializer->unload_graph(serializer, graph, graph->serializer_privacy, graph->device_privacy);
    }

    //!< 3. 销毁所有张量
    for (int i = 0; i < graph->tensor_num; i++)
    {
        destroy_ir_tensor(graph, graph->tensor_list[i]);
    }

    //!< 4. 销毁所有节点
    for (int i = 0; i < graph->node_num; i++)
    {
        destroy_ir_node(graph, graph->node_list[i]);
    }

    /* 释放图自己持有的数组和结构体 */
    sys_free(graph->tensor_list);
    sys_free(graph->node_list);
    sys_free(graph->input_nodes);
    sys_free(graph->output_nodes);

    if (NULL != graph->attribute)
    {
        destroy_attribute(graph, graph->attribute);
    }

    sys_free(graph);
}

/* ------------------------------------------------------------------ */
/* 第二部分：设置图的输入 / 输出节点                                    */
/* ------------------------------------------------------------------ */

/*
 * 设置输入/输出节点列表的公共实现，被下面两个接口调用。
 *
 * 做两件事：
 *   1. 把调用者传入的节点下标复制一份保存到图中（旧的列表直接释放）；
 *   2. 给这些节点打上类型标记（输入节点 / 输出节点）。
 *
 * 返回 0 表示成功，-1 表示失败。
 */
static int set_graph_io_nodes(ir_graph_t* graph, int16_t** nodes, uint16_t* node_num, int16_t new_nodes[],
                              int new_num, int node_type)
{
    if (0 >= new_num)
    {
        return -1;
    }

    /* 为下标列表分配内存 */
    int16_t* list = (int16_t*)sys_malloc(new_num * sizeof(int16_t));
    if (NULL == list)
    {
        return -1;
    }

    /* 释放旧列表，换上新列表 */
    if (NULL != *nodes)
    {
        sys_free(*nodes);
    }

    *nodes = list;
    *node_num = (uint16_t)new_num;

    /* 逐个记录节点下标，并给对应节点打上输入/输出标记 */
    for (int i = 0; i < new_num; i++)
    {
        ir_node_t* node = get_ir_graph_node(graph, new_nodes[i]);
        node->node_type = node_type;

        list[i] = new_nodes[i];
    }

    return 0;
}

int set_ir_graph_input_node(ir_graph_t* graph, int16_t input_nodes[], int input_number)
{
    return set_graph_io_nodes(graph, &graph->input_nodes, &graph->input_num, input_nodes, input_number,
                              TE_NODE_TYPE_INPUT);
}

int set_ir_graph_output_node(ir_graph_t* graph, int16_t output_nodes[], int output_number)
{
    return set_graph_io_nodes(graph, &graph->output_nodes, &graph->output_num, output_nodes, output_number,
                              TE_NODE_TYPE_OUTPUT);
}

/* ------------------------------------------------------------------ */
/* 第三部分：简单的取值接口                                             */
/* ------------------------------------------------------------------ */

struct tensor* get_ir_graph_tensor(ir_graph_t* graph, int index)
{
    return graph->tensor_list[index];
}

struct node* get_ir_graph_node(ir_graph_t* graph, int index)
{
    return graph->node_list[index];
}

struct subgraph* get_ir_graph_subgraph(ir_graph_t* graph, int index)
{
    return *(struct subgraph**)get_vector_data(graph->subgraph_list, index);
}

/* ------------------------------------------------------------------ */
/* 第四部分：形状推导                                                   */
/* ------------------------------------------------------------------ */

/*
 * 从前往后遍历每个节点，推导出它所有输出张量的形状（shape）。
 * 推导失败时返回 -1。
 */
int infer_ir_graph_shape(ir_graph_t* graph)
{
    const int node_num = graph->node_num;

    for (int i = 0; i < node_num; i++)
    {
        ir_node_t* node = get_ir_graph_node(graph, i);
        ir_op_t* op = &node->op;

        /* 没有输入的节点（例如常量节点）无需推导 */
        if (node->input_num == 0)
            continue;

        /*
         * 动态形状节点：原设计是把 dynamic_shape 标记扩散给下游节点，再跳过自身推导。
         * 该分支目前不可达（全工程没有任何代码把 dynamic_shape 置 1，节点创建时恒为 0），
         * 且内部把输出序号 j、消费者序号 k 直接当全局张量/节点下标使用，属于历史遗留 bug，
         * 故先整体注释掉。将来启用时须改为：
         *   get_ir_graph_tensor(graph, node->output_tensors[j])
         *   get_ir_graph_node(graph, tensor->consumer[k])
         */
#if 0
        if (node->dynamic_shape)
        {
            int output_num = node->output_num;

            for (int j = 0; j < output_num; j++)
            {
                ir_tensor_t* tensor = get_ir_graph_tensor(graph, j);

                for (int k = 0; k < tensor->consumer_num; k++)
                {
                    ir_node_t* child_node = get_ir_graph_node(graph, k);
                    child_node->dynamic_shape = 1;
                }
            }

            continue;
        }
#endif

        if (0 != op->same_shape)
        {
            /* 快速路径：该算子“输出形状与输入形状相同”，直接拷贝即可 */
            ir_tensor_t* input = get_ir_graph_tensor(graph, node->input_tensors[0]);
            ir_tensor_t* output = get_ir_graph_tensor(graph, node->output_tensors[0]);

            output->dim_num = input->dim_num;
            output->elem_num = input->elem_num;

            memcpy(output->dims, input->dims, sizeof(int32_t) * input->dim_num);
        }
        else
        {
            /* 一般路径：调用该算子自己的形状推导函数 */
            if (0 != op->infer_shape(node))
            {
                TLOG_ERR("Tengine FATAL: Infer node(id: %d, op: %s) shape failed.\n", node->index,
                         get_op_name_from_type(node->op.type));
                return -1;
            }
        }

        /* 形状已更新，清掉所有输出张量的“待重排”标记 */
        for (int j = 0; j < node->output_num; j++)
        {
            ir_tensor_t* tensor = get_ir_graph_tensor(graph, node->output_tensors[j]);

            tensor->reshaped = 0;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* 第五部分：调试打印                                                   */
/* ------------------------------------------------------------------ */

void dump_ir_graph(ir_graph_t* graph)
{
    /* 打印整体信息 */
    TLOG_INFO("graph node_num %u tensor_num: %u  subgraph_num: %u\n", graph->node_num, graph->tensor_num,
              get_vector_num(graph->subgraph_list));

    TLOG_INFO("graph layout: %s model layout: %s model_format: %s\n", get_tensor_layout_string(graph->graph_layout),
              get_tensor_layout_string(graph->model_layout), get_model_format_string(graph->model_format));

    /* 逐个打印所有节点 */
    for (int i = 0; i < graph->node_num; i++)
    {
        dump_ir_node(graph, graph->node_list[i]);
    }

    /* 打印输入节点名 */
    TLOG_INFO("\ngraph inputs: %u\n", graph->input_num);

    for (int i = 0; i < graph->input_num; i++)
    {
        ir_node_t* node = get_ir_graph_node(graph, graph->input_nodes[i]);

        if (node->name)
        {
            TLOG_INFO("\t%s\n", node->name);
        }
        else
        {
            TLOG_INFO("\tnode_%d\n", node->index);
        }
    }

    /* 打印输出节点名 */
    TLOG_INFO("graph outputs: %u\n", graph->output_num);

    for (int i = 0; i < graph->output_num; i++)
    {
        ir_node_t* node = get_ir_graph_node(graph, graph->output_nodes[i]);

        if (node->name)
        {
            TLOG_INFO("\t%s\n", node->name);
        }
        else
        {
            TLOG_INFO("\tnode_%d\n", node->index);
        }
    }
}
