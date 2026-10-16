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
 * 插件机制：在运行时加载动态库（Linux 下 .so，Windows 下 .dll），调用
 * 库里约定的初始化函数，让第三方能力（自定义算子、设备等）把自己注册
 * 进 Tengine。用法示例：
 *
 *     load_tengine_plugin("plugin_name", "libxxx.so", "load_xx_init");
 *
 * 加载流程：查重名 -> dlopen 打开动态库 -> 执行初始化函数 -> 记录到全局
 * 插件列表；卸载则反过来：执行卸载回调 -> dlclose -> 从列表移除。
 *
 * 平台差异统一用宏切换：
 *   Linux: dlopen / dlsym / dlclose    Windows: LoadLibraryA / GetProcAddress / FreeLibrary
 * ======================================================================
 */

#include "api/c_api.h"

#include "utility/sys_port.h"
#include "utility/vector.h"
#include "utility/log.h"

#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef _MSC_VER
typedef int (*fun_ptr)(void);
typedef HINSTANCE so_handle_t;
#else
typedef void* so_handle_t;
#endif

/* 一条插件记录：插件名、动态库文件名、动态库句柄 */
struct plugin_header
{
    char* name;
    char* fname;
    so_handle_t handle;
};

/* 全局插件列表，懒创建；元素是 plugin_header 结构体本身 */
static vector_t* plugin_list = NULL;

/* ------------------------------------------------------------------ */
/* 内部辅助                                                             */
/* ------------------------------------------------------------------ */

/*
 * 在已打开的动态库里找到名为 func_name 的函数并执行（无参无返回值约定，
 * 返回值 < 0 视为失败）。
 */
static int exec_so_func(so_handle_t handle, const char* func_name)
{
#ifdef _MSC_VER
    fun_ptr func = (fun_ptr)GetProcAddress(handle, func_name);
#else
    void* func = dlsym(handle, func_name);
#endif

    if (func == NULL)
    {
#ifdef _MSC_VER
        TLOG_ERR("find func: %s failed, error code %d\n", func_name, GetLastError());
#else
        TLOG_ERR("find func: %s failed, reason %s\n", func_name, dlerror());
#endif

        return -1;
    }

    int (*call_func)(void) = (int (*)(void))func;

    if (call_func() < 0)
    {
        TLOG_ERR("exec so func: %s failed\n", func_name);
        return -1;
    }
    TLOG_INFO("function:%s executed\n", func_name);

    return 0;
}

/* 按平台关闭动态库 */
static void close_so_handle(so_handle_t handle)
{
#ifdef _MSC_VER
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif
}

/* ------------------------------------------------------------------ */
/* 插件加载                                                             */
/* ------------------------------------------------------------------ */

int load_tengine_plugin(const char* plugin_name, const char* file_name, const char* init_func_name)
{
    struct plugin_header header;

    /* TODO: MT safe */

    /* 插件列表懒创建 */
    if (plugin_list == NULL)
    {
        plugin_list = create_vector(sizeof(struct plugin_header), NULL);

        if (plugin_list == NULL)
        {
            return -1;
        }
    }

    /* 检查插件名是否重复 */
    int list_num = get_vector_num(plugin_list);

    for (int i = 0; i < list_num; i++)
    {
        struct plugin_header* h = (struct plugin_header*)get_vector_data(plugin_list, i);

        if (!strcmp(h->name, plugin_name))
        {
            TLOG_ERR("duplicated plugin name: %s\n", plugin_name);
            return -1;
        }
    }

    /* 打开动态库 */
#ifdef _MSC_VER
    header.handle = LoadLibraryA(file_name);
#else
    header.handle = dlopen(file_name, RTLD_LAZY);
#endif

    if (header.handle == NULL)
    {
#ifdef _MSC_VER
        TLOG_ERR("load plugin failed: error code %d\n", GetLastError());
#else
        TLOG_ERR("load plugin failed: %s\n", dlerror());
#endif
        return -1;
    }

    /* 执行初始化函数；失败则关闭动态库退出 */
    if (init_func_name && exec_so_func(header.handle, init_func_name) < 0)
    {
        close_so_handle(header.handle);
        return -1;
    }

    /* 拷贝插件名和文件名（含结尾 '\0'）留档 */
    size_t plugin_name_length = strlen(plugin_name) + 1;
    size_t file_name_length = strlen(file_name) + 1;

    header.name = (char*)sys_malloc(plugin_name_length);
    header.fname = (char*)sys_malloc(file_name_length);

    if (NULL == header.name || NULL == header.fname)
    {
        TLOG_ERR("allocate memory for plugin %s failed\n", plugin_name);
        sys_free(header.name);
        sys_free(header.fname);
        close_so_handle(header.handle);
        return -1;
    }

    memcpy(header.name, plugin_name, plugin_name_length);
    memcpy(header.fname, file_name, file_name_length);

    push_vector_data(plugin_list, &header);

    return 0;
}

/* ------------------------------------------------------------------ */
/* 插件卸载与查询                                                       */
/* ------------------------------------------------------------------ */

int unload_tengine_plugin(const char* plugin_name, const char* rel_func_name)
{
    if (plugin_list == NULL)
        return -1;

    /* 按名字查找插件记录 */
    int list_num = get_vector_num(plugin_list);
    struct plugin_header* target = NULL;

    for (int i = 0; i < list_num; i++)
    {
        struct plugin_header* h = (struct plugin_header*)get_vector_data(plugin_list, i);

        if (!strcmp(h->name, plugin_name))
        {
            target = h;
            break;
        }
    }

    if (target == NULL)
    {
        return -1;
    }

    /* 执行卸载回调（如果有），然后关闭动态库 */
    if (rel_func_name)
        exec_so_func(target->handle, rel_func_name);

    close_so_handle(target->handle);

    /* 先释放记录里保存的两份字符串，再从列表移除记录本身 */
    sys_free(target->name);
    sys_free(target->fname);

    remove_vector_via_pointer(plugin_list, target);

    /* 列表空了就整个销毁，并把指针复位，下次加载时会重新创建 */
    if (get_vector_num(plugin_list) == 0)
    {
        release_vector(plugin_list);
        plugin_list = NULL;
    }

    return 0;
}

int get_tengine_plugin_number(void)
{
    int plugin_num = 0;

    if (plugin_list)
        plugin_num = get_vector_num(plugin_list);

    return plugin_num;
}

const char* get_tengine_plugin_name(int idx)
{
    int plugin_num = get_tengine_plugin_number();

    if (idx < 0 || idx >= plugin_num)
        return NULL;

    struct plugin_header* h = (struct plugin_header*)get_vector_data(plugin_list, idx);

    return h->name;
}
