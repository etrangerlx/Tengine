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
 * TM2 是 Tengine 自有的二进制模型格式，本文件是它的序列化器
 * （在模块注册表里的名字叫 "tengine"，见 module/module.c）。
 *
 * TM2 文件在内存中的布局（结构定义见 tm2_format.h）：
 *
 *     TM2_Header          文件头：魔数、版本、根节点偏移
 *       -> TM2_Model      模型头：子图列表偏移
 *         -> TM2_Subgraph 子图：张量 / 节点 / 缓冲区等列表的偏移
 *
 * 关键特点：所有字段存的都是“相对模型内存基址的偏移量”而不是指针。
 * 整个文件一次性读入内存（或用户直接给内存），之后零拷贝解析——
 * 取结构都是 base + offset 的强制转换。TM2_NOT_SET(=0) 表示字段未设置，
 * 使用任何偏移前必须先判断它是不是 TM2_NOT_SET。
 *
 * 加载流程（load_graph）：
 *   1. load_graph_tensors  建张量：名字、形状、常量数据、量化参数；
 *   2. load_graph_nodes    建节点：找算子的 op loader 解析参数、连张量；
 *   3. set_graph_io_nodes  设置整图的输入 / 输出节点；
 *   4. load_graph_sub_info 多子图模型的子图信息（单图模型跳过）。
 *
 * NHWC -> NCHW 布局转换：引擎内部统一用 NCHW。若模型是 NHWC 布局，
 * 加载时要做三件事：张量形状重排、常量权重数据重排（见第二部分的
 * permute_weight_nhwc_to_nchw）、个别算子的轴参数修正（见第三部分）。
 * ======================================================================
 */

#include "tm2_serializer.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <fcntl.h>

#include "graph/tensor.h"
#include "graph/node.h"
#include "graph/graph.h"
#include "graph/subgraph.h"
#include "module/module.h"
#include "device/device.h"
#include "serializer/serializer.h"
#include "utility/sys_port.h"
#include "utility/vector.h"
#include "utility/log.h"

#include <string.h>

/* 一条算子解析规则：TM2 算子类型 -> IR 算子的映射 + 参数解析函数 */
struct op_loader_entry
{
    int op_type;
    int op_version; /* 注意：查找时当前只按 op_type 匹配，此字段未参与比较 */
    tm2_op_loader_t loader;
    tm2_map_t op_map;
    tm2_map_t ver_map;
};

struct tm2_serializer
{
    struct serializer base;

    struct vector* loader_list;
};

static const char* tm2_name = "tengine";

static int unload_graph(struct serializer* s, struct graph* graph, void* s_priv, void* dev_priv);

/* ------------------------------------------------------------------ */
/* 第一部分：内部辅助                                                    */
/* ------------------------------------------------------------------ */

/* 从模型内存里拷出一段定长字符串（补上结尾 '\0'）作为名字 */
static char* strdup_name(char* buf, int size)
{
    char* p = (char*)sys_malloc(size + 1);
    if (NULL == p)
    {
        return NULL;
    }

    memcpy(p, buf, size);
    p[size] = 0x0;

    return p;
}

/* ---- 下面三个 inline 函数：沿着“偏移量链”定位各级结构 ---- */

static inline const TM2_Header* get_tm_file_header(const char* base)
{
    return (const TM2_Header*)(base);
}

static inline const TM2_Model* get_tm_file_model(const char* base, const TM2_Header* header)
{
    return (const TM2_Model*)(base + header->offset_root);
}

static inline const TM2_Subgraph* get_tm_file_subgraph(const char* base, const TM2_Model* model)
{
    const TM2_Vector_offsets* v_graphs = (TM2_Vector_offsets*)(base + model->offset_vo_subgraphs);
    const TM2_Subgraph* tm_graph = (TM2_Subgraph*)(base + v_graphs->offsets[0]);

    return tm_graph;
}

/* ---- 算子解析规则的注册表管理 ---- */

/*
 * 找某种算子的解析规则。
 * 注意：当前只按 op_type 匹配，op_version 参数未参与比较（历史行为）。
 */
static struct op_loader_entry* find_op_loader(struct tm2_serializer* s, int op_type, int op_version)
{
    (void)op_version;

    int loader_num = get_vector_num(s->loader_list);

    for (int i = 0; i < loader_num; i++)
    {
        struct op_loader_entry* e = (struct op_loader_entry*)get_vector_data(s->loader_list, i);

        if (e->op_type == op_type)
            return e;
    }

    return NULL;
}

static int register_tm2_op_loader(struct tm2_serializer* s, int op_type, int op_version, tm2_op_loader_t op_loader,
                                  tm2_map_t op_map, tm2_map_t ver_map)
{
    if (find_op_loader(s, op_type, op_version) != NULL)
    {
        TLOG_DEBUG("serializer: op: %d version %d has loader already\n", op_type, op_version);
        return -1;
    }

    struct op_loader_entry e;

    e.op_type = op_type;
    e.op_version = op_version;
    e.loader = op_loader;
    e.op_map = op_map;
    e.ver_map = ver_map;

    push_vector_data(s->loader_list, &e);

    return 0;
}

static int unregister_tm2_op_loader(struct tm2_serializer* s, int op_type, int op_version, tm2_op_loader_t op_loader)
{
    int n = get_vector_num(s->loader_list);

    for (int i = 0; i < n; i++)
    {
        struct op_loader_entry* e = (struct op_loader_entry*)get_vector_data(s->loader_list, i);

        if (e->op_type == op_type && e->loader == op_loader)
        {
            remove_vector_via_pointer(s->loader_list, e);
            return 0;
        }
    }

    return -1;
}

/* ------------------------------------------------------------------ */
/* 第二部分：张量加载                                                    */
/* ------------------------------------------------------------------ */

/*
 * 把 4D 权重数据从文件里的 NHWC 排布重排成 NCHW（元素就地写到输出缓冲）。
 * 两种排布（与生成模型的转换工具保持一致）：
 *   - cin == 1（深度可分离卷积权重等）：按 (h, w, cout) 排布；
 *   - cin >  1（普通卷积权重）：按 (cout, h, w, cin) 排布。
 * elem_size 是单个元素的字节数（FP32 为 4，INT8/UINT8 为 1）。
 */
static void permute_weight_nhwc_to_nchw(void* output, const void* input, int elem_size, int cout, int cin, int h,
                                        int w)
{
    const unsigned char* in = (const unsigned char*)input;
    unsigned char* out = (unsigned char*)output;

    if (cin == 1)
    {
        for (int co = 0; co < cout; co++)
        {
            for (int hi = 0; hi < h; hi++)
            {
                for (int wi = 0; wi < w; wi++)
                {
                    int offset_org = co + hi * w * cout + wi * cout;
                    int offset = co * h * w + hi * w + wi;

                    memcpy(out + offset * elem_size, in + offset_org * elem_size, elem_size);
                }
            }
        }
    }
    else
    {
        for (int co = 0; co < cout; co++)
        {
            for (int ci = 0; ci < cin; ci++)
            {
                for (int hi = 0; hi < h; hi++)
                {
                    for (int wi = 0; wi < w; wi++)
                    {
                        int offset_org = co * cin * h * w + ci + hi * w * cin + wi * cin;
                        int offset = co * cin * h * w + ci * h * w + hi * w + wi;

                        memcpy(out + offset * elem_size, in + offset_org * elem_size, elem_size);
                    }
                }
            }
        }
    }
}

/*
 * 遍历模型里的所有张量，在 IR 图里逐一重建。
 * 每个张量依次处理四类信息：名字、形状、常量数据、量化参数。
 */
static int load_graph_tensors(struct tm2_serializer* tm2_s, struct graph* graph, struct tm2_priv* priv)
{
    (void)tm2_s;

    char* mem_base = (char*)priv->base;
    const TM2_Subgraph* tm_graph = priv->subgraph;

    const TM2_Vector_offsets* v_tensors = (TM2_Vector_offsets*)(mem_base + tm_graph->offset_vo_tensors);
    const TM2_Vector_offsets* v_buffers = (TM2_Vector_offsets*)(mem_base + tm_graph->offset_vo_buffers);

    graph->graph_layout = tm_graph->graph_layout;
    graph->model_layout = tm_graph->model_layout;

    /* 引擎内部统一用 NCHW；NHWC 模型改记为 NCHW，数据在下面逐个重排 */
    if (graph->graph_layout == TENGINE_LAYOUT_NHWC)
    {
        graph->graph_layout = TENGINE_LAYOUT_NCHW;
        TLOG_DEBUG("permute layout: graph_layout from nhwc to nchw\n");
    }

    for (int i = 0; i < v_tensors->v_num; i++)
    {
        const TM2_Tensor* tm_tensor = (TM2_Tensor*)(mem_base + v_tensors->offsets[i]);
        int flag_permute = 0; /* 形状做过 NHWC->NCHW 重排的 4 维张量，其常量数据也要重排 */

        struct tensor* ir_tensor = create_ir_tensor(graph, NULL, tm_tensor->data_type);

        if (ir_tensor == NULL)
        {
            return -1;
        }

        ir_tensor->tensor_type = tm_tensor->type;

        /* 1. 名字 */
        if (tm_tensor->offset_s_tname != TM2_NOT_SET)
        {
            const TM2_String* tm_str = (TM2_String*)(mem_base + tm_tensor->offset_s_tname);
            ir_tensor->name = strdup_name(mem_base + tm_str->offset_data, tm_str->size);
        }

        /* 2. 形状：NHWC 模型的 4 维张量要重排成 NCHW */
        if (tm_tensor->offset_vd_dims != TM2_NOT_SET)
        {
            const TM2_Vector_dims* v_dims = (TM2_Vector_dims*)(mem_base + tm_tensor->offset_vd_dims);

            if (tm_graph->model_layout == TENGINE_LAYOUT_NCHW || v_dims->v_num != 4)
            {
                set_ir_tensor_shape(ir_tensor, v_dims->dims, v_dims->v_num);
            }
            else
            {
                /* (n, h, w, c) -> (n, c, h, w) */
                int dims[4];

                dims[0] = v_dims->dims[0]; // n
                dims[1] = v_dims->dims[3]; // c
                dims[2] = v_dims->dims[1]; // h
                dims[3] = v_dims->dims[2]; // w

                set_ir_tensor_shape(ir_tensor, dims, 4);

                flag_permute = 1;
            }
        }

        /* 3. 常量数据（卷积的 weight / bias 等），从模型缓冲区里取 */
        if (ir_tensor->tensor_type == TENSOR_TYPE_CONST)
        {
            if (tm_tensor->buffer_id < 0 || tm_tensor->buffer_id >= v_buffers->v_num)
            {
                TLOG_ERR("serializer: const tensor buffer id %d is out of range(%d)\n", tm_tensor->buffer_id,
                         v_buffers->v_num);
                return -1;
            }

            const TM2_Buffer* tm_buf = (TM2_Buffer*)(mem_base + v_buffers->offsets[tm_tensor->buffer_id]);

            /* 模型里没带数据：分配一块清零的缓冲区（供 benchmark 占位用） */
            if (tm_buf->offset_data == TM2_NOT_SET)
            {
                ir_tensor->data = sys_malloc(ir_tensor->elem_num * ir_tensor->elem_size);
                if (NULL == ir_tensor->data)
                {
                    TLOG_ERR("serializer: allocate const tensor data failed\n");
                    return -1;
                }
                memset(ir_tensor->data, 0, ir_tensor->elem_num * ir_tensor->elem_size);
                ir_tensor->free_host_mem = 1;
            }
            else
            {
                /* 数据直接指向模型内存（零拷贝），校验模型里声明的尺寸够不够 */
                ir_tensor->data = mem_base + tm_buf->offset_data;

                if (ir_tensor->elem_size * ir_tensor->elem_num > tm_buf->size)
                {
                    TLOG_ERR("serializer: const tensor size in model is too small\n");
                    return -1;
                }

                /* NHWC 模型的权重数据同样要重排成 NCHW */
                if (flag_permute)
                {
                    int size = ir_tensor->elem_num;
                    int type = ir_tensor->data_type;

                    /* 当前仅支持 FP32 / UINT8 / INT8 三种数据类型的重排 */
                    int elem_size = 0;
                    if (TENGINE_DT_FP32 == type)
                        elem_size = (int)sizeof(float);
                    else if (TENGINE_DT_UINT8 == type || TENGINE_DT_INT8 == type)
                        elem_size = (int)sizeof(unsigned char);

                    if (elem_size > 0)
                    {
                        /* 输入输出是同一块内存，必须先拷到临时缓冲区再重排 */
                        void* tensor_data_org = sys_malloc((size_t)size * elem_size);
                        if (NULL == tensor_data_org)
                        {
                            TLOG_ERR("serializer: allocate temp buffer for weight permute failed\n");
                            return -1;
                        }

                        memcpy(tensor_data_org, ir_tensor->data, (size_t)size * elem_size);

                        permute_weight_nhwc_to_nchw(ir_tensor->data, tensor_data_org, elem_size, ir_tensor->dims[0],
                                                    ir_tensor->dims[1], ir_tensor->dims[2], ir_tensor->dims[3]);

                        sys_free(tensor_data_org);
                    }
                }
            }
        }

        /* 4. 量化参数：一个时用标量字段，多个时用列表 */
        if (tm_tensor->offect_vo_quantparams != TM2_NOT_SET)
        {
            const TM2_Vector_offsets* v_quantparams = (TM2_Vector_offsets*)(mem_base + tm_tensor->offect_vo_quantparams);

            ir_tensor->quant_param_num = v_quantparams->v_num;

            if (v_quantparams->v_num == 1)
            {
                const TM2_QuantParam* tm_qtparam = (TM2_QuantParam*)(mem_base + v_quantparams->offsets[0]);
                ir_tensor->scale = tm_qtparam->scale;
                ir_tensor->zero_point = tm_qtparam->zero_point;
            }
            else if (v_quantparams->v_num > 1)
            {
                ir_tensor->scale_list = (float*)sys_malloc(sizeof(float) * v_quantparams->v_num);
                ir_tensor->zp_list = (int*)sys_malloc(sizeof(int) * v_quantparams->v_num);

                if (NULL == ir_tensor->scale_list || NULL == ir_tensor->zp_list)
                {
                    TLOG_ERR("serializer: allocate quant param list failed\n");
                    return -1;
                }

                for (int j = 0; j < v_quantparams->v_num; j++)
                {
                    const TM2_QuantParam* tm_qtparam = (TM2_QuantParam*)(mem_base + v_quantparams->offsets[j]);
                    ir_tensor->scale_list[j] = tm_qtparam->scale;
                    ir_tensor->zp_list[j] = tm_qtparam->zero_point;
                }
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* 第三部分：节点加载                                                    */
/* ------------------------------------------------------------------ */

/* NHWC 模型：修正个别算子里与维度顺序相关的参数；失败时置 *failed */
static void fix_op_param_nhwc_to_nchw(int op_type, char* mem_base, const TM2_Operator* tm_operator, int* failed)
{
    if (op_type == TM2_OPTYPE_SOFTMAX)
    {
        TM2_SoftmaxParam* tm_param = (TM2_SoftmaxParam*)(mem_base + tm_operator->offset_t_param);

        if (tm_param->axis == 3) /* NHWC 的 W 轴就是 NCHW 的 C 轴 */
            tm_param->axis = 1;
        else
            fprintf(stderr, "softmax, axis is not 3, need todo something\n");
    }

    // TODO: CONCAT 的 axis 映射暂未实现

    if (op_type == TM2_OPTYPE_REDUCTION)
    {
        TM2_ReductionParam* tm_param = (TM2_ReductionParam*)(mem_base + tm_operator->offset_t_param);

        if (tm_param->dim_0 == 1 && tm_param->dim_1 == 2)
        {
            tm_param->dim_0 = 2;
            tm_param->dim_1 = 3;
        }
        else if (tm_param->dim_0 == -1)
        {
            tm_param->dim_0 = 4;
        }
        else
        {
            fprintf(stderr, "reduction, nhwc to nchw, need todo something\n");
        }
    }

    if (op_type == TM2_OPTYPE_PAD)
    {
        TM2_PadParam* tm_param = (TM2_PadParam*)(mem_base + tm_operator->offset_t_param);

        /* 模型里的字段顺序是 n,h,w,c，内部布局是 n,c,h,w，交换即可 */
        int pads[8] = {0};
        pads[0] = tm_param->pad_n_0;
        pads[1] = tm_param->pad_n_1;
        pads[2] = tm_param->pad_c_0; // h
        pads[3] = tm_param->pad_c_1;
        pads[4] = tm_param->pad_h_0; // w
        pads[5] = tm_param->pad_h_1;
        pads[6] = tm_param->pad_w_0; // c
        pads[7] = tm_param->pad_w_1;

        tm_param->pad_c_0 = pads[6];
        tm_param->pad_c_1 = pads[7];
        tm_param->pad_h_0 = pads[2];
        tm_param->pad_h_1 = pads[3];
        tm_param->pad_w_0 = pads[4];
        tm_param->pad_w_1 = pads[5];
    }

    if (op_type == TM2_OPTYPE_STRIDEDSLICE)
    {
        TM2_StridedSliceParam* tm_param = (TM2_StridedSliceParam*)(mem_base + tm_operator->offset_t_param);

        int begin[4] = {0};
        int end[4] = {0};
        int stride[4] = {0};

        /* 模型里的字段顺序是 n,w,c,h，内部布局是 n,c,h,w */
        begin[0] = tm_param->begin_n;
        begin[1] = tm_param->begin_w;
        begin[2] = tm_param->begin_c;
        begin[3] = tm_param->begin_h;

        end[0] = tm_param->end_n;
        end[1] = tm_param->end_w;
        end[2] = tm_param->end_c;
        end[3] = tm_param->end_h;

        stride[0] = tm_param->stride_n;
        stride[1] = tm_param->stride_w;
        stride[2] = tm_param->stride_c;
        stride[3] = tm_param->stride_h;

        tm_param->begin_n = begin[0];
        tm_param->begin_c = begin[1];
        tm_param->begin_h = begin[2];
        tm_param->begin_w = begin[3];

        tm_param->end_n = end[0];
        tm_param->end_c = end[1];
        tm_param->end_h = end[2];
        tm_param->end_w = end[3];

        tm_param->stride_n = stride[0];
        tm_param->stride_c = stride[1];
        tm_param->stride_h = stride[2];
        tm_param->stride_w = stride[3];
    }

    if (op_type == TM2_OPTYPE_RESHAPE)
    {
        TM2_ReshapeParam* tm_param = (TM2_ReshapeParam*)(mem_base + tm_operator->offset_t_param);

        if (tm_param->offset_re_shape == TM2_NOT_SET)
        {
            fprintf(stderr, "the version of reshape is older, todo support\n");
            *failed = 1;
            return;
        }

        TM2_Vector_dims* v_reshape = (TM2_Vector_dims*)(mem_base + tm_param->offset_re_shape);
        int reshape_num = v_reshape->v_num;

        if (reshape_num > MAX_SHAPE_DIM_NUM * 2)
        {
            fprintf(stderr, "the reshape dims is to bigger than 8\n");
            *failed = 1;
            return;
        }

        if (reshape_num >= 5)
        {
            fprintf(stderr, "nhwc reshape num >= 5, todo support\n");
            *failed = 1;
            return;
        }

        int dims[MAX_SHAPE_DIM_NUM * 2] = {0};
        for (int j = 0; j < reshape_num; j++)
            dims[j] = v_reshape->dims[j];

        /* (n, h, w) -> (n, c=w, h, w=h)：交换中间两维 */
        if (reshape_num == 3)
        {
            v_reshape->dims[0] = dims[0];
            v_reshape->dims[1] = dims[2];
            v_reshape->dims[2] = dims[1];
        }

        /* (n, h, w, c) -> (n, c, h, w) */
        if (reshape_num == 4)
        {
            v_reshape->dims[0] = dims[0];
            v_reshape->dims[1] = dims[3];
            v_reshape->dims[2] = dims[1];
            v_reshape->dims[3] = dims[2];
        }
    }
}

/*
 * 遍历模型里的所有节点，在 IR 图里逐一重建：
 * 找到该算子的解析规则 -> 建节点 -> 起名字 -> 连输入输出张量
 * -> 必要时修正 NHWC 参数 -> 调用算子自己的 loader 解析参数。
 * 任一节点失败则整体失败（返回 -1）。
 */
static int load_graph_nodes(struct tm2_serializer* tm2_s, struct graph* ir_graph, struct tm2_priv* priv)
{
    char* mem_base = (char*)priv->base;
    const TM2_Subgraph* tm_graph = priv->subgraph;
    const TM2_Vector_offsets* v_nodes = (TM2_Vector_offsets*)(mem_base + tm_graph->offset_vo_seq_nodes);

    unsigned int i;

    for (i = 0; i < v_nodes->v_num; i++)
    {
        const TM2_Node* tm_node = (TM2_Node*)(mem_base + v_nodes->offsets[i]);
        const TM2_Operator* tm_operator = (TM2_Operator*)(mem_base + tm_node->offset_t_operator);
        int op_type = tm_operator->operator_type;
        int op_version = tm_operator->op_ver;

        /* 找这个算子的解析规则（由各算子在注册原型时挂进来） */
        struct op_loader_entry* e = find_op_loader(tm2_s, op_type, op_version);

        if (e == NULL)
        {
            TLOG_ERR("serializer: cannot find op loader for op: %d version: %d\n", op_type, op_version);
            break;
        }

        /* 算子类型 / 版本映射：TM2 编号 -> IR 编号 */
        int op_type_mapped = op_type;

        if (e->op_map)
            op_type_mapped = e->op_map(op_type);

        int op_ver_mapped = op_version;

        if (e->ver_map)
            op_ver_mapped = e->ver_map(op_version);

        struct node* ir_node = create_ir_node(ir_graph, NULL, op_type_mapped, op_ver_mapped);

        if (ir_node == NULL)
        {
            break;
        }

        if (tm_node->offset_s_nname != TM2_NOT_SET)
        {
            const TM2_String* str = (TM2_String*)(mem_base + tm_node->offset_s_nname);
            ir_node->name = strdup_name(mem_base + str->offset_data, str->size);
        }

        /* 本节点是否加载失败：失败时跳出大循环，整体返回 -1 */
        int node_failed = 0;

        /* 连接输入张量（按模型里的下标列表） */
        if (tm_node->offset_vi_input_tensors != TM2_NOT_SET)
        {
            const TM2_Vector_indices* v_input_tensors = (TM2_Vector_indices*)(mem_base + tm_node->offset_vi_input_tensors);

            for (int j = 0; j < v_input_tensors->v_num; j++)
            {
                int tensor_idx = v_input_tensors->indices[j];

                if (tensor_idx < 0 || tensor_idx >= ir_graph->tensor_num)
                {
                    TLOG_ERR("invalid input tensor slot: %d idx: %d for node: %d\n", j, tensor_idx, ir_node->index);
                    node_failed = 1;
                    break;
                }

                struct tensor* ir_tensor = get_ir_graph_tensor(ir_graph, tensor_idx);

                set_ir_node_input_tensor(ir_node, j, ir_tensor);
            }
        }

        /* 连接输出张量；完全没有输出的节点视为模型错误 */
        if (!node_failed && tm_node->offset_vi_output_tensors == TM2_NOT_SET)
        {
            TLOG_ERR("node: %d has no output\n", ir_node->index);
            node_failed = 1;
        }

        if (!node_failed)
        {
            const TM2_Vector_indices* v_output_tensors = (TM2_Vector_indices*)(mem_base + tm_node->offset_vi_output_tensors);

            for (int j = 0; j < v_output_tensors->v_num; j++)
            {
                int tensor_idx = v_output_tensors->indices[j];

                if (tensor_idx < 0 || tensor_idx >= ir_graph->tensor_num)
                {
                    TLOG_ERR("invalid output tensor slot: %d idx: %d for node: %d\n", j, tensor_idx, ir_node->index);
                    node_failed = 1;
                    break;
                }

                struct tensor* ir_tensor = get_ir_graph_tensor(ir_graph, tensor_idx);

                set_ir_node_output_tensor(ir_node, j, ir_tensor);
            }
        }

        /* NHWC 模型：修正个别算子的轴参数（失败时置位 node_failed） */
        if (!node_failed && tm_graph->graph_layout == TENGINE_LAYOUT_NHWC)
        {
            fix_op_param_nhwc_to_nchw(op_type, mem_base, tm_operator, &node_failed);
        }

        /* 调用算子自己的参数解析函数 */
        if (!node_failed && e->loader != NULL_TM2_OP_LOADER && e->loader(ir_graph, ir_node, tm_node, tm_operator) < 0)
        {
            TLOG_ERR("failed to load op: %d version: %d for node: %d\n", op_type, op_version, ir_node->index);
            node_failed = 1;
        }

        if (node_failed)
        {
            break;
        }
    }

    if (i < v_nodes->v_num)
    {
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* 第四部分：图 IO 与子图信息                                            */
/* ------------------------------------------------------------------ */

/* 把模型里记录的输入 / 输出节点下标列表设置到 IR 图上 */
static int set_graph_io_nodes(struct tm2_serializer* tm2_s, struct graph* ir_graph, struct tm2_priv* priv)
{
    (void)tm2_s;

    char* mem_base = (char*)priv->base;
    const TM2_Subgraph* tm_graph = priv->subgraph;
    const TM2_Vector_indices* v_input_nodes = (TM2_Vector_indices*)(mem_base + tm_graph->offset_vi_input_indices);
    const TM2_Vector_indices* v_output_nodes = (TM2_Vector_indices*)(mem_base + tm_graph->offset_vi_output_indices);

    /* 输入节点 */
    int16_t* node_idx = (int16_t*)sys_malloc(sizeof(int16_t) * v_input_nodes->v_num);

    if (node_idx == NULL)
    {
        return -1;
    }

    for (unsigned int i = 0; i < v_input_nodes->v_num; i++)
    {
        node_idx[i] = v_input_nodes->indices[i];
    }

    set_ir_graph_input_node(ir_graph, node_idx, v_input_nodes->v_num);

    sys_free(node_idx);

    /* 输出节点 */
    node_idx = (int16_t*)sys_malloc(sizeof(int16_t) * v_output_nodes->v_num);

    if (node_idx == NULL)
    {
        return -1;
    }

    for (unsigned int i = 0; i < v_output_nodes->v_num; i++)
    {
        node_idx[i] = v_output_nodes->indices[i];
    }

    set_ir_graph_output_node(ir_graph, node_idx, v_output_nodes->v_num);

    sys_free(node_idx);

    return 0;
}

/*
 * 多子图模型的子图信息（单图模型没有这段，直接跳过）。
 * 每个子图：节点列表 + 输入输出张量列表 + 绑定的设备名。
 */
static int load_graph_sub_info(struct tm2_serializer* s, struct graph* graph, struct tm2_priv* priv)
{
    (void)s;

    char* mem_base = (char*)priv->base;
    const TM2_Vector_offsets* v_graphs = (TM2_Vector_offsets*)(mem_base + priv->model->offset_vo_subgraphs);
    const TM2_Subgraph* tm_graph = priv->subgraph;

    /* 没有子图信息段，或整图只有一个子图：无需处理 */
    if (TM2_NOT_SET == tm_graph->offset_vo_sub_info || 1 == v_graphs->v_num)
    {
        return 0;
    }

    fprintf(stderr, "---load graph sub info!---\n");

    const TM2_Vector_offsets* v_sub_info = (TM2_Vector_offsets*)(mem_base + tm_graph->offset_vo_sub_info);

    int sub_graph_num = v_sub_info->v_num;
    for (int i = 0; i < sub_graph_num; i++)
    {
        struct subgraph* subgraph = (struct subgraph*)sys_malloc(sizeof(struct subgraph));
        if (NULL == subgraph)
        {
            TLOG_ERR("serializer: allocate subgraph failed\n");
            return -1;
        }

        /* 先显式清空三个列表指针，保证中途失败时能安全释放 */
        subgraph->node_list = NULL;
        subgraph->input_tensor_list = NULL;
        subgraph->output_tensor_list = NULL;

        init_ir_subgraph(graph, subgraph, i);

        TM2_Sub_Info* sub_info = (TM2_Sub_Info*)(mem_base + v_sub_info->offsets[i]);
        subgraph->index = sub_info->subgraph_id;
        subgraph->input_wait_count = sub_info->input_wait_count;

        // to do. data type? device type?
        subgraph->device = find_default_device();

        /* 节点列表 */
        TM2_Vector_indices* v_node_list = (TM2_Vector_indices*)(mem_base + sub_info->offset_vi_node_list);
        subgraph->node_num = v_node_list->v_num;
        subgraph->node_list = (uint16_t*)sys_malloc(sizeof(uint16_t) * subgraph->node_num);
        if (NULL == subgraph->node_list)
        {
            release_ir_subgraph(graph, subgraph);
            return -1;
        }
        for (int j = 0; j < v_node_list->v_num; j++)
        {
            subgraph->node_list[j] = v_node_list->indices[j];
        }

        /* 输入张量列表 */
        TM2_Vector_indices* v_input_tensor = (TM2_Vector_indices*)(mem_base + sub_info->offset_vi_input_tensor);
        subgraph->input_num = v_input_tensor->v_num;
        subgraph->input_tensor_list = (uint16_t*)sys_malloc(sizeof(uint16_t) * subgraph->input_num);
        if (NULL == subgraph->input_tensor_list)
        {
            release_ir_subgraph(graph, subgraph);
            return -1;
        }
        for (int j = 0; j < v_input_tensor->v_num; j++)
        {
            subgraph->input_tensor_list[j] = v_input_tensor->indices[j];
        }

        /* 输出张量列表 */
        TM2_Vector_indices* v_output_tensor = (TM2_Vector_indices*)(mem_base + sub_info->offset_vi_output_tensor);
        subgraph->output_num = v_output_tensor->v_num;
        subgraph->output_tensor_list = (uint16_t*)sys_malloc(sizeof(uint16_t) * subgraph->output_num);
        if (NULL == subgraph->output_tensor_list)
        {
            release_ir_subgraph(graph, subgraph);
            return -1;
        }
        for (int j = 0; j < v_output_tensor->v_num; j++)
        {
            subgraph->output_tensor_list[j] = v_output_tensor->indices[j];
        }

        // dump sub info
        fprintf(stderr, "sub id:%d, node num: %d, input num:%d, output num:%d, input wait count:%d, device name:%s\n",
                subgraph->index, subgraph->node_num, subgraph->input_num, subgraph->output_num,
                subgraph->input_wait_count, subgraph->device->name);

        push_vector_data(graph->subgraph_list, &subgraph);
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* 第五部分：模型加载总装                                                */
/* ------------------------------------------------------------------ */

/* 总装：按 张量 -> 节点 -> IO -> 子图信息 的顺序重建整张图 */
static int load_graph(struct serializer* s, struct graph* graph, struct tm2_priv* priv)
{
    struct tm2_serializer* tm2_s = (struct tm2_serializer*)s;

    /* 版本检查：只认 TM2 主版本 */
    if (priv->header->ver_main != TM2_FILE_VER_MAIN)
    {
        TLOG_ERR("model is not version 2\n");
        return -1;
    }

    if (load_graph_tensors(tm2_s, graph, priv) < 0)
        goto error;

    if (load_graph_nodes(tm2_s, graph, priv) < 0)
        goto error;

    if (set_graph_io_nodes(tm2_s, graph, priv) < 0)
        goto error;

    if (load_graph_sub_info(tm2_s, graph, priv) < 0)
        goto error;

    return 0;

error:
    /* 释放模型内存；半成品图由调用方（create_graph_error）负责销毁 */
    unload_graph(s, graph, priv, NULL);
    return -1;
}

/* 从文件加载：整个文件读入一块堆内存，再走 load_graph */
static int load_model(struct serializer* s, struct graph* graph, const char* fname, va_list ap)
{
    (void)ap;

    struct stat file_stat;

    int fd = open(fname, O_RDONLY);

    if (fd < 0)
    {
        TLOG_ERR("cannot open file %s\n", fname);
        return -1;
    }

    fstat(fd, &file_stat);

    int file_len = file_stat.st_size;

    /* 一次性把文件读进内存（历史上用 mmap，现改为 read，便于统一释放） */
    void* mem_base = (void*)sys_malloc(file_len);
    if (NULL == mem_base)
    {
        TLOG_ERR("serializer: allocate %d bytes for model failed\n", file_len);
        close(fd);
        return -1;
    }

    if (read(fd, mem_base, file_len) != file_len)
    {
        TLOG_ERR("serializer: read model file %s failed\n", fname);
        sys_free(mem_base);
        close(fd);
        return -1;
    }

    struct tm2_priv* priv = (struct tm2_priv*)sys_malloc(sizeof(struct tm2_priv));

    if (priv == NULL)
    {
        sys_free(mem_base);
        close(fd);
        return -1;
    }

    priv->fd = fd;
    priv->mem_len = file_len;
    priv->base = (const char*)mem_base;
    priv->self_allocated = 1; /* 内存是本模块分配的，unload 时要释放 */
    priv->header = get_tm_file_header((const char*)mem_base);
    priv->model = get_tm_file_model((const char*)mem_base, priv->header);
    priv->subgraph = get_tm_file_subgraph((const char*)mem_base, priv->model);

    graph->serializer = s;
    graph->serializer_privacy = priv;
    graph->device_privacy = NULL;

    return load_graph(s, graph, priv);
}

/* 从内存加载：模型缓冲区由调用者提供并负责释放 */
static int load_mem(struct serializer* s, struct graph* graph, const void* addr, int size, va_list ap)
{
    (void)ap;

    struct tm2_priv* priv = (struct tm2_priv*)sys_malloc(sizeof(struct tm2_priv));

    if (priv == NULL)
    {
        return -1;
    }

    priv->fd = -1;
    priv->mem_len = size;
    priv->base = (const char*)addr;
    priv->self_allocated = 0; /* 内存属于调用者，unload 时不能释放 */
    priv->header = get_tm_file_header((const char*)addr);
    priv->model = get_tm_file_model((const char*)addr, priv->header);
    priv->subgraph = get_tm_file_subgraph((const char*)addr, priv->model);

    graph->serializer = s;
    graph->serializer_privacy = priv;
    graph->device_privacy = NULL;

    return load_graph(s, graph, priv);
}

/*
 * 卸载模型：关文件、释放模型内存（仅限本模块分配的）、释放 priv。
 * 加载失败时 priv 可能已被上一次调用清空，此时直接返回。
 */
static int unload_graph(struct serializer* s, struct graph* graph, void* s_priv, void* dev_priv)
{
    (void)s;
    (void)dev_priv;

    struct tm2_priv* priv = (struct tm2_priv*)s_priv;

    if (NULL == priv)
    {
        return 0;
    }

    if (priv->fd >= 0)
    {
        close(priv->fd);
        priv->fd = -1;
    }

    /* load_mem 路径的内存属于调用者，绝不能在这里释放 */
    if (priv->self_allocated && priv->base)
    {
        sys_free((void*)priv->base);
    }
    priv->base = NULL;

    graph->serializer = NULL;
    graph->serializer_privacy = NULL;

    sys_free(priv);

    return 0;
}

/* ------------------------------------------------------------------ */
/* 第六部分：序列化器接口实现与注册                                       */
/* ------------------------------------------------------------------ */

/* 接口层的简单转接：把 void* 参数转成具体函数指针类型 */
static int register_op_loader(struct serializer* s, int op_type, int op_ver, void* op_load_func, void* op_map_func,
                              void* ver_map_func)
{
    struct tm2_serializer* tm2_s = (struct tm2_serializer*)s;
    tm2_op_loader_t op_load = (tm2_op_loader_t)op_load_func;
    tm2_map_t op_map = (tm2_map_t)op_map_func;
    tm2_map_t ver_map = (tm2_map_t)ver_map_func;

    return register_tm2_op_loader(tm2_s, op_type, op_ver, op_load, op_map, ver_map);
}

static int unregister_op_loader(struct serializer* s, int op_type, int op_ver, void* op_load_func)
{
    struct tm2_serializer* tm2_s = (struct tm2_serializer*)s;
    tm2_op_loader_t op_load = (tm2_op_loader_t)op_load_func;

    return unregister_tm2_op_loader(tm2_s, op_type, op_ver, op_load);
}

static const char* get_name(struct serializer* s)
{
    (void)s;

    return tm2_name;
}

/* INPUTOP / CONST 这两类特殊节点的类型映射：TM2 编号 -> IR 编号 */
static int const_op_map(int op)
{
    (void)op;

    return OP_CONST;
}

static int input_op_map(int op)
{
    (void)op;

    return OP_INPUT;
}

/* 序列化器初始化：建解析规则表，并注册两个内置的“伪算子” */
static int init_tm2_serializer(struct serializer* s)
{
    struct tm2_serializer* tm2_s = (struct tm2_serializer*)s;

    tm2_s->loader_list = create_vector(sizeof(struct op_loader_entry), NULL);

    if (tm2_s->loader_list == NULL)
        return -1;

    s->register_op_loader(s, TM2_OPTYPE_INPUTOP, 1, NULL_TM2_OP_LOADER, input_op_map, NULL);
    s->register_op_loader(s, TM2_OPTYPE_CONST, 1, NULL_TM2_OP_LOADER, const_op_map, NULL);

    return 0;
}

static int release_tm2_serializer(struct serializer* s)
{
    struct tm2_serializer* tm2_s = (struct tm2_serializer*)s;

    s->unregister_op_loader(s, TM2_OPTYPE_INPUTOP, 1, NULL_TM2_OP_LOADER);
    s->unregister_op_loader(s, TM2_OPTYPE_CONST, 1, NULL_TM2_OP_LOADER);

    release_vector(tm2_s->loader_list);

    return 0;
}

/* 全局唯一一份序列化器实例 */
static struct tm2_serializer tm2_serializer = {
    .base = {
        .get_name = get_name,
        .load_model = load_model,
        .load_mem = load_mem,
        .unload_graph = unload_graph,
        .register_op_loader = register_op_loader,
        .unregister_op_loader = unregister_op_loader,
        .init = init_tm2_serializer,
        .release = release_tm2_serializer,
    },
    .loader_list = NULL,
};

int register_tm2_serializer()
{
    return register_serializer((struct serializer*)&tm2_serializer);
}

int unregister_tm2_serializer()
{
    return unregister_serializer((struct serializer*)&tm2_serializer);
}
