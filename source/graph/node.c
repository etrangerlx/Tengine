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
 * 节点（ir_node_t）是计算图中的一个算子，例如卷积、池化、激活。
 * 每个节点记录两类信息：
 *   - 算子本身：类型、版本、参数、形状推导函数（op 字段，实现在
 *     operator/ 目录，由模块注册机制挂进来，见 module/ 目录）；
 *   - 连接关系：输入/输出张量的下标（input_tensors / output_tensors）。
 *     图正是靠这些下标，把“上游节点的输出”接到“下游节点的输入”上。
 *
 * 本文件负责节点的创建、销毁、按名字查找和调试打印。
 * ======================================================================
 */

#include "graph/node.h"

#include "utility/sys_port.h"
#include "graph/tensor.h"
#include "graph/graph.h"
#include "operator/op.h"
#include "module/module.h"
#include "utility/log.h"
#include "utility/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* 第一部分：节点的创建与销毁                                           */
/* ------------------------------------------------------------------ */

/*
 * 初始化节点成员的默认值：
 * - 默认为“中间节点”（既非输入也非输出）；
 * - 默认 same_shape = 1，即假设“输出形状与输入形状相同”，
 *   形状推导时可走快速路径（见 graph.c 的 infer_ir_graph_shape）；
 * - op 的其余成员（参数、infer_shape 回调等）由算子注册信息的 init 回调填充。
 */
static void init_ir_node(ir_node_t* ir_node, int op_type, int op_version, int node_index)
{
    ir_node->index = node_index;
    ir_node->dynamic_shape = 0;
    ir_node->input_num = 0;
    ir_node->output_num = 0;
    ir_node->node_type = TE_NODE_TYPE_INTER;
    ir_node->input_tensors = NULL;
    ir_node->output_tensors = NULL;
    ir_node->name = NULL;
    ir_node->op.type = op_type;
    ir_node->op.version = op_version;
    ir_node->op.same_shape = 1;
    ir_node->op.param_size = 0;
    ir_node->op.param_mem = NULL;
    ir_node->op.infer_shape = NULL;
    ir_node->subgraph_idx = -1;
}

ir_node_t* create_ir_node(struct graph* ir_graph, const char* node_name, int op_type, int op_version)
{
    /* 1. 申请节点结构体 */
    ir_node_t* node = (ir_node_t*)sys_malloc(sizeof(ir_node_t));
    if (NULL == node)
    {
        return NULL;
    }

    init_ir_node(node, op_type, op_version, ir_graph->node_num);

    /* 2. 找到该算子的注册信息，调用其 init 回调初始化算子参数；失败则回滚 */
    ir_method_t* method = find_op_method(op_type, op_version);
    if (NULL == method || NULL == method->init || 0 != method->init(&node->op))
    {
        sys_free(node);
        return NULL;
    }

    /* 3. 把节点指针追加到图的节点列表末尾 */
    ir_node_t** new_node_list = (ir_node_t**)sys_realloc(ir_graph->node_list, sizeof(ir_node_t*) * (ir_graph->node_num + 1));

    if (NULL == new_node_list)
    {
        /* 追加失败：释放 init 回调申请的算子参数内存和节点本身，避免泄漏 */
        if (NULL != method->release)
        {
            method->release(&node->op);
        }

        sys_free(node);
        return NULL;
    }

    node->graph = ir_graph;

    if (NULL != node_name)
    {
        node->name = strdup(node_name);
    }

    new_node_list[ir_graph->node_num] = node;

    ir_graph->node_list = new_node_list;
    ir_graph->node_num++;

    return node;
}

/*
 * 销毁节点：释放名字、输入/输出下标数组，并调用算子的 release 回调
 * 释放它自己申请的参数内存，最后释放节点结构体本身。
 */
void destroy_ir_node(struct graph* ir_graph, ir_node_t* ir_node)
{
    if (NULL != ir_node->name)
    {
        sys_free(ir_node->name);
        ir_node->name = NULL;
    }

    if (0 < ir_node->input_num)
    {
        sys_free(ir_node->input_tensors);
        ir_node->input_tensors = NULL;
    }

    if (0 < ir_node->output_num)
    {
        sys_free(ir_node->output_tensors);
        ir_node->output_tensors = NULL;
    }

    ir_method_t* method = find_op_method(ir_node->op.type, ir_node->op.version);

    if (NULL != method && NULL != method->release)
    {
        method->release(&ir_node->op);
    }

    sys_free(ir_node);
}

/* ------------------------------------------------------------------ */
/* 第二部分：按名字查找                                                 */
/* ------------------------------------------------------------------ */

/*
 * 为没有名字的节点生成默认名字：“node_” + 7 位宽的节点下标。
 * 返回的字符串由调用方负责释放。
 */
char* create_ir_node_name_from_index(int index)
{
    char* name = (char*)sys_malloc(TE_COMMON_ALIGN_SIZE * 2);
    if (NULL == name)
    {
        return NULL;
    }

    sprintf(name, "node_%7d", index);

    return name;
}

/*
 * 按名字查找节点下标，分两步：
 * 1. 快速路径：匿名节点名形如“node_N”，先把下划线后的数字解析成下标，
 *    再校验该下标处的节点名是否真的匹配（防止名字碰巧形如 node_N）；
 * 2. 慢速路径：逐个比较所有节点的名字。
 * 找不到返回 -1。
 */
int get_ir_node_index_from_name(struct graph* ir_graph, const char* node_name)
{
    ir_node_t* ir_node;

    // first: try to get idx from suffix
    const char* p = strrchr(node_name, '_');
    if (p)
    {
        int idx = atoi(++p);

        if (idx >= 0 && idx < ir_graph->node_num)
        {
            ir_node = ir_graph->node_list[idx];

            if (NULL != ir_node->name && 0 == strcmp(ir_node->name, node_name))
            {
                return idx;
            }
        }
    }

    // second: search all nodes to compare name
    for (int i = 0; i < ir_graph->node_num; i++)
    {
        ir_node = ir_graph->node_list[i];

        if (ir_node->name && !strcmp(ir_node->name, node_name))
        {
            return i;
        }
    }

    return -1;
}

/* ------------------------------------------------------------------ */
/* 第三部分：连接节点与张量                                             */
/* ------------------------------------------------------------------ */

/*
 * 把节点的输入/输出张量下标数组扩展到 capacity 个槽位：
 * 重新分配内存，并把新增槽位填成 -1（表示“尚未关联任何张量”）。
 * 下面 set_ir_node_input_tensor / set_ir_node_output_tensor 的公共逻辑。
 * 返回 0 成功，-1 失败。
 */
static int resize_node_tensor_array(uint16_t** tensor_ids, uint8_t* num, int capacity)
{
    uint16_t* new_ids = (uint16_t*)sys_realloc(*tensor_ids, sizeof(uint16_t) * capacity);
    if (NULL == new_ids)
    {
        return -1;
    }

    for (int i = *num; i < capacity; i++)
    {
        new_ids[i] = -1;
    }

    *tensor_ids = new_ids;
    *num = (uint8_t)capacity;

    return 0;
}

int set_ir_node_input_tensor(ir_node_t* node, int input_idx, ir_tensor_t* tensor)
{
    if (input_idx >= node->input_num)
    {
        if (0 != resize_node_tensor_array(&node->input_tensors, &node->input_num, input_idx + 1))
        {
            return -1;
        }
    }

    /* 正向登记：本节点的第 input_idx 个输入是这根张量 */
    node->input_tensors[input_idx] = tensor->index;

    /* 反向登记：告诉张量，本节点是它的一个消费者 */
    return set_ir_tensor_consumer(tensor, node->index);
}

int set_ir_node_output_tensor(ir_node_t* node, int output_idx, ir_tensor_t* tensor)
{
    if (output_idx >= node->output_num)
    {
        if (0 != resize_node_tensor_array(&node->output_tensors, &node->output_num, output_idx + 1))
        {
            return -1;
        }
    }

    /* 正向登记：本节点的第 output_idx 个输出是这根张量 */
    node->output_tensors[output_idx] = tensor->index;

    /* 反向登记：告诉张量，它的生产者是谁 */
    tensor->producer = node->index;

    return 0;
}

/* ------------------------------------------------------------------ */
/* 第四部分：调试打印                                                   */
/* ------------------------------------------------------------------ */

void dump_ir_node(struct graph* ir_graph, ir_node_t* ir_node)
{
    /* 打印节点本身：下标、算子类型、名字 */
    if (NULL != ir_node->name)
    {
        TLOG_INFO("\nnode: %d op: %s name: %s\n", ir_node->index, get_op_name_from_type(ir_node->op.type), ir_node->name);
    }
    else
    {
        TLOG_INFO("\nnode: %d op: %s name: node_%d\n", ir_node->index, get_op_name_from_type(ir_node->op.type), ir_node->index);
    }

    /* 逐个打印输入张量 */
    if (0 < ir_node->input_num)
    {
        TLOG_INFO("\tinput tensors: %d\n", ir_node->input_num);
    }

    for (int i = 0; i < ir_node->input_num; i++)
    {
        ir_tensor_t* ir_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[i]);

        TLOG_INFO("\t    %d: [id: %d] ", i, ir_tensor->index);

        dump_ir_tensor(ir_graph, ir_tensor);
    }

    /* 逐个打印输出张量 */
    if (0 < ir_node->output_num)
        TLOG_INFO("\toutput tensors: %d\n", ir_node->output_num);

    for (int i = 0; i < ir_node->output_num; i++)
    {
        ir_tensor_t* ir_tensor = get_ir_graph_tensor(ir_graph, ir_node->output_tensors[i]);

        TLOG_INFO("\t    %d: [id: %d] ", i, ir_tensor->index);

        dump_ir_tensor(ir_graph, ir_tensor);
    }
}
