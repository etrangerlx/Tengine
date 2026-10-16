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
 * 子图（ir_subgraph_t）是把整张计算图按执行设备切分后的一段：
 *   - CPU、GPU 等不同后端各自认领图中的一段节点，形成多个子图；
 *   - 每个子图记录自己包含的节点（node_list），
 *     以及对外依赖的输入 / 对外产出的输出张量下标列表。
 *
 * 本文件只做子图的初始化与释放；切分逻辑和执行调度见 executer/ 目录。
 * ======================================================================
 */

#include "graph/subgraph.h"

#include "utility/sys_port.h"
#include "device/device.h"
#include "api/c_api.h"

/*
 * 初始化子图成员的默认值：
 * - 节点列表、输入/输出张量列表先置空，由上层切图逻辑填充；
 * - input_wait_count / input_ready_count 用于运行时同步：
 *   子图执行前需要等所有依赖的输入张量就绪。
 */
void init_ir_subgraph(struct graph* graph, struct subgraph* subgraph, int index)
{
    subgraph->index = index;
    subgraph->input_ready_count = 0;
    subgraph->input_wait_count = 0;
    subgraph->input_num = 0;
    subgraph->output_num = 0;
    subgraph->node_num = 0;
    subgraph->node_list = NULL;
    subgraph->input_tensor_list = NULL;
    subgraph->output_tensor_list = NULL;
    subgraph->graph = graph;
    subgraph->device = NULL;
    subgraph->device_graph = NULL;
    subgraph->status = GRAPH_STAT_CREATED;
}

/*
 * 释放子图：先通知设备释放它自己创建的设备侧图（device_graph），
 * 再释放宿主侧的三个数组，最后释放子图结构体本身。
 * 正常切图流程保证 device_graph 与 device 成对设置。
 */
void release_ir_subgraph(struct graph* graph, struct subgraph* subgraph)
{
    struct device* device = subgraph->device;

    if (NULL != subgraph->device_graph && NULL != device->interface && NULL != device->interface->release_graph)
    {
        device->interface->release_graph(device, (void*)subgraph->device_graph);
    }

    sys_free(subgraph->input_tensor_list);
    sys_free(subgraph->output_tensor_list);
    sys_free(subgraph->node_list);
    sys_free(subgraph);
}
