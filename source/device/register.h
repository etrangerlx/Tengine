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

extern int register_cpu_device(void);
extern int unregister_cpu_device(void);

#ifdef TENGINE_ENABLE_ACL
extern int register_acl_device(void);
extern int unregister_acl_device(void);
#endif

#ifdef TENGINE_ENABLE_CUDA
extern int register_cuda_device(void);
extern int unregister_cuda_device(void);
#endif

#ifdef TENGINE_ENABLE_OPENCL
extern int register_ocl_device(void);
extern int unregister_ocl_device(void);
#endif

#ifdef TENGINE_ENABLE_OPENDLA
extern int register_odla_device(void);
extern int unregister_odla_device(void);
#endif

#ifdef TENGINE_ENABLE_TENSORRT
extern int register_trt_device(void);
extern int unregister_trt_device(void);
#endif

#ifdef TENGINE_ENABLE_TIM_VX
extern int register_timvx_device(void);
extern int unregister_timvx_device(void);
#endif

#ifdef TENGINE_ENABLE_TORCH
extern int register_torch_device(void);
extern int unregister_torch_device(void);
#endif

#ifdef TENGINE_ENABLE_VULKAN
extern int register_vulkan_device(void);
extern int unregister_vulkan_device(void);
#endif

/*!
 * @brief Register all neural network devices.
 *
 * @return statue value, 0 success, other value failure.
 */
int register_all_devices(void)
{
    int ret = 0;

    ret = register_cpu_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_cpu_device", ret);
    }
#ifdef TENGINE_ENABLE_ACL
    ret = register_acl_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_acl_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_CUDA
    ret = register_cuda_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_cuda_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_OPENCL
    ret = register_ocl_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_ocl_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_OPENDLA
    ret = register_odla_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_odla_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_TENSORRT
    ret = register_trt_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_trt_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_TIM_VX
    ret = register_timvx_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_timvx_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_TORCH
    ret = register_torch_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_torch_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_VULKAN
    ret = register_vulkan_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_vulkan_device", ret);
    }
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

    ret = unregister_cpu_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_cpu_device", ret);
    }
#ifdef TENGINE_ENABLE_ACL
    ret = unregister_acl_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_acl_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_CUDA
    ret = unregister_cuda_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_cuda_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_OPENCL
    ret = unregister_ocl_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_ocl_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_OPENDLA
    ret = unregister_odla_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_odla_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_TENSORRT
    ret = unregister_trt_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_trt_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_TIM_VX
    ret = unregister_timvx_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_timvx_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_TORCH
    ret = unregister_torch_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_torch_device", ret);
    }
#endif
#ifdef TENGINE_ENABLE_VULKAN
    ret = unregister_vulkan_device();
    if (0 != ret)
    {
        TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_vulkan_device", ret);
    }
#endif

    return ret;
}
