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
 * All operator prototypes are listed here. When adding a new operator
 * prototype, append one entry to the list below. The entry name is the
 * base name of the source file under source/operator/prototype, whose
 * register_<name>_op() and unregister_<name>_op() will be invoked.
 */
#define TENGINE_OP_PROTOTYPE_LIST(X)            \
    X(absval)                                   \
    X(add_n)                                    \
    X(argmax)                                   \
    X(argmin)                                   \
    X(batchnorm)                                \
    X(batchtospacend)                           \
    X(bias)                                     \
    X(broadmul)                                 \
    X(cast)                                     \
    X(ceil)                                     \
    X(clip)                                     \
    X(comparison)                               \
    X(concat)                                   \
    X(const)                                    \
    X(convolution)                              \
    X(crop)                                     \
    X(deconvolution)                            \
    X(depthtospace)                             \
    X(detection_output)                         \
    X(detection_postprocess)                    \
    X(dropout)                                  \
    X(eltwise)                                  \
    X(elu)                                      \
    X(embedding)                                \
    X(expand)                                   \
    X(expanddims)                               \
    X(fc)                                       \
    X(flatten)                                  \
    X(gather)                                   \
    X(gelu)                                     \
    X(gemm)                                     \
    X(generic)                                  \
    X(gru)                                      \
    X(hardsigmoid)                              \
    X(hardswish)                                \
    X(input)                                    \
    X(instancenorm)                             \
    X(interp)                                   \
    X(l2normalization)                          \
    X(l2pool)                                   \
    X(layernorm)                                \
    X(logical)                                  \
    X(logsoftmax)                               \
    X(lrn)                                      \
    X(lstm)                                     \
    X(matmul)                                   \
    X(maximum)                                  \
    X(mean)                                     \
    X(minimum)                                  \
    X(mish)                                     \
    X(mvn)                                      \
    X(noop)                                     \
    X(normalize)                                \
    X(pad)                                      \
    X(permute)                                  \
    X(pooling)                                  \
    X(prelu)                                    \
    X(priorbox)                                 \
    X(psroipooling)                             \
    X(reciprocal)                               \
    X(reducel2)                                 \
    X(reduction)                                \
    X(region)                                   \
    X(relu)                                     \
    X(relu1)                                    \
    X(relu6)                                    \
    X(reorg)                                    \
    X(reshape)                                  \
    X(resize)                                   \
    X(reverse)                                  \
    X(rnn)                                      \
    X(roialign)                                 \
    X(roipooling)                               \
    X(round)                                    \
    X(rpn)                                      \
    X(scale)                                    \
    X(scatter)                                  \
    X(selu)                                     \
    X(shape)                                    \
    X(shuffle_channel)                          \
    X(sigmoid)                                  \
    X(slice)                                    \
    X(softmax)                                  \
    X(softplus)                                 \
    X(spacetobatchnd)                           \
    X(spacetodepth)                             \
    X(sparsetodense)                            \
    X(spatialtransformer)                       \
    X(split)                                    \
    X(squareddifference)                        \
    X(squeeze)                                  \
    X(strided_slice)                            \
    X(swap_axis)                                \
    X(tanh)                                     \
    X(threshold)                                \
    X(tile)                                     \
    X(topkv2)                                   \
    X(transpose)                                \
    X(unary)                                    \
    X(unsqueeze)                                \
    X(upsample)                                 \
    X(where)                                    \
    X(zeroslike)

#define TENGINE_DECLARE_OP_PROTO(name)          \
    extern int register_##name##_op(void);      \
    extern int unregister_##name##_op(void);

TENGINE_OP_PROTOTYPE_LIST(TENGINE_DECLARE_OP_PROTO)
#undef TENGINE_DECLARE_OP_PROTO

#define TENGINE_INVOKE_REG_OP(name)                                                         \
    do                                                                                      \
    {                                                                                       \
        ret = register_##name##_op();                                                       \
        if (0 != ret)                                                                       \
        {                                                                                   \
            TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_" #name "_op", ret); \
        }                                                                                   \
    } while (0);

/*!
 * @brief Register all operators.
 *
 * @return statue value, 0 success, other value failure.
 */
int register_all_op_prototype(void)
{
    int ret = 0;

    TENGINE_OP_PROTOTYPE_LIST(TENGINE_INVOKE_REG_OP)

    return 0;
}

#define TENGINE_INVOKE_UNREG_OP(name)                                                           \
    do                                                                                          \
    {                                                                                           \
        ret = unregister_##name##_op();                                                         \
        if (0 != ret)                                                                           \
        {                                                                                       \
            TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_" #name "_op", ret);   \
        }                                                                                       \
    } while (0);

/*!
 * @brief unregister all operators.
 *
 * @return statue value, 0 success, other value failure.
 */
int unregister_all_op_prototype(void)
{
    int ret = 0;

    TENGINE_OP_PROTOTYPE_LIST(TENGINE_INVOKE_UNREG_OP)

    return 0;
}
