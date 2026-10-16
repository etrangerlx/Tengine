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
 *
 * Author: haitao@openailab.com
 * Revised: lswang@openailab.com
 */

/*
 * ============================== 新手导读 ==============================
 * 本文件是 Tengine 对外的 C 接口层（c_api），内部所有 ir_* 结构体在这里
 * 都被封装成不透明句柄（见 api/c_api.h）：
 *
 *     context_t = struct context*   运行环境（绑定的设备、调度器）
 *     graph_t   = struct graph*     计算图
 *     node_t    = struct node*      算子节点
 *     tensor_t  = struct tensor*    张量
 *
 * 一个典型推理程序按下面的顺序调用（函数按此流程分节）：
 *
 *     init_tengine()                          注册算子/序列化器/设备
 *   -> create_context()                       创建运行环境，绑定设备
 *   -> create_graph()                         加载模型文件，得到计算图
 *   -> set_graph_input_node() / set_graph_output_node()
 *   -> prerun_graph_multithread()             形状推导、图优化、调度器准备
 *   -> run_graph()                            推理（block=1 同步，0 异步）
 *   -> get_graph_output_tensor() + get_tensor_buffer()   取结果
 *   -> postrun_graph() / destroy_graph() / release_tengine()
 *
 * 注意：一批 set_/get_*_attr、set_custom_kernel、errno 系列接口是预留桩，
 * 恒返回 -1，尚未实现。
 * ======================================================================
 */

#include "api/c_api.h"

#include "graph/tensor.h"
#include "graph/node.h"
#include "graph/graph.h"
#include "graph/subgraph.h"
#include "device/register.h"
#include "operator/op.h"
#include "operator/op_name.h"
#include "operator/prototype.h"
#include "serializer/serializer.h"
#include "serializer/register.h"
#include "device/device.h"
#include "executer/executer.h"
#include "scheduler/scheduler.h"
#include "system/cpu.h"
#include "module/module.h"
#include "utility/sys_port.h"
#include "utility/vector.h"
#include "utility/utils.h"
#include "utility/log.h"

#include "cpu_define.h"

#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>

#ifdef TENGINE_ONLINE_REPORT
#include "tenginereportmgr.h"
#endif

#define STR_VERSION2(a) #a
#define STR_VERSION(a)  STR_VERSION2(a)

#ifdef TENGINE_LITE_VERSION
static const char* tengine_lite_version = STR_VERSION(TENGINE_LITE_VERSION);
#else
static const char* tengine_lite_version = "1.5";
#endif

#ifdef TENGINE_VERSION_POSTFIX
static const char* ver_postfix = STR_VERSION(TENGINE_VERSION_POSTFIX);
#else
static const char* ver_postfix = "dev";
#endif

static const char* hcl_version = NULL;

/* 引擎是否已初始化：0 = 未初始化，> 0 = 已初始化 */
static int init_flag = 0;

/* ------------------------------------------------------------------ */
/* 第一部分：运行上下文（context）                                       */
/* ------------------------------------------------------------------ */

/*
 * 创建运行上下文。
 *   - empty_context = 0：自动绑定默认设备（CPU），大多数场景用这个；
 *   - empty_context = 1：不绑定任何设备，稍后用 set_context_device 指定。
 */
context_t create_context(const char* context_name, int empty_context)
{
    struct context* context = (struct context*)sys_malloc(sizeof(struct context));
    if (NULL == context)
    {
        return NULL;
    }

    init_ir_context(context, context_name);

    context->scheduler = find_default_scheduler();

    if (0 == empty_context)
    {
        context->device = find_default_device();
    }

    return context;
}

/* 销毁上下文：释放名字、设备配置项，最后释放结构体自身 */
void destroy_context(context_t context)
{
    struct context* ctx = (struct context*)context;

    if (NULL == context)
    {
        return;
    }

    if (NULL != ctx->name)
    {
        sys_free(ctx->name);
    }

    if (NULL != ctx->default_options)
    {
        sys_free(ctx->default_options);
    }

    if (NULL != ctx->device_options)
    {
        sys_free(ctx->device_options);
    }

    sys_free(ctx);
}

/* 取计算图挂的运行上下文（图 -> 上下文的桥） */
struct context* get_ir_graph_context(struct graph* ir_graph)
{
    return ir_graph->attribute->context;
}

/* 当前设计里一个上下文最多绑一个设备，所以设备数只会是 0 或 1 */
int get_context_device_number(context_t context)
{
    struct context* ctx = (struct context*)context;
    if (NULL != ctx && NULL != ctx->device)
    {
        return 1;
    }

    return 0;
}

struct device* get_context_device(context_t context, int index)
{
    struct context* ctx = (struct context*)context;
    if (NULL == ctx)
    {
        TLOG_ERR("Tengine: Context pointer is null.\n");
        return NULL;
    }

    if (NULL != ctx->device && 0 == index)
    {
        return ctx->device;
    }

    return NULL;
}

int add_context_device(context_t context, const char* dev_name)
{
    struct context* ctx = (struct context*)context;
    if (NULL == ctx)
    {
        TLOG_ERR("Tengine: Context pointer is null.\n");
        return -1;
    }

    if (NULL != ctx->device)
    {
        TLOG_ERR("Tengine: Context(%s) is not multi-device collaborative.\n", ctx->name);
        return -1;
    }

    struct device* selected_device = find_device_via_name(dev_name);
    if (NULL == selected_device)
    {
        TLOG_ERR("Tengine: Device(%s) is not found(may not registered).\n", dev_name);
        return -1;
    }

    ctx->device = selected_device;

    return 0;
}

/* 绑定设备，并可附带一份设备私有配置（按字节拷贝保存） */
int set_context_device(context_t context, const char* dev_name, const void* dev_option, size_t dev_opt_size)
{
    struct context* ctx = (struct context*)context;
    if (NULL == ctx)
    {
        TLOG_ERR("Tengine: Context pointer is null.\n");
        return -1;
    }

    if (NULL != ctx->device)
    {
        TLOG_ERR("Tengine: A device(%s) has been set for this context(%s).\n", ctx->device->name, ctx->name);
        return -1;
    }

    struct device* selected_device = find_device_via_name(dev_name);
    if (NULL == selected_device)
    {
        TLOG_ERR("Tengine: Device(%s) is not found(may not registered).\n", dev_name);
        return -1;
    }

    ctx->device = selected_device;

    if (NULL != dev_option)
    {
        ctx->device_options = sys_malloc(dev_opt_size);
        if (NULL == ctx->device_options)
        {
            TLOG_ERR("Tengine: Allocate device option for context failed.\n");
            return -1;
        }
        memcpy(ctx->device_options, dev_option, dev_opt_size);
    }

    return 0;
}

/* 解绑指定名字的设备（当前一个上下文只有一个设备，对上名字才解绑） */
int remove_context_device(context_t context, const char* dev_name)
{
    struct context* ctx = (struct context*)context;
    if (NULL == ctx)
    {
        TLOG_ERR("Tengine: Context pointer is null.\n");
        return -1;
    }

    if (NULL == dev_name)
    {
        TLOG_ERR("Tengine: Device name is null.\n");
        return 0;
    }

    if (NULL == ctx->device)
    {
        TLOG_ERR("Tengine: Context(%s) does not has any device.\n", ctx->name);
        return -1;
    }

    if (0 == strcmp(ctx->device->name, dev_name))
    {
        ctx->device = NULL;
        return 0;
    }

    TLOG_ERR("Tengine: Context(%s) does not has a device named %s.\n", ctx->name, dev_name);
    return -1;
}

/* 预留接口，尚未实现 */
int set_context_attr(context_t context, const char* attr_name, const void* val, int val_size)
{
    (void)context;
    (void)attr_name;
    (void)val;
    (void)val_size;

    return -1;
}

/* 预留接口，尚未实现 */
int get_context_attr(context_t context, const char* attr_name, void* val, int val_size)
{
    (void)context;
    (void)attr_name;
    (void)val;
    (void)val_size;

    return -1;
}

/* ------------------------------------------------------------------ */
/* 第二部分：引擎初始化与版本信息                                        */
/* ------------------------------------------------------------------ */

/* 版本字符串形如 "1.5-dev"，静态缓冲区，非线程安全 */
const char* get_tengine_version(void)
{
    static char buf[128];

    snprintf(buf, 128, "%s-%s", tengine_lite_version, ver_postfix);

    buf[127] = 0x0; /* 防御：确保结尾一定有 '\0' */

    return buf;
}

/* 版本检查桩：恒返回 1，表示接受任意版本要求 */
int request_tengine_version(const char* version)
{
    (void)version;

    return 1;
}

/*
 * 引擎初始化：把三类组件注册进各自的注册表（见 module/module.c）：
 *   1. 算子原型（每种算子的 infer_shape 等方法）
 *   2. 序列化器（各种模型格式的加载器）
 *   3. 执行设备（CPU 等）
 * 重复调用直接返回 0（靠 init_flag 保证只初始化一次）。
 */
int init_tengine(void)
{
    if (0 != init_flag)
    {
        return 0;
    }

    int ret = register_all_op_prototype();
    if (0 != ret)
    {
        TLOG_ERR("Tengine: Register operator failed: %d\n", ret);
        return ret;
    }

    ret = register_all_serializer();
    if (0 != ret)
    {
        TLOG_ERR("Tengine: Register serializer failed: %d\n", ret);
        return ret;
    }

    ret = register_all_devices();
    if (0 != ret)
    {
        TLOG_ERR("Tengine: Register neural network devices failed: %d\n", ret);
        return ret;
    }

#ifdef TENGINE_ONLINE_REPORT
    hcl_version = get_tengine_version();
    init_tengine_report_mgr();
    do_tengine_report(ACTION_INIT);
#endif

    init_flag++;

    return ret;
}

/* 引擎反初始化：按注册的相反顺序注销三类组件 */
void release_tengine(void)
{
    if (0 == init_flag)
    {
        return;
    }

    int ret = unregister_all_op_prototype();
    if (0 != ret)
    {
        TLOG_ERR("Tengine: Unregister operator failed: %d\n", ret);
    }

    ret = release_op_registry();
    if (0 != ret)
    {
        TLOG_ERR("Tengine: Release operator prototype registry failed: %d\n", ret);
    }

    ret = unregister_all_serializer();
    if (0 != ret)
    {
        TLOG_ERR("Tengine: Unregister serializer failed: %d\n", ret);
    }

    ret = release_serializer_registry();
    if (0 != ret)
    {
        TLOG_ERR("Tengine: Release serializer registry failed: %d\n", ret);
    }

    ret = unregister_all_devices();
    if (0 != ret)
    {
        TLOG_ERR("Tengine: Unregister neural network devices failed: %d\n", ret);
    }

#ifdef TENGINE_ONLINE_REPORT
    release_tengine_report_mgr();
#endif

    init_flag = 0;
}

/* ------------------------------------------------------------------ */
/* 第三部分：计算图的生命周期与运行                                      */
/* ------------------------------------------------------------------ */

/* 加载失败时的统一收尾：销毁半成品图，返回 NULL */
graph_t create_graph_error(ir_graph_t* graph)
{
    destroy_graph(graph);
    return NULL;
}

/*
 * 加载模型，创建计算图。两种加载方式由 model_format 参数区分：
 *   - 传格式名（"tengine"、"onnx" 等）：从文件加载，file_name 是路径，
 *     其余可变参数交给序列化器自行解释；
 *   - 传 ":m" 结尾的形式：从内存加载，file_name 实际是内存地址，
 *     可变参数依次为 (int size, ...)；
 *   - 传 NULL：创建空图，由调用者手动搭建（建节点、连张量）。
 * context 传 NULL 时会自动创建一个私有上下文（随图一起销毁）。
 */
graph_t create_graph(context_t context, const char* model_format, const char* file_name, ...)
{
    int is_new_context = 0;

    if (context == NULL)
    {
        context = create_context(NULL, 1);
        is_new_context = 1;
    }

    ir_graph_t* ir_graph = create_ir_graph((struct context*)context);

    if (ir_graph == NULL)
    {
        if (is_new_context)
        {
            destroy_context(context);
        }

        return NULL;
    }

    /* 标记上下文归属：私有上下文在 destroy_graph 时一并销毁 */
    ir_graph->attribute->private_context = is_new_context;

    if (NULL != model_format)
    {
        int ret = 0;
        struct serializer* loader = find_serializer_via_name(model_format);
        va_list ap;
        if (loader == NULL)
        {
            /* 内存加载：格式串里应有 ":m" 后缀 */
            const char* p = strchr(model_format, ':');
            if ((NULL == p) || (p[1] != 'm'))
            {
                TLOG_ERR("Tengine: Invalid postfix(%s) for model format: should 'm' only.\n", model_format);
                return create_graph_error(ir_graph);
            }

            loader = find_serializer_via_name("tengine");

            if (NULL == loader)
            {
                TLOG_ERR("Tengine: Serializer(tengine) is not registered.\n");
                return create_graph_error(ir_graph);
            }

            if (NULL == loader->load_mem)
            {
                TLOG_ERR("Tengine: Serializer(%s) does not support loading from memory.\n", loader->get_name(loader));
                return create_graph_error(ir_graph);
            }

            va_start(ap, file_name);

            int size = va_arg(ap, int);

            ret = loader->load_mem(loader, ir_graph, (void*)file_name, size, ap);

            va_end(ap);
        }
        else
        {
            /* 文件加载 */
            va_start(ap, file_name);

            ret = loader->load_model(loader, ir_graph, file_name, ap);

            va_end(ap);
        }

        if (0 != ret)
        {
            return create_graph_error(ir_graph);
        }

        ir_graph->device = find_default_device();
    }

    return ir_graph;
}

/* 单线程 prerun 的便捷封装：默认 1 线程、大核、不做精度/亲和性约束 */
int prerun_graph(graph_t graph)
{
    struct options option;
    option.num_thread = 1;
    option.precision = -1;
    option.affinity = -1;
    option.cluster = TENGINE_CLUSTER_BIG;

    return prerun_graph_multithread(graph, option);
}

/*
 * 推理前的准备工作，按顺序做五件事：
 *   1. 形状推导：从输入开始推算所有中间张量的形状；
 *   2. 设备图优化：让设备把图按子图切分、做算子融合等优化；
 *   3. 组装 CPU 运行选项（线程数、大/小核、精度、亲和性）放进上下文；
 *   4. 调度器 prerun：为每个子图绑定执行器、分配内存等；
 *   5. 设置 CPU 亲和性；环境变量 TENGINE_DUMP_GRAPH=1 时 dump 图结构。
 */
int prerun_graph_multithread(graph_t graph, struct options option)
{
    struct graph* ir_graph = (struct graph*)graph;

    /* 1. 形状推导 */
    int ret = infer_ir_graph_shape(ir_graph);
    if (0 != ret)
    {
        ir_graph->status = GRAPH_STAT_ERROR;
        fprintf(stderr, "Tengine: Infer shape of graph failed(%d).\n", ret);
        return -1;
    }

    /* 2. 设备侧的图切分与优化（若设备实现了相应回调） */
    struct context* ctx = get_ir_graph_context(ir_graph);
    struct device* dev = ctx->device;

    if (NULL == dev)
    {
        dev = find_default_device();
    }

    if (NULL != dev && NULL != dev->optimizer)
    {
        if (NULL != dev->optimizer->split_graph)
        {
            if (0 != dev->optimizer->split_graph(ir_graph))
            {
                ir_graph->status = GRAPH_STAT_ERROR;
                fprintf(stderr, "Tengine: Split graph via device(%s) failed.\n", dev->name);
                return -1;
            }
        }

        if (NULL != dev->optimizer->optimize_graph)
        {
            ret = dev->optimizer->optimize_graph(ir_graph, -1);
            if (0 != ret)
            {
                ir_graph->status = GRAPH_STAT_ERROR;
                fprintf(stderr, "Tengine: Optimize graph via device(%s) failed.\n", dev->name);
                return -1;
            }
        }
    }

    /* 3. 组装 CPU 运行选项：线程数不超过所选簇的核数 */
    check_cpu();

    size_t mask = get_cpu_cluster_mask(TENGINE_CLUSTER_BIG);
    if (0 <= option.cluster)
    {
        mask = get_cpu_cluster_mask(option.cluster);
    }

    int count = get_cpu_mask_count(mask);
    if (0 < option.num_thread && count > option.num_thread)
    {
        count = option.num_thread;
    }

    /* 精度：传 -1 或非法值时回退到 FP32 */
    int precision = TENGINE_MODE_FP32;
    if (0 <= option.precision && (TENGINE_MODE_FP32 == option.precision || TENGINE_MODE_FP16 == option.precision || TENGINE_MODE_HYBRID_INT8 == option.precision || TENGINE_MODE_UINT8 == option.precision || TENGINE_MODE_INT8 == option.precision))
    {
        precision = option.precision;
    }

    ctx->default_options = sys_malloc(sizeof(struct cpu_option));
    if (NULL == ctx->default_options)
    {
        ir_graph->status = GRAPH_STAT_ERROR;
        fprintf(stderr, "Tengine: Allocate cpu option failed.\n");
        return -1;
    }

    struct cpu_option* opt = (struct cpu_option*)ctx->default_options;
    opt->dev_name = CPU_DEVICE_NAME;
    opt->num_thread = count;
    opt->cluster = TENGINE_CLUSTER_BIG;
    opt->precision = precision;
    opt->affinity = option.affinity;

    /* 4. 调度器 prerun */
    struct scheduler* scheduler = ctx->scheduler;
    ret = scheduler->prerun(scheduler, ir_graph);
    if (0 != ret)
    {
        ir_graph->status = GRAPH_STAT_ERROR;
        fprintf(stderr, "Tengine: Scheduler(%s) prerun failed.\n", scheduler->name);
        return ret;
    }

    ir_graph->status = GRAPH_STAT_READY;

    /* 5. 设置 CPU 亲和性：优先用用户指定且与所选簇有交集的掩码 */
    if (0 != opt->affinity && 0 != (opt->affinity & mask))
    {
        set_cpu_affine(opt->affinity);
    }
    else
    {
        set_cpu_affine(mask);
    }

    /* 调试用：设置环境变量 TENGINE_DUMP_GRAPH=1 可输出图结构 */
    const char* env = getenv(TENGINE_DUMP_GRAPH);
    if (env && env[0] == '1')
    {
        set_log_level(LOG_INFO);
        dump_ir_graph(ir_graph);
    }

    return 0;
}

/*
 * 执行一次推理。
 *   - block = 1：同步执行，返回时结果已就绪（状态回到 READY）；
 *   - block = 0：异步执行，之后用 wait_graph 等待完成。
 */
int run_graph(graph_t graph, int block)
{
    struct graph* ir_graph = (struct graph*)graph;
    struct context* context = get_ir_graph_context(ir_graph);
    struct scheduler* scheduler = context->scheduler;

    ir_graph->status = GRAPH_STAT_RUNNING;

    if (scheduler->run(scheduler, ir_graph, block) < 0)
    {
        ir_graph->status = GRAPH_STAT_ERROR;
        return -1;
    }
    else
    {
        if (block)
            ir_graph->status = GRAPH_STAT_READY;
    }

    return 0;
}

/*
 * 等待异步推理完成。
 *   - 图已完成（READY）：直接返回 0；
 *   - 图未在运行：返回 -1；
 *   - 图运行中：try_wait 非 0 时阻塞等调度器完成，为 0 时返回 -1。
 */
int wait_graph(graph_t graph, int try_wait)
{
    struct graph* ir_graph = (struct graph*)graph;
    struct context* context = get_ir_graph_context(ir_graph);
    struct scheduler* scheduler = context->scheduler;

    if (GRAPH_STAT_RUNNING != ir_graph->status && GRAPH_STAT_READY != ir_graph->status)
    {
        return -1;
    }

    if (ir_graph->status == GRAPH_STAT_READY)
    {
        return 0;
    }

    if (!try_wait)
    {
        return -1;
    }

    return scheduler->wait(scheduler, ir_graph);
}

/*
 * 推理结束后的清理：通知调度器释放执行期资源（执行器、内存池等），
 * 并释放设备私有数据（当前实现为一个 vector，可能尚未使用）。
 */
int postrun_graph(graph_t graph)
{
    struct graph* ir_graph = (struct graph*)graph;
    struct context* context = get_ir_graph_context(ir_graph);
    struct scheduler* scheduler = context->scheduler;

    if (scheduler->postrun(scheduler, ir_graph) < 0)
    {
        ir_graph->status = GRAPH_STAT_ERROR;
        return -1;
    }

    if (NULL != ir_graph->attribute->device_privacy)
    {
        release_vector((vector_t*)ir_graph->attribute->device_privacy);
        ir_graph->attribute->device_privacy = NULL;
    }

    ir_graph->status = GRAPH_STAT_DONE;

    return 0;
}

/* 设置图的数据布局（NCHW / NHWC），影响输入输出张量的内存排布 */
int set_graph_layout(graph_t graph, int layout_type)
{
    struct graph* ir_graph = (struct graph*)graph;

    if ((layout_type != TENGINE_LAYOUT_NCHW) && (layout_type != TENGINE_LAYOUT_NHWC))
    {
        return -1;
    }

    ir_graph->graph_layout = layout_type;

    return 0;
}

/* 预留接口，尚未实现 */
int set_graph_attr(graph_t graph, const char* attr_name, const void* buf, int size)
{
    (void)graph;
    (void)attr_name;
    (void)buf;
    (void)size;

    return -1;
}

/* 预留接口，尚未实现 */
int get_graph_attr(graph_t graph, const char* attr_name, void* buf, int size)
{
    (void)graph;
    (void)attr_name;
    (void)buf;
    (void)size;

    return -1;
}

/* 预留接口，尚未实现 */
int set_graph_thread(graph_t graph, int cluster, int threads)
{
    (void)graph;
    (void)cluster;
    (void)threads;

    return -1;
}

/* 预留接口，尚未实现 */
int set_graph_thread_mask(graph_t graph, size_t cpu_mask)
{
    (void)graph;
    (void)cpu_mask;

    return -1;
}

/* 销毁图；若图用的是 create_graph 自动创建的私有上下文，一并销毁 */
int destroy_graph(graph_t graph)
{
    struct graph* ir_graph = (struct graph*)graph;

    if (ir_graph->attribute->private_context)
        destroy_context(ir_graph->attribute->context);

    destroy_ir_graph(ir_graph);

    return 0;
}

/* 打印图结构（节点、张量、子图信息），调试用 */
void dump_graph(graph_t graph)
{
    dump_ir_graph((ir_graph_t*)graph);
}

/* 把图绑定到指定名字的设备（影响后续子图切分） */
int set_graph_device(graph_t graph, const char* dev_name)
{
    struct graph* ir_graph = (struct graph*)graph;
    struct device* nn_dev = find_device_via_name(dev_name);

    if (NULL == nn_dev)
    {
        return -1;
    }

    ir_graph->device = nn_dev;
    return 0;
}

/* ------------------------------------------------------------------ */
/* 第四部分：节点                                                       */
/* ------------------------------------------------------------------ */

/*
 * 下面两个函数把“按名字指定输入/输出节点”翻译成“按下标列表”，
 * 再调用 graph 层的 set_ir_graph_input/output_node。
 */
int set_graph_input_node(graph_t graph, const char* input_nodes[], int input_number)
{
    struct graph* ir_graph = (struct graph*)graph;
    int16_t* input_node_indexes;

    input_node_indexes = (int16_t*)sys_malloc(sizeof(int16_t) * input_number);

    if (input_node_indexes == NULL)
    {
        return -1;
    }

    for (int i = 0; i < input_number; i++)
    {
        int node_idx = get_ir_node_index_from_name(ir_graph, input_nodes[i]);

        if (node_idx < 0)
        {
            sys_free(input_node_indexes);
            return -1;
        }

        input_node_indexes[i] = node_idx;
    }

    int ret = set_ir_graph_input_node(ir_graph, input_node_indexes, input_number);

    sys_free(input_node_indexes);

    return ret;
}

int set_graph_output_node(graph_t graph, const char* output_nodes[], int output_number)
{
    struct graph* ir_graph = (struct graph*)graph;

    int16_t* output_node_indexes;

    output_node_indexes = (int16_t*)sys_malloc(sizeof(int16_t) * output_number);

    if (output_node_indexes == NULL)
    {
        return -1;
    }

    for (int i = 0; i < output_number; i++)
    {
        int index = get_ir_node_index_from_name(ir_graph, output_nodes[i]);

        if (index < 0)
        {
            sys_free(output_node_indexes);
            return -1;
        }

        output_node_indexes[i] = index;
    }

    int ret = set_ir_graph_output_node(ir_graph, output_node_indexes, output_number);

    sys_free(output_node_indexes);

    return ret;
}

/* ---- 图的输入 / 输出节点访问 ---- */

int get_graph_input_node_number(graph_t graph)
{
    struct graph* ir_graph = (struct graph*)graph;

    return ir_graph->input_num;
}

node_t get_graph_input_node(graph_t graph, int idx)
{
    struct graph* ir_graph = (struct graph*)graph;

    if (idx < 0 || idx >= ir_graph->input_num)
    {
        return NULL;
    }

    return get_ir_graph_node(ir_graph, ir_graph->input_nodes[idx]);
}

int get_graph_output_node_number(graph_t graph)
{
    struct graph* ir_graph = (struct graph*)graph;

    return ir_graph->output_num;
}

node_t get_graph_output_node(graph_t graph, int idx)
{
    struct graph* ir_graph = (struct graph*)graph;

    if (idx < 0 || idx >= ir_graph->output_num)
    {
        return NULL;
    }

    return get_ir_graph_node(ir_graph, ir_graph->output_nodes[idx]);
}

/*
 * 输入/输出节点的第 tensor_idx 个输出张量。
 * （输入节点也是普通节点，它的“输出张量”就是整张图的输入数据。）
 */
tensor_t get_graph_input_tensor(graph_t graph, int input_idx, int tensor_idx)
{
    struct graph* ir_graph = (struct graph*)graph;

    if (input_idx < 0 || input_idx >= ir_graph->input_num)
    {
        return NULL;
    }

    int input_node_idx = ir_graph->input_nodes[input_idx];

    struct node* ir_node = ir_graph->node_list[input_node_idx];

    if (tensor_idx < 0 || tensor_idx >= ir_node->output_num)
    {
        return NULL;
    }

    return get_ir_graph_tensor(ir_node->graph, ir_node->output_tensors[tensor_idx]);
}

tensor_t get_graph_output_tensor(graph_t graph, int output_idx, int tensor_idx)
{
    struct graph* ir_graph = (struct graph*)graph;

    if (output_idx < 0 || output_idx >= ir_graph->output_num)
    {
        return NULL;
    }

    int output_node_idx = ir_graph->output_nodes[output_idx];

    struct node* ir_node = ir_graph->node_list[output_node_idx];

    if (tensor_idx < 0 || tensor_idx >= ir_node->output_num)
    {
        return NULL;
    }

    return get_ir_graph_tensor(ir_node->graph, ir_node->output_tensors[tensor_idx]);
}

/* ---- 节点的创建与查找 ---- */

/* 按名字创建节点；名字已存在或算子名非法时返回 NULL */
node_t create_graph_node(graph_t graph, const char* node_name, const char* op_name)
{
    struct graph* ir_graph = (struct graph*)graph;

    int node_idx = get_ir_node_index_from_name(ir_graph, node_name);

    if (node_idx >= 0)
    {
        return NULL;
    }

    int op_type = get_op_type_from_name(op_name);

    if (op_type < 0)
    {
        return NULL;
    }

    return create_ir_node(ir_graph, node_name, op_type, 1);
}

node_t get_graph_node(graph_t graph, const char* node_name)
{
    struct graph* ir_graph = (struct graph*)graph;

    int node_idx = get_ir_node_index_from_name(ir_graph, node_name);

    if (node_idx < 0)
    {
        return NULL;
    }

    return ir_graph->node_list[node_idx];
}

node_t get_graph_node_by_idx(graph_t graph, int idx)
{
    struct graph* ir_graph = (struct graph*)graph;

    if (idx < 0 || idx >= ir_graph->node_num)
        return NULL;

    return ir_graph->node_list[idx];
}

int get_graph_node_num(graph_t graph)
{
    struct graph* ir_graph = (struct graph*)graph;

    return ir_graph->node_num;
}

/* ---- 节点信息查询 ---- */

int get_node_output_number(node_t node)
{
    struct node* ir_node = (struct node*)node;

    return ir_node->output_num;
}

int get_node_input_number(node_t node)
{
    struct node* ir_node = (struct node*)node;

    return ir_node->input_num;
}

/* 节点名为空时按节点下标生成一个默认名字并记住 */
const char* get_node_name(node_t node)
{
    struct node* ir_node = (struct node*)node;

    if (ir_node->name)
    {
        return ir_node->name;
    }

    ir_node->name = create_ir_node_name_from_index(ir_node->index);

    return ir_node->name;
}

const char* get_node_op(node_t node)
{
    struct node* ir_node = (struct node*)node;

    int op_type = ir_node->op.type;

    return get_op_name_from_type(op_type);
}

/*
 * 查节点落在哪个设备上：图已切分子图时看节点所属子图的设备；
 * 否则看图绑定的设备。
 */
const char* get_node_device(node_t node)
{
    struct node* ir_node = (struct node*)node;
    struct graph* graph = ir_node->graph;

    int subgraph_count = get_vector_num(graph->subgraph_list);
    if (subgraph_count > 0)
    {
        if (0 <= ir_node->subgraph_idx)
        {
            struct subgraph* subgraph = get_ir_graph_subgraph(graph, ir_node->subgraph_idx);
            if (subgraph->device)
            {
                return subgraph->device->name;
            }
        }
    }
    else if (NULL != graph->device)
    {
        return graph->device->name;
    }

    return NULL;
}

/* ---- 节点属性访问（均为预留桩，尚未实现） ---- */

int get_node_attr_int(node_t node, const char* attr_name, int* attr_val)
{
    (void)node;
    (void)attr_name;
    (void)attr_val;

    return -1;
}

int get_node_attr_float(node_t node, const char* attr_name, float* attr_val)
{
    (void)node;
    (void)attr_name;
    (void)attr_val;

    return -1;
}

int get_node_attr_pointer(node_t node, const char* attr_name, void* attr_val)
{
    (void)node;
    (void)attr_name;
    (void)attr_val;

    return -1;
}

int get_node_attr_generic(node_t node, const char* attr_name, const char* type_name, void* buf, int size)
{
    (void)node;
    (void)attr_name;
    (void)type_name;
    (void)buf;
    (void)size;

    return -1;
}

int set_node_attr_int(node_t node, const char* attr_name, const int* attr_val)
{
    (void)node;
    (void)attr_name;
    (void)attr_val;

    return -1;
}

int set_node_attr_float(node_t node, const char* attr_name, const float* attr_val)
{
    (void)node;
    (void)attr_name;
    (void)attr_val;

    return -1;
}

int set_node_attr_pointer(node_t node, const char* attr_name, const void* attr_val)
{
    (void)node;
    (void)attr_name;
    (void)attr_val;

    return -1;
}

int set_node_attr_generic(node_t node, const char* attr_name, const char* type_name, const void* buf, int size)
{
    (void)node;
    (void)attr_name;
    (void)type_name;
    (void)buf;
    (void)size;

    return -1;
}

int add_node_attr(node_t node, const char* attr_name, const char* type_name, int size)
{
    (void)node;
    (void)attr_name;
    (void)type_name;
    (void)size;

    return -1;
}

/* 句柄由图持有，这里无需释放 */
void release_graph_node(node_t node)
{
    (void)node;
    // NOTHING NEEDS TO DO
}

/* ------------------------------------------------------------------ */
/* 第五部分：张量                                                       */
/* ------------------------------------------------------------------ */

/* ---- 节点与张量的连接 ---- */

tensor_t get_node_input_tensor(node_t node, int input_idx)
{
    struct node* ir_node = (struct node*)node;

    if (input_idx < 0 || input_idx >= ir_node->input_num)
    {
        return NULL;
    }

    return get_ir_graph_tensor(ir_node->graph, ir_node->input_tensors[input_idx]);
}

tensor_t get_node_output_tensor(node_t node, int output_idx)
{
    struct node* ir_node = (struct node*)node;

    if (output_idx < 0 || output_idx >= ir_node->output_num)
    {
        return NULL;
    }

    return get_ir_graph_tensor(ir_node->graph, ir_node->output_tensors[output_idx]);
}

/* 把张量接到节点的第 input_idx 个输入上 */
int set_node_input_tensor(node_t node, int input_idx, tensor_t tensor)
{
    struct node* ir_node = (struct node*)node;
    struct tensor* ir_tensor = (struct tensor*)tensor;

    return set_ir_node_input_tensor(ir_node, input_idx, ir_tensor);
}

/* 把张量接到节点的第 output_idx 个输出上，同时设置张量类型（输入/输出/普通） */
int set_node_output_tensor(node_t node, int output_idx, tensor_t tensor, int tensor_type)
{
    struct node* ir_node = (struct node*)node;
    struct tensor* ir_tensor = (struct tensor*)tensor;

    ir_tensor->tensor_type = tensor_type;

    return set_ir_node_output_tensor(ir_node, output_idx, ir_tensor);
}

/* ---- 张量的创建与查找 ---- */

tensor_t create_graph_tensor(graph_t graph, const char* tensor_name, int data_type)
{
    struct graph* ir_graph = (struct graph*)graph;

    return create_ir_tensor(ir_graph, tensor_name, data_type);
}

/* 在全图所有节点的输入/输出张量里按名字查找 */
tensor_t get_graph_tensor(graph_t graph, const char* tensor_name)
{
    struct graph* ir_graph = (struct graph*)graph;

    for (int i = 0; i < ir_graph->node_num; i++)
    {
        struct node* ir_node = get_ir_graph_node(ir_graph, i);
        if (NULL == ir_node)
        {
            continue;
        }

        for (int j = 0; j < ir_node->input_num; j++)
        {
            struct tensor* ir_tensor = get_ir_graph_tensor(ir_node->graph, ir_node->input_tensors[j]);
            if (ir_tensor && ir_tensor->name && !strcmp(ir_tensor->name, tensor_name))
                return (tensor_t)ir_tensor;
        }

        for (int j = 0; j < ir_node->output_num; j++)
        {
            struct tensor* ir_tensor = get_ir_graph_tensor(ir_node->graph, ir_node->output_tensors[j]);
            if (ir_tensor && ir_tensor->name && !strcmp(ir_tensor->name, tensor_name))
                return (tensor_t)ir_tensor;
        }
    }

    return NULL;
}

/* 张量名为空时按张量下标生成一个默认名字并记住 */
const char* get_tensor_name(tensor_t tensor)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    if (ir_tensor->name == NULL)
        ir_tensor->name = create_ir_tensor_name_from_index(ir_tensor->index);

    return ir_tensor->name;
}

/* 句柄由图持有，这里无需释放 */
void release_graph_tensor(tensor_t tensor)
{
    (void)tensor;
    // NOTHING NEEDS TO DO
}

/* ---- 形状与内存 ---- */

int set_tensor_shape(tensor_t tensor, const int dims[], int dim_number)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    return set_ir_tensor_shape(ir_tensor, dims, dim_number);
}

/* 把张量形状拷到 dims 里；dim_number 缓冲区不够时返回 -1，成功返回维度数 */
int get_tensor_shape(tensor_t tensor, int dims[], int dim_number)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    if (dim_number < ir_tensor->dim_num)
    {
        return -1;
    }

    for (int i = 0; i < ir_tensor->dim_num; i++)
        dims[i] = ir_tensor->dims[i];

    return ir_tensor->dim_num;
}

/* 张量数据占用的字节数 = 单元素大小 * 元素个数 */
int get_tensor_buffer_size(tensor_t tensor)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    return (int)(ir_tensor->elem_size * ir_tensor->elem_num);
}

/* 取张量数据指针（CPU 侧内存；设备内存场景尚未处理） */
void* get_tensor_buffer(tensor_t tensor)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    // TODO: take dev mem into consideration

    return ir_tensor->data;
}

/*
 * 把外部缓冲区挂到张量上（零拷贝）。要求缓冲区大小与张量严格一致；
 * 若张量原来持有自分配的内存，先释放。
 */
int set_tensor_buffer(tensor_t tensor, void* buffer, int buffer_size)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;
    int tensor_size = get_tensor_buffer_size(tensor);

    if (tensor_size != buffer_size)
    {
        fprintf(stderr, "Tengine: Size of tensor != size of buffer(%d vs %d).\n", tensor_size, buffer_size);
        return -1;
    }

    if (ir_tensor->data && ir_tensor->free_host_mem)
        sys_free(ir_tensor->data);

    ir_tensor->free_host_mem = 0;
    ir_tensor->internal_allocated = 0;
    ir_tensor->data = buffer;

    return 0;
}

/* 把张量数据拷出到 output_data（缓冲区须不小于张量大小） */
int get_tensor_data(tensor_t tensor, void* output_data, int data_size)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;
    int tensor_size = get_tensor_buffer_size(tensor);

    if (data_size < tensor_size)
    {
        return -1;
    }

    if (ir_tensor->data)
    {
        memcpy(output_data, ir_tensor->data, tensor_size);
        return 0;
    }

    if (ir_tensor->dev_mem == NULL)
    {
        return -1;
    }

    // TODO: handle dev_mem case

    return -1;
}

/*
 * 把 input_data 拷进张量，最多拷 data_size 字节（不允许超过张量大小）。
 * 传入的数据可以小于张量大小，此时只覆盖前面部分。
 */
int set_tensor_data(tensor_t tensor, const void* input_data, int data_size)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;
    int tensor_size = get_tensor_buffer_size(tensor);

    if (data_size > tensor_size)
    {
        return -1;
    }

    if (ir_tensor->data)
    {
        memcpy(ir_tensor->data, input_data, data_size);
        return 0;
    }

    // TODO: handle dev_mem case
    return -1;
}

/* ---- 数据类型 / 布局 / 量化参数 ---- */

int get_tensor_data_type(tensor_t tensor)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    return ir_tensor->data_type;
}

int set_tensor_data_type(tensor_t tensor, int data_type)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    ir_tensor->data_type = data_type;

    return 0;
}

int get_tensor_layout(tensor_t tensor)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    return ir_tensor->layout;
}

int set_tensor_layout(tensor_t tensor, int layout)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    ir_tensor->layout = layout;

    return 0;
}

int set_tensor_quant_param(tensor_t tensor, const float* scale, const int* zero_point, int number)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    return set_ir_tensor_quantization_parameter(ir_tensor, scale, zero_point, number);
}

int get_tensor_quant_param(tensor_t tensor, float* scale, int* zero_point, int number)
{
    struct tensor* ir_tensor = (struct tensor*)tensor;

    return get_ir_tensor_quantization_parameter(ir_tensor, scale, zero_point, number);
}

/* ------------------------------------------------------------------ */
/* 第六部分：杂项                                                       */
/* ------------------------------------------------------------------ */

/* HCL（混合低精度库）版本号，仅在开启 TENGINE_ONLINE_REPORT 时有值 */
const char* get_tengine_hcl_version()
{
    return hcl_version;
}

/* 预留接口，尚未实现 */
int set_default_device(const char* device)
{
    (void)device;

    return -1;
}

void set_log_level(enum log_level level)
{
    SET_LOG_LEVEL(level);
}

void set_log_output(log_print_t func)
{
    SET_LOG_OUTPUT(func);
}

/* 预留接口，尚未实现 */
int get_tengine_errno(void)
{
    return -1;
}

/* 预留接口，尚未实现 */
int clr_tengine_errno(void)
{
    return -1;
}

/* 取某个 CPU 簇（大核/小核）的亲和性掩码，用于 set_context_device 等 */
size_t get_cluster_affinity_mask(int cluster)
{
    check_cpu();
    return get_cpu_cluster_mask(cluster);
}

/* ------------------------------------------------------------------ */
/* 第七部分：自定义算子（预留桩）                                        */
/* ------------------------------------------------------------------ */

/* 预留接口，尚未实现 */
int set_custom_kernel(node_t node, const char* dev_name, struct custom_kernel_ops* kernel_ops)
{
    (void)node;
    (void)dev_name;
    (void)kernel_ops;

    // TODO: set custom kernel
    return -1;
}

/* 预留接口，尚未实现 */
int remove_custom_kernel(node_t node, const char* dev_name)
{
    (void)node;
    (void)dev_name;

    // TODO: remove custom kernel
    return -1;
}
