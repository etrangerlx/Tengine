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
 */

/*
 * ============================== 新手导读 ==============================
 * 内存分配的统一入口。全工程所有堆内存都必须通过
 * sys_malloc / sys_free / sys_realloc 申请和释放，
 * 便于集中排查内存问题：
 *   - 默认配置：直接转发标准库 malloc / free / realloc；
 *   - 打开 CONFIG_MEM_STAT：转发到带统计的版本（见 mem_stat.c），
 *     可在运行期追踪每一块内存的分配来源。
 * ======================================================================
 */

#include "utility/sys_port.h"

#include <string.h>

#ifdef CONFIG_MEM_STAT

#include "mem_stat.h"

void* sys_malloc(size_t size)
{
    if (skip_stat())
        return malloc(size);
    else
        return stat_malloc(size);
}

void sys_free(void* ptr)
{
    if (skip_stat())
        return free(ptr);
    else
        return stat_free(ptr);
}

void* sys_realloc(void* ptr, size_t size)
{
    if (skip_stat())
        return realloc(ptr, size);
    else
        return stat_realloc(ptr, size);
}

#else

void* sys_malloc(size_t size)
{
    return malloc(size);
}

void sys_free(void* ptr)
{
    return free(ptr);
}

void* sys_realloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

#endif

/* Cortex-M 等嵌入式平台的标准库可能没有 strdup，这里补一个实现 */
#ifdef CONFIG_ARCH_CORTEX_M

char* strdup(const char* src)
{
    if (src == NULL)
        return NULL;

    int n = strlen(src);

    char* new_str = (char*)sys_malloc(n + 1);

    if (new_str == NULL)
        return NULL;

    memcpy(new_str, src, n + 1);

    return new_str;
}

#endif
