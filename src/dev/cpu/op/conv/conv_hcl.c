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
#include "sys_port.h"
#include "module.h"
#include "tengine_errno.h"
#include "tengine_log.h"
#include "tengine_ir.h"
#include "../../cpu_node_ops.h"
#include "tengine_op.h"
#include "convolution_param.h"
#include "cortex_a/conv_kernel_arm.h"

static int prerun(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    struct ir_node* ir_node = exec_node->ir_node;
    struct ir_graph* ir_graph = ir_node->graph;
    struct ir_tensor* input_tensor;
    struct ir_tensor* filter_tensor;
    struct ir_tensor* output_tensor;

    struct conv_priv_info* conv_priv_info = ( struct conv_priv_info* )exec_node->ops_priv;

    if (conv_hcl_set_shared_mem && exec_node->shared_mem_size < exec_graph->shared_mem_size)
    {
        if (conv_hcl_set_shared_mem(conv_priv_info, exec_graph->shared_mem, exec_graph->shared_mem_size) < 0)
        {
            TLOG_ERR("hcl conv: set shared memory failed\n");
            set_tengine_errno(EFAULT);
            return -1;
        }
    }

    input_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[0]);
    filter_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[1]);
    output_tensor = get_ir_graph_tensor(ir_graph, ir_node->output_tensors[0]);

    struct conv_param* conv_param = ( struct conv_param* )ir_node->op.param_mem;

    // get cpu affinity
    conv_priv_info->cpu_type = exec_graph->cpu_affinity;

    /* prerun now */
    if (conv_hcl_prerun(input_tensor, filter_tensor, output_tensor, conv_priv_info, conv_param) < 0)
    {
        TLOG_ERR("hcl conv prerun failed\n");
        set_tengine_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int run(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    // fprintf(stderr, "conv hcl start\n");
    struct ir_node* ir_node = exec_node->ir_node;
    struct ir_graph* ir_graph = ir_node->graph;
    struct ir_tensor* input_tensor;
    struct ir_tensor* weight_tensor;
    struct ir_tensor* bias_tensor = NULL;
    struct ir_tensor* output_tensor = NULL;
    int num_thread = exec_graph->num_thread;
    int cpu_affinity = exec_graph->cpu_affinity;

    /* set the input data and shape again, in case of reshape or dynamic shape */
    input_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[0]);
    weight_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[1]);
    if (ir_node->input_num > 2)
        bias_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[2]);
    output_tensor = get_ir_graph_tensor(ir_graph, ir_node->output_tensors[0]);

    struct conv_param* conv_param = ( struct conv_param* )ir_node->op.param_mem;
    struct conv_priv_info* conv_priv_info = ( struct conv_priv_info* )exec_node->ops_priv;

    if (conv_hcl_run(input_tensor, weight_tensor, bias_tensor, output_tensor, conv_priv_info, conv_param, num_thread,
                     cpu_affinity) < 0)
    {
        TLOG_ERR("hcl conv run failed\n");
        set_tengine_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int reshape(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    return 0;
}

static int postrun(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    struct conv_priv_info* conv_priv_info = ( struct conv_priv_info* )exec_node->ops_priv;

    if (conv_hcl_postrun(conv_priv_info) < 0)
    {
        TLOG_ERR("hcl conv prerun failed\n");
        set_tengine_errno(EFAULT);
        return -1;
    }
    return 0;
}

static int init_node(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    struct ir_node* ir_node = exec_node->ir_node;
    struct ir_graph* ir_graph = ir_node->graph;
    struct ir_tensor* input_tensor;
    struct ir_tensor* output_tensor;

    input_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[0]);
    output_tensor = get_ir_graph_tensor(ir_graph, ir_node->output_tensors[0]);

    struct conv_param* conv_param = ( struct conv_param* )ir_node->op.param_mem;
    struct conv_priv_info* conv_priv_info = ( struct conv_priv_info* )sys_malloc(sizeof(struct conv_priv_info));
    if (conv_priv_info == NULL)
    {
        set_tengine_errno(ENOMEM);
        return -1;
    }

    memset(conv_priv_info, 0, sizeof(struct conv_priv_info));

    /* get shared memory size */
    exec_node->ops_priv = conv_priv_info;
    exec_node->shared_mem_size = conv_hcl_get_shared_mem_size(input_tensor, output_tensor, conv_param);

    return 0;
}

static int release_node(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    struct conv_priv_info* conv_priv_info = ( struct conv_priv_info* )exec_node->ops_priv;
    sys_free(conv_priv_info);
    exec_node->ops_priv = NULL;

    return 0;
}

static int score(struct node_ops* node_ops, struct exec_graph* exec_graph, struct ir_node* exec_node)
{
    return OPS_SCORE_BEST;
}

static struct node_ops hcl_node_ops = {.prerun = prerun,
                                       .run = run,
                                       .reshape = reshape,
                                       .postrun = postrun,
                                       .init_node = init_node,
                                       .release_node = release_node,
                                       .score = score};

static int reg_conv_hcl_ops(void* arg)
{
    return register_builtin_node_ops(OP_CONV, &hcl_node_ops);
}

static int unreg_conv_hcl_ops(void* arg)
{
    unregister_builtin_node_ops(OP_CONV, &hcl_node_ops);
    return 0;
}

AUTO_REGISTER_OPS(reg_conv_hcl_ops);
AUTO_UNREGISTER_OPS(unreg_conv_hcl_ops);
