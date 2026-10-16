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
 * 本文件目前存放两组“运行环境”结构体的初始化 / 销毁函数：
 *
 *   - ir_context_t（运行上下文）：描述一次推理运行的环境——名字、绑定的
 *     调度器与设备、设备相关配置。它在创建运行图时被创建。
 *   - ir_attribute_t（图属性）：挂在计算图（ir_graph_t->attribute）上的
 *     运行参数——图状态、优先级、调度策略，以及设备 / 调度器的私有数据。
 *
 * 两者的关系：属性里的 context 字段指向运行上下文，图通过它知道自己
 * “在什么环境下跑”。
 *
 * 注意：release_device_mem 是预留接口，尚未实现，恒返回 -1。
 * ======================================================================
 */

#include "executer/executer.h"

#include "utility/sys_port.h"
#include "api/c_api.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* 图属性：初始化与销毁                                                 */
/* ------------------------------------------------------------------ */

/*
 * 初始化图属性：各项运行参数填默认值，私有数据指针置空，
 * 真正的内存分配推迟到设备 / 调度器接入时。
 */
void init_attribute(ir_attribute_t* attribute, ir_context_t* context)
{
    attribute->status = GRAPH_STAT_CREATED;
    attribute->priority = 0;
    attribute->policy = DEFAULT_POLICY;
    attribute->private_context = 0;
    attribute->context = context;
    attribute->device_privacy = NULL;
    attribute->scheduler_privacy = NULL;
}

/*
 * 销毁图属性：释放设备 / 调度器挂上来的私有数据，最后释放结构体自身。
 * 注意：attribute 是 create_ir_graph 时单独 malloc 的，所以这里要 free。
 */
void destroy_attribute(struct graph* graph, ir_attribute_t* attribute)
{
    (void)graph; /* 预留参数：销毁时暂不需要反查图 */

    if (NULL != attribute->device_privacy)
    {
        sys_free(attribute->device_privacy);
    }

    if (NULL != attribute->scheduler_privacy)
    {
        sys_free(attribute->scheduler_privacy);
    }

    sys_free(attribute);
}

/* ------------------------------------------------------------------ */
/* 运行上下文                                                           */
/* ------------------------------------------------------------------ */

/*
 * 初始化运行上下文：拷贝一份名字字符串（含结尾 '\0'），
 * 调度器、设备及其配置项此时都未绑定。
 */
void init_ir_context(ir_context_t* context, const char* name)
{
    if (NULL != name)
    {
        size_t length = strlen(name) + 1;
        context->name = (char*)sys_malloc(length);
        if (NULL == context->name)
        {
            return;
        }
        memcpy(context->name, name, length);
    }
    else
    {
        context->name = NULL;
    }

    context->scheduler = NULL;
    context->device = NULL;
    context->default_options = NULL;
    context->device_options = NULL;
}

/* ------------------------------------------------------------------ */
/* 预留接口                                                             */
/* ------------------------------------------------------------------ */

/* 释放设备内存：接口预留，尚未实现 */
int release_device_mem(struct device* dev, ir_memory_t* dev_mem)
{
    (void)dev;
    (void)dev_mem;

    // TODO:
    return -1;
}
