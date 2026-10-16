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
 * 序列化器（serializer_t）是“模型文件读写”的插件接口：每种模型格式
 * （Tengine 自有格式、ONNX 等）各实现一份函数表，负责把模型文件加载成
 * 计算图，以及在保存模型时把计算图写回文件。
 *
 * 接口定义见 serializer/serializer.h，各格式的实现位于 serializer/tm/、
 * serializer/onnx/ 等子目录；注册与查找见 module/module.c。
 *
 * 本文件只有一个函数：把函数表整体清零。各格式实现拿到结构体后，先调
 * init_serializer 清零，再填上自己支持的回调函数——这样没实现的接口就
 * 是 NULL，调用方可以据此判断。
 * ======================================================================
 */

#include "serializer/serializer.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* 序列化器函数表初始化                                                 */
/* ------------------------------------------------------------------ */

void init_serializer(struct serializer* serializer)
{
    memset(serializer, 0, sizeof(serializer_t));
}
