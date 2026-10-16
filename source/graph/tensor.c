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
 * 张量（ir_tensor_t）是节点之间流动的数据，例如卷积的输入特征图。
 * 每根张量记录三类信息：
 *   - 形状：维度个数（dim_num）、各维大小（dims）、元素总数（elem_num）；
 *   - 类型与量化：数据类型（data_type）、量化 scale / zero point；
 *   - 连接关系：生产者节点（producer）和消费者节点列表（consumer），
 *     与 node.c 里的 input_tensors / output_tensors 互为正反向登记。
 *
 * 本文件负责张量的创建、销毁、形状设置和调试打印。
 * ======================================================================
 */

#include "graph/tensor.h"

#include "defines.h"
#include "graph/node.h"
#include "graph/graph.h"
#include "graph/subgraph.h"
#include "utility/math.h"
#include "utility/utils.h"
#include "utility/sys_port.h"
#include "utility/log.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* 第一部分：张量的创建与销毁                                           */
/* ------------------------------------------------------------------ */

/*
 * 初始化张量成员的默认值：
 * - 消费者数组预分配 TE_MAX_CONSUMER_NUM 个槽位并全部填 -1，
 *   超出上限后再按需扩容（见 set_ir_tensor_consumer）；
 * - 默认类型为 VAR（网络中间变量），可按需改为 CONST / INPUT；
 * - elem_size 由数据类型直接决定。
 */
void init_ir_tensor(ir_tensor_t* ir_tensor, int tensor_index, int data_type)
{
    ir_tensor->index = tensor_index;
    ir_tensor->producer = -1;

    ir_tensor->consumer = (int16_t*)sys_malloc(sizeof(int16_t) * TE_MAX_CONSUMER_NUM);
    for (int i = 0; i < TE_MAX_CONSUMER_NUM; i++)
    {
        ir_tensor->consumer[i] = -1;
    }

    ir_tensor->reshaped = 0;
    ir_tensor->consumer_num = 0;
    ir_tensor->tensor_type = TENSOR_TYPE_VAR;
    ir_tensor->data_type = data_type;
    ir_tensor->dim_num = 0;
    ir_tensor->elem_size = get_tenser_element_size(data_type);
    ir_tensor->subgraph_num = 0;
    ir_tensor->free_host_mem = 0;
    ir_tensor->internal_allocated = 1;
    ir_tensor->layout = TENGINE_LAYOUT_NCHW;
    ir_tensor->quant_param_num = 0;
    ir_tensor->elem_num = 0;

    for (int i = 0; i < TE_MAX_SHAPE_DIM_NUM; i++)
    {
        ir_tensor->dims[i] = 0;
    }

    ir_tensor->data = NULL;
    ir_tensor->name = NULL;
    ir_tensor->scale_list = NULL;
    ir_tensor->zp_list = NULL;
    ir_tensor->dev_mem = NULL;
    ir_tensor->subgraph_list = NULL;
}

ir_tensor_t* create_ir_tensor(ir_graph_t* ir_graph, const char* tensor_name, int data_type)
{
    /* 1. 申请张量结构体 */
    ir_tensor_t* ir_tensor = (ir_tensor_t*)sys_malloc(sizeof(ir_tensor_t));

    if (NULL == ir_tensor)
    {
        return NULL;
    }

    init_ir_tensor(ir_tensor, ir_graph->tensor_num, data_type);

    /* 2. 数据布局继承自所属图 */
    ir_tensor->layout = ir_graph->graph_layout;

    /* 3. 把张量指针追加到图的张量列表末尾 */
    ir_tensor_t** new_tensor_list = (ir_tensor_t**)sys_realloc(ir_graph->tensor_list, sizeof(ir_tensor_t*) * (ir_graph->tensor_num + 1));

    if (NULL == new_tensor_list)
    {
        /* 追加失败：释放 init_ir_tensor 里申请的消费者数组，避免泄漏 */
        sys_free(ir_tensor->consumer);
        sys_free(ir_tensor);
        return NULL;
    }

    /* 4. 记录张量名字（按 8 字节对齐分配，方便后续复用） */
    if (NULL != tensor_name)
    {
        const int str_length = align((int)strlen(tensor_name) + 1, TE_COMMON_ALIGN_SIZE);
        ir_tensor->name = (char*)sys_malloc(str_length);

        if (NULL == ir_tensor->name)
        {
            /* 名字分配失败：先释放消费者数组，再释放结构体，避免泄漏 */
            sys_free(ir_tensor->consumer);
            sys_free(ir_tensor);
            return NULL;
        }

        memset(ir_tensor->name, 0, str_length);
        strcpy(ir_tensor->name, tensor_name);
    }

    new_tensor_list[ir_graph->tensor_num] = ir_tensor;

    ir_graph->tensor_list = new_tensor_list;
    ir_graph->tensor_num++;

    return ir_tensor;
}

/*
 * 释放张量持有的所有资源，顺序：
 * 逐元素量化参数 -> 设备内存 -> 宿主数据 -> 附属数组 -> 结构体本身。
 */
void destroy_ir_tensor(ir_graph_t* ir_graph, ir_tensor_t* ir_tensor)
{
    /* 逐元素量化时 scale / zp 是堆数组，需要释放；单组量化时是 union 标量，无需释放 */
    if (ir_tensor->quant_param_num > 1)
    {
        sys_free(ir_tensor->scale_list);
        sys_free(ir_tensor->zp_list);
    }

    /* 设备内存：设备侧的释放接口尚未实现，目前只释放句柄结构 */
    if (ir_tensor->dev_mem)
    {
        // TODO: add release impl on device
        // do something to release this tensor on device

        sys_free(ir_tensor->dev_mem);
    }

    /* 宿主数据：仅当标记了“由张量自己释放”时才释放 */
    if (0 != ir_tensor->free_host_mem && NULL != ir_tensor->data)
    {
        sys_free(ir_tensor->data);
    }

    if (0 < ir_tensor->subgraph_num)
    {
        sys_free(ir_tensor->subgraph_list);
    }

    if (NULL != ir_tensor->name)
    {
        sys_free(ir_tensor->name);
    }

    if (NULL != ir_tensor->consumer)
    {
        sys_free(ir_tensor->consumer);
    }

    sys_free(ir_tensor);
}

/* ------------------------------------------------------------------ */
/* 第二部分：形状                                                       */
/* ------------------------------------------------------------------ */

/*
 * 设置张量形状：拷贝各维大小，并重新计算元素总数。
 * 维度个数超过 TE_MAX_SHAPE_DIM_NUM 时返回 -1。
 */
int set_ir_tensor_shape(ir_tensor_t* tensor, const int dims[], int dim_number)
{
    if (TE_MAX_SHAPE_DIM_NUM < dim_number)
    {
        return -1;
    }

    const int old_num = tensor->elem_num;
    int new_num = 1;

    for (int i = 0; i < dim_number; i++)
    {
        tensor->dims[i] = dims[i];
        new_num *= dims[i];
    }

    tensor->dim_num = dim_number;
    tensor->elem_num = new_num;

    /* 元素个数变化说明形状变了：把消费者个数存入 reshaped 标记，供重排逻辑使用 */
    if (new_num != old_num)
    {
        tensor->reshaped = tensor->consumer_num;
    }

    return 0;
}

/*
 * 为没有名字的张量生成默认名字：“tensor_” + 7 位宽的张量下标。
 * 返回的字符串由调用方负责释放。
 */
char* create_ir_tensor_name_from_index(int index)
{
    char* name = (char*)sys_malloc(TE_COMMON_ALIGN_SIZE * 2);
    if (NULL == name)
    {
        return NULL;
    }

    sprintf(name, "tensor_%7d", index);

    return name;
}

/*
 * 按名字查找张量下标，分两步（与 node.c 的按名查找同样的套路）：
 * 1. 快速路径：匿名张量名形如“tensor_N”，先解析后缀数字成下标，
 *    再校验该下标处的张量名是否真的匹配；
 * 2. 慢速路径：逐个比较所有张量的名字。
 * 找不到返回 -1。
 */
int get_ir_tensor_index_from_name(ir_graph_t* graph, const char* tensor_name)
{
    const char* last_symbol_ptr = strrchr(tensor_name, '_');

    if (NULL != last_symbol_ptr)
    {
        const int index = atoi(++last_symbol_ptr);

        if (0 <= index && index < graph->tensor_num)
        {
            const ir_tensor_t* const tensor = graph->tensor_list[index];

            if (NULL != tensor->name && 0 == strcmp(tensor->name, tensor_name))
            {
                return index;
            }
        }
    }

    // search all tensors
    for (int i = 0; i < graph->tensor_num; i++)
    {
        const ir_tensor_t* const tensor = graph->tensor_list[i];

        if (tensor->name && 0 == strcmp(tensor->name, tensor_name))
        {
            return i;
        }
    }

    return -1;
}

/* ------------------------------------------------------------------ */
/* 第三部分：量化参数                                                   */
/* ------------------------------------------------------------------ */

/*
 * 设置量化参数（scale / zero point）。
 * 只有一组参数时，直接存进 union 的标量成员，省去堆分配；
 * 多组参数（逐通道量化）时，拷贝进堆数组，并释放旧数组。
 * 传入空指针返回 -1。
 */
int set_ir_tensor_quantization_parameter(ir_tensor_t* tensor, const float* scale, const int* zero_point, int number)
{
    if (NULL == scale || NULL == zero_point)
    {
        return -1;
    }

    if (1 == number)
    {
        tensor->scale = scale[0];
        tensor->zero_point = zero_point[0];
        tensor->quant_param_num = 1;
        return 0;
    }

    float* t_scale = (float*)sys_malloc(sizeof(float) * number);
    int* t_zero = (int*)sys_malloc(sizeof(int) * number);

    if (NULL == t_scale || NULL == t_zero)
    {
        sys_free(t_scale);
        sys_free(t_zero);
        return -1;
    }

    memcpy(t_scale, scale, sizeof(float) * number);
    memcpy(t_zero, zero_point, sizeof(int) * number);

    /* 之前也存过多组参数：先释放旧数组再替换 */
    if (1 < tensor->quant_param_num)
    {
        sys_free(tensor->scale_list);
        sys_free(tensor->zp_list);
    }

    tensor->scale_list = t_scale;
    tensor->zp_list = t_zero;
    tensor->quant_param_num = number;

    return 0;
}

/*
 * 读取量化参数，写入调用方的 scale / zero_point 数组。
 * 调用方缓冲区必须能装下全部参数（number 不小于 quant_param_num），否则返回 -1。
 * 成功时返回实际写入的参数组数。
 */
int get_ir_tensor_quantization_parameter(ir_tensor_t* tensor, float* scale, int* zero_point, int number)
{
    if (number < tensor->quant_param_num)
    {
        return -1;
    }

    if (1 == tensor->quant_param_num)
    {
        scale[0] = tensor->scale;
        zero_point[0] = tensor->zero_point;

        return 1;
    }

    memcpy(scale, tensor->scale_list, sizeof(float) * tensor->quant_param_num);
    memcpy(zero_point, tensor->zp_list, sizeof(int) * tensor->quant_param_num);

    return tensor->quant_param_num;
}

/* ------------------------------------------------------------------ */
/* 第四部分：调试打印                                                   */
/* ------------------------------------------------------------------ */

void dump_ir_tensor(ir_graph_t* g, ir_tensor_t* t)
{
    /* 名字、数据类型、张量类别（const / var / input / dep） */
    if (NULL != t->name)
    {
        TLOG_INFO("%s type: %s/%s", t->name, get_tensor_data_type_string(t->data_type), get_tensor_type_string(t->tensor_type));
    }
    else
    {
        TLOG_INFO("tensor_%d type: %s/%s", t->index, get_tensor_data_type_string(t->data_type),
                  get_tensor_type_string(t->tensor_type));
    }

    /* 形状，例如 shape: [1,3,224,224] */
    if (0 < t->dim_num)
    {
        char shape_buf[64];
        sprintf(shape_buf, " shape: [");

        for (int i = 0; i < t->dim_num - 1; i++)
        {
            sprintf(shape_buf + strlen(shape_buf), "%d,", t->dims[i]);
        }

        sprintf(shape_buf + strlen(shape_buf), "%d]", t->dims[t->dim_num - 1]);

        TLOG_INFO("%s", shape_buf);
    }
    else
    {
        TLOG_INFO(" shape: []");
    }

    /* 生产者与消费者个数 */
    if (0 <= t->producer)
    {
        TLOG_INFO(" from node: %d", t->producer);
    }

    if (t->consumer_num > 0)
        TLOG_INFO(" (consumer: %d)", t->consumer_num);

    TLOG_INFO("\n");
}

/*
 * 给张量登记一个消费者节点（下标），追加到 consumer 数组末尾。
 * 默认容量为 TE_MAX_CONSUMER_NUM，超出后每次扩容一个槽位。
 */
int set_ir_tensor_consumer(ir_tensor_t* ir_tensor, const int index)
{
    if (TE_MAX_CONSUMER_NUM <= ir_tensor->consumer_num)
    {
        int16_t* new_consumer = (int16_t*)sys_realloc(ir_tensor->consumer, sizeof(int16_t) * (ir_tensor->consumer_num + 1));
        if (NULL == new_consumer)
        {
            return -1;
        }

        ir_tensor->consumer = new_consumer;
    }

    ir_tensor->consumer[ir_tensor->consumer_num] = index;
    ir_tensor->consumer_num++;

    return 0;
}
