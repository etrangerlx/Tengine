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
 * All CPU operators are listed here. When adding a new CPU operator, append
 * one entry to the list below. The entry name is the base name of the source
 * file, whose register_<name>_op() and unregister_<name>_op() will be invoked.
 *
 * - TENGINE_CPU_REF_OP_LIST: reference implementations, always built.
 * - TENGINE_CPU_ARM_HCL_OP_LIST: ARM NEON kernels under op/<name>/cortex-a.
 * - TENGINE_CPU_CORTEX_M_OP_LIST: CMSIS kernels under op/<name>/cortex-m.
 * - TENGINE_CPU_X86_HCL_OP_LIST: x86 kernels under op/<name>/x86.
 * - TENGINE_CPU_MIPS_HCL_OP_LIST: MIPS kernels under op/<name>/mips.
 * - TENGINE_CPU_RISCV64_HCL_OP_LIST: RISC-V kernels under op/<name>/risc-v/lp64dv.
 */

#define TENGINE_CPU_REF_OP_LIST(X)              \
    X(absval_ref)                               \
    X(add_n_ref)                                \
    X(argmax_ref)                               \
    X(argmin_ref)                               \
    X(batchnorm_ref)                            \
    X(batchtospacend_ref)                       \
    X(bias_ref)                                 \
    X(broadmul_ref)                             \
    X(cast_ref)                                 \
    X(ceil_ref)                                 \
    X(clip_ref)                                 \
    X(comparison_ref)                           \
    X(concat_ref)                               \
    X(conv_ref)                                 \
    X(crop_ref)                                 \
    X(deconv_ref)                               \
    X(depthtospace_ref)                         \
    X(detection_output_ref)                     \
    X(detection_postprocess_ref)                \
    X(dropout_ref)                              \
    X(eltwise_ref)                              \
    X(elu_ref)                                  \
    X(embedding_ref)                            \
    X(expand_ref)                               \
    X(expanddims_ref)                           \
    X(fc_ref)                                   \
    X(flatten_ref)                              \
    X(gather_ref)                               \
    X(gelu_ref)                                 \
    X(gru_ref)                                  \
    X(hardsigmoid_ref)                          \
    X(hardswish_ref)                            \
    X(input_ref)                                \
    X(instancenorm_ref)                         \
    X(interp_ref)                               \
    X(l2normalization_ref)                      \
    X(l2pool_ref)                               \
    X(layernorm_ref)                            \
    X(logical_ref)                              \
    X(logistic_ref)                             \
    X(logsoftmax_ref)                           \
    X(lrn_ref)                                  \
    X(lstm_ref)                                 \
    X(matmul_ref)                               \
    X(maximum_ref)                              \
    X(mean_ref)                                 \
    X(minimum_ref)                              \
    X(mish_ref)                                 \
    X(mvn_ref)                                  \
    X(noop_ref)                                 \
    X(normalize_ref)                            \
    X(pad_ref)                                  \
    X(permute_ref)                              \
    X(pooling_ref)                              \
    X(prelu_ref)                                \
    X(priorbox_ref)                             \
    X(psroipooling_ref)                         \
    X(reciprocal_ref)                           \
    X(reducel2_ref)                             \
    X(reduction_ref)                            \
    X(region_ref)                               \
    X(relu1_ref)                                \
    X(relu6_ref)                                \
    X(relu_ref)                                 \
    X(reorg_ref)                                \
    X(reshape_ref)                              \
    X(resize_ref)                               \
    X(reverse_ref)                              \
    X(rnn_ref)                                  \
    X(roialign_ref)                             \
    X(roipooling_ref)                           \
    X(round_ref)                                \
    X(rpn_ref)                                  \
    X(scale_ref)                                \
    X(scatter_ref)                              \
    X(selu_ref)                                 \
    X(shape_ref)                                \
    X(shuffle_channel_ref)                      \
    X(sigmoid_ref)                              \
    X(slice_ref)                                \
    X(softmax_ref)                              \
    X(softplus_ref)                             \
    X(spacetobatchnd_ref)                       \
    X(spacetodepth_ref)                         \
    X(sparsetodense_ref)                        \
    X(spatialtransformer_ref)                   \
    X(split_ref)                                \
    X(squareddifference_ref)                    \
    X(squeeze_ref)                              \
    X(strided_slice_ref)                        \
    X(swap_axis_ref)                            \
    X(tanh_ref)                                 \
    X(threshold_ref)                            \
    X(tile_ref)                                 \
    X(topkv2_ref)                               \
    X(transpose_ref)                            \
    X(unary_ref)                                \
    X(unsqueeze_ref)                            \
    X(upsample_ref)                             \
    X(where_ref)                                \
    X(zeroslike_ref)

/* application profile ARM, exclude M profile which is handled by CMSIS below */
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#if !defined(__ARM_ARCH_6M__) && !defined(__ARM_ARCH_7M__) && !defined(__ARM_ARCH_7EM__) && \
    !defined(__ARM_ARCH_8M_BASE__) && !defined(__ARM_ARCH_8M_MAIN__) && !defined(__ARM_ARCH_8_1M_MAIN__)
#define TENGINE_CPU_ARM_HCL_OP_LIST(X)          \
    X(absval_hcl_arm)                           \
    X(batchnorm_hcl_arm)                        \
    X(conv_dw_hcl_arm)                          \
    X(conv_hcl_arm)                             \
    X(eltwise_hcl_arm)                          \
    X(elu_hcl_arm)                              \
    X(fc_hcl_arm)                               \
    X(interp_hcl_arm)                           \
    X(lrn_hcl_arm)                              \
    X(mish_hcl_arm)                             \
    X(pooling_hcl_arm)                          \
    X(relu_hcl_arm)                             \
    X(selu_hcl_arm)                             \
    X(sigmoid_hcl_arm)                          \
    X(softmax_hcl_arm)                          \
    X(tanh_hcl_arm)
#endif
#endif

#if defined(__ARM_ARCH_6M__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__) || \
    defined(__ARM_ARCH_8M_BASE__) || defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8_1M_MAIN__)
#define TENGINE_CPU_CORTEX_M_OP_LIST(X)         \
    X(conv_cmsis)                               \
    X(fc_cmsis)                                 \
    X(pooling_cmsis)                            \
    X(relu_cmsis)                               \
    X(softmax_cmsis)
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64)
#define TENGINE_CPU_X86_HCL_OP_LIST(X)          \
    X(conv_dw_hcl_x86)                          \
    X(conv_hcl_x86)                             \
    X(fc_hcl_x86)
#endif

#if defined(__mips__)
#define TENGINE_CPU_MIPS_HCL_OP_LIST(X)         \
    X(conv_dw_hcl_mips)                         \
    X(conv_hcl_mips)
#endif

#if defined(__riscv) && defined(__riscv_xlen) && (64 == __riscv_xlen)
#define TENGINE_CPU_RISCV64_HCL_OP_LIST(X)      \
    X(conv_dw_hcl_rv64)                         \
    X(conv_hcl_rv64)
#endif

#define TENGINE_DECLARE_CPU_OP(name)            \
    extern int register_##name##_op(void);      \
    extern int unregister_##name##_op(void);

TENGINE_CPU_REF_OP_LIST(TENGINE_DECLARE_CPU_OP)
#ifdef TENGINE_CPU_ARM_HCL_OP_LIST
TENGINE_CPU_ARM_HCL_OP_LIST(TENGINE_DECLARE_CPU_OP)
#endif
#ifdef TENGINE_CPU_CORTEX_M_OP_LIST
TENGINE_CPU_CORTEX_M_OP_LIST(TENGINE_DECLARE_CPU_OP)
#endif
#ifdef TENGINE_CPU_X86_HCL_OP_LIST
TENGINE_CPU_X86_HCL_OP_LIST(TENGINE_DECLARE_CPU_OP)
#endif
#ifdef TENGINE_CPU_MIPS_HCL_OP_LIST
TENGINE_CPU_MIPS_HCL_OP_LIST(TENGINE_DECLARE_CPU_OP)
#endif
#ifdef TENGINE_CPU_RISCV64_HCL_OP_LIST
TENGINE_CPU_RISCV64_HCL_OP_LIST(TENGINE_DECLARE_CPU_OP)
#endif
#undef TENGINE_DECLARE_CPU_OP

#define TENGINE_INVOKE_CPU_REG(name)                                                        \
    do                                                                                      \
    {                                                                                       \
        ret = register_##name##_op();                                                       \
        if (0 != ret)                                                                       \
        {                                                                                   \
            TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "register_" #name "_op", ret); \
        }                                                                                   \
    } while (0);

#define TENGINE_INVOKE_CPU_UNREG(name)                                                          \
    do                                                                                          \
    {                                                                                           \
        ret = unregister_##name##_op();                                                         \
        if (0 != ret)                                                                           \
        {                                                                                       \
            TLOG_ERR("Tengine FATAL: Call %s failed(%d).\n", "unregister_" #name "_op", ret);   \
        }                                                                                       \
    } while (0);

/*!
 * @brief Register all cpu operators.
 *
 * @return statue value, 0 success, other value failure.
 */
int register_all_cpu_ops(void)
{
    int ret = 0;

    TENGINE_CPU_REF_OP_LIST(TENGINE_INVOKE_CPU_REG)
#ifdef TENGINE_CPU_ARM_HCL_OP_LIST
    TENGINE_CPU_ARM_HCL_OP_LIST(TENGINE_INVOKE_CPU_REG)
#endif
#ifdef TENGINE_CPU_CORTEX_M_OP_LIST
    TENGINE_CPU_CORTEX_M_OP_LIST(TENGINE_INVOKE_CPU_REG)
#endif
#ifdef TENGINE_CPU_X86_HCL_OP_LIST
    TENGINE_CPU_X86_HCL_OP_LIST(TENGINE_INVOKE_CPU_REG)
#endif
#ifdef TENGINE_CPU_MIPS_HCL_OP_LIST
    TENGINE_CPU_MIPS_HCL_OP_LIST(TENGINE_INVOKE_CPU_REG)
#endif
#ifdef TENGINE_CPU_RISCV64_HCL_OP_LIST
    TENGINE_CPU_RISCV64_HCL_OP_LIST(TENGINE_INVOKE_CPU_REG)
#endif

    return ret;
}

/*!
 * @brief unregister all cpu operators.
 *
 * @return statue value, 0 success, other value failure.
 */
int unregister_all_cpu_ops(void)
{
    int ret = 0;

    TENGINE_CPU_REF_OP_LIST(TENGINE_INVOKE_CPU_UNREG)
#ifdef TENGINE_CPU_ARM_HCL_OP_LIST
    TENGINE_CPU_ARM_HCL_OP_LIST(TENGINE_INVOKE_CPU_UNREG)
#endif
#ifdef TENGINE_CPU_CORTEX_M_OP_LIST
    TENGINE_CPU_CORTEX_M_OP_LIST(TENGINE_INVOKE_CPU_UNREG)
#endif
#ifdef TENGINE_CPU_X86_HCL_OP_LIST
    TENGINE_CPU_X86_HCL_OP_LIST(TENGINE_INVOKE_CPU_UNREG)
#endif
#ifdef TENGINE_CPU_MIPS_HCL_OP_LIST
    TENGINE_CPU_MIPS_HCL_OP_LIST(TENGINE_INVOKE_CPU_UNREG)
#endif
#ifdef TENGINE_CPU_RISCV64_HCL_OP_LIST
    TENGINE_CPU_RISCV64_HCL_OP_LIST(TENGINE_INVOKE_CPU_UNREG)
#endif

    return 0;
}
