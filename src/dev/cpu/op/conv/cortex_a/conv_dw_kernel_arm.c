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
 * Copyright (c) 2020, OPEN AI LAB
 * Author: haoluo@openailab.com
 */
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "conv_dw_kernel_arm.h"
#include "conv_dw_k5_k7_kernel_arm.h"
#include "conv_dw_dilation_kernel_arm.h"

#ifdef __aarch64__
void dw_k3s2p0(float* data, int h, int w, float* kernel, float* output, float* bias, int out_w, int act);
void dw_k3s2p0p1(float* data, int h, int w, float* kernel, float* output, float* bias, int out_w, int act);
void dw_k3s1p1_a72(float* data, int h, int w, float* kernel, float* output, float* bias, int act);
void dw_k3s2p1_a72(float* data, int h, int w, float* kernel, float* output, float* bias, int act);

static void DirectConv(float* input_buf, int input_h, int input_w, float* output_buf, int output_h, int output_w,
                       float* weight_buf, int channel_num, int stride, float* bias, int* pads, int activation,
                       int num_thread, int cpu_affinity)
{
    int channel_size = input_h * input_w;
    int channel_size_out = output_h * output_w;
    int pad_h0 = pads[0];
    int pad_h1 = pads[2];

    if (stride == 1)
    {
#pragma omp parallel for num_threads(num_thread)
        for (int i = 0; i < channel_num; i++)
        {
            float* cur_input = input_buf + i * channel_size;
            float* cur_output = output_buf + i * channel_size_out;
            float* bias_tmp = NULL;
            if (bias)
                bias_tmp = bias + i;
            dw_k3s1p1_a72(cur_input, input_h, input_w, weight_buf + i * 9, cur_output, bias_tmp, activation);
        }
    }
    else if (pad_h0 == 0)
    {
#pragma omp parallel for num_threads(num_thread)
        for (int i = 0; i < channel_num; i++)
        {
            float* cur_input = input_buf + i * channel_size;
            float* cur_output = output_buf + i * channel_size_out;
            float* bias_tmp = NULL;
            if (bias)
                bias_tmp = bias + i;

            if (pad_h1 == 0)
                dw_k3s2p0(cur_input, input_h, input_w, weight_buf + i * 9, cur_output, bias_tmp, output_w, activation);
            else
                dw_k3s2p0p1(cur_input, input_h, input_w, weight_buf + i * 9, cur_output, bias_tmp, output_w,
                            activation);
        }
    }
    else
    {
#pragma omp parallel for num_threads(num_thread)
        for (int i = 0; i < channel_num; i++)
        {
            float* cur_input = input_buf + i * channel_size;
            float* cur_output = output_buf + i * channel_size_out;
            float* bias_tmp = NULL;
            if (bias)
                bias_tmp = bias + i;
            dw_k3s2p1_a72(cur_input, input_h, input_w, weight_buf + i * 9, cur_output, bias_tmp, activation);
        }
    }
}
#else
void dw_k3s2(float* input, float* kernel, float* output, int channel, int width, int height, float* bias, int pad0);
void dw_k3s2_relu_fused(float* input, float* kernel, float* output, int channel, int width, int height, float* bias,
                        int pad0);
void dw_k3s2_relu6_fused(float* input, float* kernel, float* output, int channel, int width, int height, float* bias,
                         int pad0);

void dw_k3s1p1(float* input, float* kernel, float* output, int channel, int width, int height, float* bias);
void dw_k3s1p1_relu_fused(float* input, float* kernel, float* output, int channel, int width, int height, float* bias);
void dw_k3s1p1_relu6_fused(float* input, float* kernel, float* output, int channel, int width, int height, float* bias);

static void DirectConv(float* input_buf, int input_h, int input_w, float* output_buf, int output_h, int output_w,
                       float* weight_buf, int channel_num, int stride, float* bias, int* pads, int activation,
                       int num_thread, int cpu_affinity)
{
    int pad_h0 = pads[0];

    if (stride == 1)
    {
#pragma omp parallel for num_threads(num_thread)
        for (int c = 0; c < channel_num; c++)
        {
            float* cur_input = input_buf + c * input_h * input_w;
            float* cur_output = output_buf + c * output_h * output_w;
            float* cur_weight = weight_buf + c * 9;
            float* cur_bias = bias ? bias + c : bias;
            if (activation >= 0)
            {
                if (activation == 0)
                    dw_k3s1p1_relu_fused(cur_input, cur_weight, cur_output, 1, input_w, input_h, cur_bias);
                else
                    dw_k3s1p1_relu6_fused(cur_input, cur_weight, cur_output, 1, input_w, input_h, cur_bias);
            }
            else
            {
                dw_k3s1p1(cur_input, cur_weight, cur_output, 1, input_w, input_h, cur_bias);
            }
        }
    }
    else if (stride == 2)
    {
#pragma omp parallel for num_threads(num_thread)
        for (int c = 0; c < channel_num; c++)
        {
            float* cur_input = input_buf + c * input_h * input_w;
            float* cur_output = output_buf + c * output_h * output_w;
            float* cur_weight = weight_buf + c * 9;
            float* cur_bias = bias ? bias + c : bias;
            if (activation >= 0)
            {
                if (activation == 0)
                    dw_k3s2_relu_fused(cur_input, cur_weight, cur_output, 1, input_w, input_h, cur_bias, pad_h0);
                else
                    dw_k3s2_relu6_fused(cur_input, cur_weight, cur_output, 1, input_w, input_h, cur_bias, pad_h0);
            }
            else
            {
                dw_k3s2(cur_input, cur_weight, cur_output, 1, input_w, input_h, cur_bias, pad_h0);
            }
        }
    }
}
#endif

int conv_dw_run(struct ir_tensor* input_tensor, struct ir_tensor* filter_tensor, struct ir_tensor* bias_tensor,
                struct ir_tensor* output_tensor, struct conv_param* param, int num_thread, int cpu_affinity)
{
    /* param */
    int pads[4];
    int group = param->group;
    int kernel_h = param->kernel_h;
    int kernel_w = param->kernel_w;
    int stride_h = param->stride_h;
    int stride_w = param->stride_w;
    int dilation_h = param->dilation_h;
    int dilation_w = param->dilation_w;
    pads[0] = param->pad_h0;
    pads[1] = param->pad_w0;
    pads[2] = param->pad_h1;
    pads[3] = param->pad_w1;

    if (stride_h != stride_w)
        return -1;

    int act_type = param->activation;
    int batch = input_tensor->dims[0];
    int in_c = input_tensor->dims[1] / group;
    int in_h = input_tensor->dims[2];
    int in_w = input_tensor->dims[3];
    int input_size = in_c * in_h * in_w;

    int out_c = output_tensor->dims[1] / group;
    int out_h = output_tensor->dims[2];
    int out_w = output_tensor->dims[3];
    int output_size = out_c * out_h * out_w;

    /* buffer addr */
    float* input_buf = ( float* )input_tensor->data;
    float* kernel_buf = ( float* )filter_tensor->data;
    float* output_buf = ( float* )output_tensor->data;
    float* biases_buf = NULL;
    if (bias_tensor)
        biases_buf = ( float* )bias_tensor->data;

    for (int n = 0; n < batch; n++)    // batch size
    {
        float* cur_input = input_buf + n * input_size * group;
        float* cur_output = output_buf + n * output_size * group;

        if (dilation_h != 1 && dilation_w != 1 && dilation_h == pads[0])
        {
            conv_dw_dilation_run(cur_input, kernel_buf, biases_buf, cur_output, in_h, in_w, group, pads[0], act_type,
                                 num_thread);
        }
        else if (kernel_h == 3 && kernel_w == 3)
        {
            DirectConv(cur_input, in_h, in_w, cur_output, out_h, out_w, kernel_buf, group, stride_h, biases_buf, pads,
                       act_type, num_thread, cpu_affinity);
        }
        else if (kernel_h == 5 && kernel_w == 5)
        {
            if (stride_h == 1)
                depthwise_conv_k5s1(cur_input, kernel_buf, biases_buf, cur_output, in_h, in_w, group, out_h, out_w,
                                    pads[0], pads[1], act_type, num_thread);
            else if (stride_h == 2)
                depthwise_conv_k5s2(cur_input, kernel_buf, biases_buf, cur_output, in_h, in_w, group, out_h, out_w,
                                    act_type, num_thread);
        }
        else if (kernel_h == 7 && kernel_w == 7)
        {
            if (stride_h == 1)
                depthwise_conv_k7s1(cur_input, kernel_buf, biases_buf, cur_output, in_h, in_w, group, out_h, out_w,
                                    act_type, num_thread);
            else if (stride_h == 2)
                depthwise_conv_k7s2(cur_input, kernel_buf, biases_buf, cur_output, in_h, in_w, group, out_h, out_w,
                                    act_type, num_thread);
        }
    }
    /*
    static char name[100] = "conv11.txt";
    if(name[4] == '1')
    {
        FILE* pf = fopen(name, "w");
        fprintf(pf,"k_size: %d, out: %d, %d, %d\n",kernel_size, out_c, out_h, out_w);

        for(int i = 0; i < out_c; i++)
            for(int j = 0; j < out_h; j++)
            for(int k = 0; k < out_w; k++)
            {
                if( k ==0)
                    fprintf(pf,"[%3d/%3d]: ", i, j);
                fprintf(pf,"%f ", output_buf[i * out_h * out_w + j * out_w + k]);
                if( k == out_w -1)
                    fprintf(pf,"\n");
            }
        fclose(pf);
    }
    name[4] = '2';
    */
    return 0;
}
