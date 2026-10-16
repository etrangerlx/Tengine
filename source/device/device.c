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
 * Author: lswang@openailab.com
 */

/*
 * ============================== 新手导读 ==============================
 * 设备（ir_device_t）代表一个可执行网络的硬件后端，例如 CPU、GPU。
 * 它本质上是几组函数指针的容器（见 device/device.h）：
 *   - interface：设备接口（初始化、卸载、建图、执行等）；
 *   - allocator / optimizer / scheduler：内存分配、图优化、调度策略。
 *
 * 设备在 module.c 的设备注册表中登记，按名字（如 "CPU"）查找。
 * ======================================================================
 */

#include "device/device.h"

#include "utility/vector.h"

#include <string.h>

void init_ir_device(ir_device_t* device, const char* name)
{
    if (NULL != name)
    {
        device->name = name;
    }
    else
    {
        /* 兜底叫 "CPU"：正常流程都会传入名字，这里仅作防御 */
        device->name = "CPU";
    }

    device->interface = NULL;
    device->allocator = NULL;
    device->optimizer = NULL;
    device->scheduler = NULL;
    device->privacy = NULL;
}

/* 设备私有配置的大小尚未定义 */
int get_device_option_size(ir_device_t* device)
{
    // TODO: need an impl
    return -1;
}
