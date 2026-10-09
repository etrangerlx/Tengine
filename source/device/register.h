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

#pragma once

#include "utility/log.h"

/*
 * Enabled devices are listed here. The CPU device is always built. Each
 * optional device is guarded by the TENGINE_ENABLE_<DEVICE> macro, which is
 * defined by source/device/CMakeLists.txt when the matching CMake option is
 * turned on.
 */

/* the CPU device is always available */
#define TENGINE_DEVICE_LIST(X)                  \
    X(cpu_device)

#ifdef TENGINE_ENABLE_ACL
#define TENGINE_DEVICE_ACL_LIST(X)              X(acl_device)
#endif

#ifdef TENGINE_ENABLE_CUDA
#define TENGINE_DEVICE_CUDA_LIST(X)             X(cuda_device)
#endif

#ifdef TENGINE_ENABLE_OPENCL
#define TENGINE_DEVICE_OPENCL_LIST(X)           X(ocl_device)
#endif

#ifdef TENGINE_ENABLE_OPENDLA
#define TENGINE_DEVICE_OPENDLA_LIST(X)          X(odla_device)
#endif

#ifdef TENGINE_ENABLE_TENSORRT
#define TENGINE_DEVICE_TENSORRT_LIST(X)         X(trt_device)
#endif

#ifdef TENGINE_ENABLE_TIM_VX
#define TENGINE_DEVICE_TIM_VX_LIST(X)           X(timvx_device)
#endif

#ifdef TENGINE_ENABLE_TORCH
#define TENGINE_DEVICE_TORCH_LIST(X)            X(torch_device)
#endif

#ifdef TENGINE_ENABLE_VULKAN
#define TENGINE_DEVICE_VULKAN_LIST(X)           X(vulkan_device)
#endif

#define TENGINE_DECLARE_DEVICE(name)            \
    extern int register_##name(void);           \
    extern int unregister_##name(void);

TENGINE_DEVICE_LIST(TENGINE_DECLARE_DEVICE)
#ifdef TENGINE_DEVICE_ACL_LIST
TENGINE_DEVICE_ACL_LIST(TENGINE_DECLARE_DEVICE)
#endif
#ifdef TENGINE_DEVICE_CUDA_LIST
TENGINE_DEVICE_CUDA_LIST(TENGINE_DECLARE_DEVICE)
#endif
#ifdef TENGINE_DEVICE_OPENCL_LIST
TENGINE_DEVICE_OPENCL_LIST(TENGINE_DECLARE_DEVICE)
#endif
#ifdef TENGINE_DEVICE_OPENDLA_LIST
TENGINE_DEVICE_OPENDLA_LIST(TENGINE_DECLARE_DEVICE)
#endif
#ifdef TENGINE_DEVICE_TENSORRT_LIST
TENGINE_DEVICE_TENSORRT_LIST(TENGINE_DECLARE_DEVICE)
#endif
#ifdef TENGINE_DEVICE_TIM_VX_LIST
TENGINE_DEVICE_TIM_VX_LIST(TENGINE_DECLARE_DEVICE)
#endif
#ifdef TENGINE_DEVICE_TORCH_LIST
TENGINE_DEVICE_TORCH_LIST(TENGINE_DECLARE_DEVICE)
#endif
#ifdef TENGINE_DEVICE_VULKAN_LIST
TENGINE_DEVICE_VULKAN_LIST(TENGINE_DECLARE_DEVICE)
#endif
#undef TENGINE_DECLARE_DEVICE

#define TENGINE_INVOKE_DEVICE_REG(name)                                             \
    do                                                                              \
    {                                                                               \
        ret = register_##name();                                                    \
        if (0 != ret)                                                               \
        {                                                                           \
            TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_" #name, ret); \
        }                                                                           \
    } while (0);

#define TENGINE_INVOKE_DEVICE_UNREG(name)                                               \
    do                                                                                  \
    {                                                                                   \
        ret = unregister_##name();                                                      \
        if (0 != ret)                                                                   \
        {                                                                               \
            TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_" #name, ret); \
        }                                                                               \
    } while (0);

/*!
 * @brief Register all neural network devices.
 *
 * @return statue value, 0 success, other value failure.
 */
int register_all_devices(void)
{
    int ret = 0;

    TENGINE_DEVICE_LIST(TENGINE_INVOKE_DEVICE_REG)
#ifdef TENGINE_DEVICE_ACL_LIST
    TENGINE_DEVICE_ACL_LIST(TENGINE_INVOKE_DEVICE_REG)
#endif
#ifdef TENGINE_DEVICE_CUDA_LIST
    TENGINE_DEVICE_CUDA_LIST(TENGINE_INVOKE_DEVICE_REG)
#endif
#ifdef TENGINE_DEVICE_OPENCL_LIST
    TENGINE_DEVICE_OPENCL_LIST(TENGINE_INVOKE_DEVICE_REG)
#endif
#ifdef TENGINE_DEVICE_OPENDLA_LIST
    TENGINE_DEVICE_OPENDLA_LIST(TENGINE_INVOKE_DEVICE_REG)
#endif
#ifdef TENGINE_DEVICE_TENSORRT_LIST
    TENGINE_DEVICE_TENSORRT_LIST(TENGINE_INVOKE_DEVICE_REG)
#endif
#ifdef TENGINE_DEVICE_TIM_VX_LIST
    TENGINE_DEVICE_TIM_VX_LIST(TENGINE_INVOKE_DEVICE_REG)
#endif
#ifdef TENGINE_DEVICE_TORCH_LIST
    TENGINE_DEVICE_TORCH_LIST(TENGINE_INVOKE_DEVICE_REG)
#endif
#ifdef TENGINE_DEVICE_VULKAN_LIST
    TENGINE_DEVICE_VULKAN_LIST(TENGINE_INVOKE_DEVICE_REG)
#endif

    return ret;
}

/*!
 * @brief unregister all neural network devices.
 *
 * @return statue value, 0 success, other value failure.
 */
int unregister_all_devices(void)
{
    int ret = 0;

    TENGINE_DEVICE_LIST(TENGINE_INVOKE_DEVICE_UNREG)
#ifdef TENGINE_DEVICE_ACL_LIST
    TENGINE_DEVICE_ACL_LIST(TENGINE_INVOKE_DEVICE_UNREG)
#endif
#ifdef TENGINE_DEVICE_CUDA_LIST
    TENGINE_DEVICE_CUDA_LIST(TENGINE_INVOKE_DEVICE_UNREG)
#endif
#ifdef TENGINE_DEVICE_OPENCL_LIST
    TENGINE_DEVICE_OPENCL_LIST(TENGINE_INVOKE_DEVICE_UNREG)
#endif
#ifdef TENGINE_DEVICE_OPENDLA_LIST
    TENGINE_DEVICE_OPENDLA_LIST(TENGINE_INVOKE_DEVICE_UNREG)
#endif
#ifdef TENGINE_DEVICE_TENSORRT_LIST
    TENGINE_DEVICE_TENSORRT_LIST(TENGINE_INVOKE_DEVICE_UNREG)
#endif
#ifdef TENGINE_DEVICE_TIM_VX_LIST
    TENGINE_DEVICE_TIM_VX_LIST(TENGINE_INVOKE_DEVICE_UNREG)
#endif
#ifdef TENGINE_DEVICE_TORCH_LIST
    TENGINE_DEVICE_TORCH_LIST(TENGINE_INVOKE_DEVICE_UNREG)
#endif
#ifdef TENGINE_DEVICE_VULKAN_LIST
    TENGINE_DEVICE_VULKAN_LIST(TENGINE_INVOKE_DEVICE_UNREG)
#endif

    return ret;
}
