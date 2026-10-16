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
 * Author: bhu@openailab.com
 */

/*
 * TM2_OPTYPE_BROADMUL 算子的 TM2 模型参数解析器。
 * 这一百来个解析器都是同一个三段式模板（统一说明见 tmfile/tm2_serializer.c
 * 文件头的新手导读），本文件的区别只在于：
 *   - tm2_load_broadmul：该算子没有需要解析的参数，加载时直接返回成功；
 *   - register/unregister：把解析函数挂到 "tengine" 序列化器的解析规则表上。
 */

#include "graph/tensor.h"
#include "graph/node.h"
#include "graph/graph.h"
#include "graph/subgraph.h"
#include "module/module.h"
#include "serializer/serializer.h"
#include "tmfile/tm2_serializer.h"
#include "device/device.h"
#include "utility/log.h"

static int broadmul_op_map(int op)
{
    return OP_BROADMUL;
}

static int tm2_load_broadmul(struct graph* ir_graph, struct node* ir_node, const TM2_Node* tm_node,
                             const TM2_Operator* tm_op)
{
    return 0;
}

int register_tm2_broadmul_op()
{
    struct serializer* tm2_s = find_serializer_via_name("tengine");

    if (tm2_s == NULL)
    {
        TLOG_ERR("tengine serializer has not been registered yet\n");
        return -1;
    }

    tm2_s->register_op_loader(tm2_s, TM2_OPTYPE_BROADMUL, 1, tm2_load_broadmul, broadmul_op_map, NULL);

    return 0;
}

int unregister_tm2_broadmul_op()
{
    struct serializer* tm2_s = find_serializer_via_name("tengine");

    if (tm2_s == NULL)
    {
        TLOG_ERR("tengine serializer has not been registered yet\n");
        return -1;
    }

    tm2_s->unregister_op_loader(tm2_s, TM2_OPTYPE_BROADMUL, 1, tm2_load_broadmul);

    return 0;
}
