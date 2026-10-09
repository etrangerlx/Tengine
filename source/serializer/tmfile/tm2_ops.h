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
 * All tm2 op loaders are listed here. When adding a new op loader under
 * source/serializer/tmfile/op, append one entry to the list below. The
 * entry name is the base name of the source file, whose
 * register_<name>_op() and unregister_<name>_op() will be invoked.
 */
#define TENGINE_TM2_OPS_LIST(X)                 \
    X(tm2_add_n)                                \
    X(tm2_argmax)                               \
    X(tm2_argmin)                               \
    X(tm2_batchnorm)                            \
    X(tm2_batchtospacend)                       \
    X(tm2_bias)                                 \
    X(tm2_broadmul)                             \
    X(tm2_cast)                                 \
    X(tm2_ceil)                                 \
    X(tm2_clip)                                 \
    X(tm2_comparison)                           \
    X(tm2_concat)                               \
    X(tm2_conv)                                 \
    X(tm2_crop)                                 \
    X(tm2_deconv)                               \
    X(tm2_depthtospace)                         \
    X(tm2_detection_output)                     \
    X(tm2_detection_postprocess)                \
    X(tm2_dropout)                              \
    X(tm2_eltwise)                              \
    X(tm2_elu)                                  \
    X(tm2_embedding)                            \
    X(tm2_expand)                               \
    X(tm2_expanddims)                           \
    X(tm2_fc)                                   \
    X(tm2_flatten)                              \
    X(tm2_gather)                               \
    X(tm2_gelu)                                 \
    X(tm2_gemm)                                 \
    X(tm2_generic)                              \
    X(tm2_gru)                                  \
    X(tm2_hardsigmoid)                          \
    X(tm2_hardswish)                            \
    X(tm2_instancenorm)                         \
    X(tm2_interp)                               \
    X(tm2_l2normalization)                      \
    X(tm2_l2pool)                               \
    X(tm2_layernorm)                            \
    X(tm2_logical)                              \
    X(tm2_logistic)                             \
    X(tm2_logsoftmax)                           \
    X(tm2_lrn)                                  \
    X(tm2_lstm)                                 \
    X(tm2_matmul)                               \
    X(tm2_maximum)                              \
    X(tm2_mean)                                 \
    X(tm2_mish)                                 \
    X(tm2_mvn)                                  \
    X(tm2_noop)                                 \
    X(tm2_normalize)                            \
    X(tm2_pad)                                  \
    X(tm2_permute)                              \
    X(tm2_pool)                                 \
    X(tm2_prelu)                                \
    X(tm2_priorbox)                             \
    X(tm2_psroipooling)                         \
    X(tm2_reciprocal)                           \
    X(tm2_reducel2)                             \
    X(tm2_reduction)                            \
    X(tm2_region)                               \
    X(tm2_relu)                                 \
    X(tm2_relu1)                                \
    X(tm2_relu6)                                \
    X(tm2_reorg)                                \
    X(tm2_reshape)                              \
    X(tm2_resize)                               \
    X(tm2_reverse)                              \
    X(tm2_rnn)                                  \
    X(tm2_roialign)                             \
    X(tm2_roipooling)                           \
    X(tm2_round)                                \
    X(tm2_rpn)                                  \
    X(tm2_scale)                                \
    X(tm2_scatter)                              \
    X(tm2_selu)                                 \
    X(tm2_shape)                                \
    X(tm2_shuffle_channel)                      \
    X(tm2_sigmoid)                              \
    X(tm2_slice)                                \
    X(tm2_softmax)                              \
    X(tm2_softplus)                             \
    X(tm2_spacetobatchnd)                       \
    X(tm2_spacetodepth)                         \
    X(tm2_sparsetodense)                        \
    X(tm2_spatialtransformer)                   \
    X(tm2_split)                                \
    X(tm2_squareddifference)                    \
    X(tm2_squeeze)                              \
    X(tm2_strided_slice)                        \
    X(tm2_swap_axis)                            \
    X(tm2_tanh)                                 \
    X(tm2_threshold)                            \
    X(tm2_tile)                                 \
    X(tm2_topkv2)                               \
    X(tm2_transpose)                            \
    X(tm2_unary)                                \
    X(tm2_unsqueeze)                            \
    X(tm2_upsample)                             \
    X(tm2_where)                                \
    X(tm2_zeroslike)

#define TENGINE_DECLARE_TM2_OP(name)            \
    extern int register_##name##_op(void);      \
    extern int unregister_##name##_op(void);

TENGINE_TM2_OPS_LIST(TENGINE_DECLARE_TM2_OP)
#undef TENGINE_DECLARE_TM2_OP

#define TENGINE_INVOKE_TM2_REG(name)                                                        \
    do                                                                                      \
    {                                                                                       \
        ret = register_##name##_op();                                                       \
        if (0 != ret)                                                                       \
        {                                                                                   \
            TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_" #name "_op", ret); \
        }                                                                                   \
    } while (0);

/*!
 * @brief Register all serializer.
 *
 * @return statue value, 0 success, other value failure.
 */
int register_all_tm2_ops(void)
{
    int ret = 0;

    TENGINE_TM2_OPS_LIST(TENGINE_INVOKE_TM2_REG)

    return 0;
}

#define TENGINE_INVOKE_TM2_UNREG(name)                                                          \
    do                                                                                          \
    {                                                                                           \
        ret = unregister_##name##_op();                                                         \
        if (0 != ret)                                                                           \
        {                                                                                       \
            TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_" #name "_op", ret);   \
        }                                                                                       \
    } while (0);

/*!
 * @brief unregister all serializer.
 *
 * @return statue value, 0 success, other value failure.
 */
int unregister_all_tm2_ops(void)
{
    int ret = 0;

    TENGINE_TM2_OPS_LIST(TENGINE_INVOKE_TM2_UNREG)

    return 0;
}
