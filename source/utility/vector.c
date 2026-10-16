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
 * vector 是一个 C 语言版动态数组（类似精简版 std::vector）：
 *   - 元素连续存放，按下标随机访问（get_vector_data）；
 *   - 空间不足时按 ahead_num（默认 8）的步长扩容；
 *   - 每个槽位前带一个 vector_entry_t 头，其中 valid 标记记录
 *     “该槽位是否装着需要归还的资源”——若创建时注册了 free_func，
 *     覆盖、删除元素或销毁 vector 时会先调用它释放旧元素。
 *
 * 内存布局：
 *   real_mem 是 malloc/realloc 返回的原始地址（可能不对齐）；
 *   mem 是向下对齐后的数据区起点，第 i 个槽位位于 mem + i * entry_size。
 *
 * 典型用法见 graph.c：create_vector(sizeof(struct subgraph*), NULL)。
 * ======================================================================
 */

#include "utility/vector.h"

#include "defines.h"
#include "utility/sys_port.h"
#include "utility/math.h"

#include <string.h>

/* ------------------------------------------------------------------ */
/* 内部辅助                                                             */
/* ------------------------------------------------------------------ */

/* 每个槽位的头部：valid 标记 + 元素数据（柔性数组） */
typedef struct vector_entry
{
    int valid;
    unsigned char data[];
} vector_entry_t;

/* 取第 idx 个槽位的地址 */
static inline vector_entry_t* get_vector_entry(vector_t* v, int idx)
{
    return (vector_entry_t*)((char*)v->mem + v->entry_size * idx);
}

/* 归还第 idx 个槽位持有的资源（若槽位有效且注册了释放回调），并清除标记 */
static inline void free_vector_data_resource(vector_t* v, int idx)
{
    vector_entry_t* e = get_vector_entry(v, idx);

    if (e->valid && v->free_func)
    {
        v->free_func(e->data);
    }

    e->valid = 0;
}

/* 删除非末尾元素：先归还资源，再把后面的元素整体前移一格 */
static inline void remove_vector_data_not_tail(vector_t* v, int idx)
{
    free_vector_data_resource(v, idx);

    void* start_data_ptr = (char*)v->mem + idx * v->entry_size;
    int remaining_elements_count = v->elem_num - 1 - idx;

    memmove(start_data_ptr, (char*)start_data_ptr + v->entry_size, (size_t)remaining_elements_count * v->entry_size);

    v->elem_num--;

    /* 元素前移后，原最后一个槽位空了出来，清除它的 valid 标记 */
    vector_entry_t* entry_ptr = get_vector_entry(v, v->elem_num);
    entry_ptr->valid = 0;
}

/* ------------------------------------------------------------------ */
/* 创建与释放                                                           */
/* ------------------------------------------------------------------ */

vector_t* create_vector(int elem_size, void (*free_data)(void*))
{
    vector_t* v = (vector_t*)sys_malloc(sizeof(vector_t));

    if (v == NULL)
    {
        return NULL;
    }

    /* 单个槽位大小 = 元素 + 头部，并对齐 */
    v->elem_num = 0;
    v->elem_size = elem_size;
    v->free_func = free_data;
    v->entry_size = align(elem_size + (int)sizeof(vector_entry_t), TE_VECTOR_ALIGN_SIZE);

    /* 初始容量 8，满了以后每次再扩 8 */
    v->ahead_num = 8;
    v->space_num = v->ahead_num;

    v->real_mem = sys_malloc(v->entry_size * v->space_num + TE_VECTOR_ALIGN_SIZE);
    if (NULL == v->real_mem)
    {
        sys_free(v);
        return NULL;
    }

    /* 多分配一个对齐单位的余量，保证对齐后仍有 space_num 个完整槽位 */
    v->mem = align_address(v->real_mem, TE_VECTOR_ALIGN_SIZE);

    for (int i = 0; i < v->space_num; i++)
    {
        vector_entry_t* e = get_vector_entry(v, i);
        e->valid = 0;
    }

    return v;
}

/* 释放整个 vector：先逐个归还元素资源，再释放内存和结构体 */
void release_vector(vector_t* v)
{
    for (int i = 0; i < v->elem_num; i++)
    {
        free_vector_data_resource(v, i);
    }

    sys_free(v->real_mem);
    sys_free(v);
}

/* ------------------------------------------------------------------ */
/* 容量与增删改查                                                       */
/* ------------------------------------------------------------------ */

int get_vector_num(vector_t* v)
{
    if (NULL != v)
    {
        return v->elem_num;
    }

    return 0;
}

/*
 * 把元素个数调整为 new_size，分三种情况：
 * 1. 变小：逐个删除多余的尾部元素；
 * 2. 容量足够：只改计数；
 * 3. 容量不够：realloc 内存，新增槽位的 valid 全部清零。
 * 返回 0 成功，-1 失败。
 */
int resize_vector(vector_t* v, int new_size)
{
    /* 缩容：删除超出 new_size 的尾部元素 */
    if (new_size < v->elem_num)
    {
        for (int i = v->elem_num - 1; i > new_size - 1; i--)
        {
            remove_vector_via_index(v, i);
        }

        return 0;
    }

    /* 容量足够：只需扩大计数 */
    if (new_size <= v->space_num)
    {
        v->elem_num = new_size;
        return 0;
    }

    void* new_mem = sys_realloc(v->real_mem, new_size * v->entry_size + TE_VECTOR_ALIGN_SIZE);

    if (new_mem == NULL)
    {
        return -1;
    }

    v->real_mem = new_mem;
    v->mem = align_address(v->real_mem, TE_VECTOR_ALIGN_SIZE);

    /* 新增的槽位都是空的 */
    for (int i = v->space_num; i < new_size; i++)
    {
        vector_entry_t* e = get_vector_entry(v, i);
        e->valid = 0;
    }

    v->space_num = new_size;

    return 0;
}

/* 追加一个元素，空间不足时自动扩容。返回 0 成功，-1 失败 */
int push_vector_data(vector_t* v, void* data)
{
    if (v->elem_num == v->space_num && resize_vector(v, v->elem_num + v->ahead_num) < 0)
    {
        return -1;
    }

    v->elem_num++;
    set_vector_data(v, v->elem_num - 1, data);

    return 0;
}

/* 覆盖第 idx 个元素：若旧元素持有资源，先归还。下标越界返回 -1 */
int set_vector_data(vector_t* v, int idx, void* data)
{
    if (idx >= v->elem_num)
        return -1;

    free_vector_data_resource(v, idx);

    vector_entry_t* e = get_vector_entry(v, idx);
    e->valid = 1;

    memcpy(e->data, data, v->elem_size);

    return 0;
}

/* 取第 index 个元素的地址；下标越界返回 NULL */
void* get_vector_data(vector_t* v, int index)
{
    if (index >= v->elem_num)
    {
        return NULL;
    }

    vector_entry_t* e = get_vector_entry(v, index);

    return e->data;
}

/* 按内容逐字节匹配元素，找到后删除。找不到返回 -1 */
int remove_vector_via_pointer(vector_t* v, void* data)
{
    const int count = v->elem_num;
    int index;

    for (index = 0; index < count; index++)
    {
        void* content = get_vector_data(v, index);

        if (memcmp(content, data, v->elem_size) == 0)
        {
            break;
        }
    }

    if (count == index)
    {
        return -1;
    }

    remove_vector_via_index(v, index);
    return 0;
}

/* 删除第 idx 个元素：末尾直接减计数，中间位置则把后续元素前移 */
void remove_vector_via_index(vector_t* v, int idx)
{
    // the last one
    if (idx == v->elem_num - 1)
    {
        free_vector_data_resource(v, idx);
        v->elem_num--;

        return;
    }

    remove_vector_data_not_tail(v, idx);
}
