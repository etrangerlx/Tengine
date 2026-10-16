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
 * 日志模块。业务代码里大量使用的 TLOG_ERR / TLOG_INFO 等宏（见 log.h）
 * 最终都会调到这里的核心函数 do_log，它做三件事：
 *   1. 按日志级别过滤，低于设定级别的一律丢弃；
 *   2. 把 时间戳 / 级别名 / 前缀 / 用户消息 依次拼进缓冲区；
 *   3. 加锁后调用输出函数（默认输出到 stderr，可被替换）。
 *
 * Android 平台走 __android_log_vprint，由 logcat 收集。
 * ======================================================================
 */

#include "utility/log.h"

#include "defines.h"
#include "api/c_api.h"
#include "utility/lock.h"

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#ifdef ANDROID
#include <android/log.h>
#endif

static mutex_t log_locker;
static const char* map_table[] = {"EMERG", "ALERT", "CRIT", "ERROR", "WARN", "NOTICE", "INFO", "DEBUG"};

/* 加锁输出一条日志；兜底保证消息缓冲区以 '\0' 结尾 */
static void safety_log(struct logger* logger, char* message)
{
    if (0 != message[TE_MAX_LOG_LENGTH - 1])
    {
        message[TE_MAX_LOG_LENGTH - 1] = 0;
    }

    lock_mutex(&log_locker);
    logger->output_func(message);
    unlock_mutex(&log_locker);
}

/* 日志核心：过滤级别 -> 拼接缓冲区 -> 安全输出 */
static void do_log(struct logger* logger, enum log_level level, const char* fmt, ...)
{
    if (logger->log_level < level || level > LOG_DEBUG)
    {
        return;
    }
#ifdef ANDROID
    va_list _ap;
    va_start(_ap, fmt);

    /* 把 Tengine 日志级别映射到 Android logcat 的优先级 */
    switch (level)
    {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    {
        __android_log_vprint(ANDROID_LOG_FATAL, "Tengine", fmt, _ap);
        break;
    }
    case LOG_ERR:
    {
        __android_log_vprint(ANDROID_LOG_ERROR, "Tengine", fmt, _ap);
        break;
    }
    case LOG_WARNING:
    {
        __android_log_vprint(ANDROID_LOG_WARN, "Tengine", fmt, _ap);
        break;
    }
    case LOG_NOTICE:
    case LOG_INFO:
    {
        __android_log_vprint(ANDROID_LOG_INFO, "Tengine", fmt, _ap);
        break;
    }
    case LOG_DEBUG:
    {
        __android_log_vprint(ANDROID_LOG_DEBUG, "Tengine", fmt, _ap);
        break;
    }
    default:
    {
        __android_log_vprint(ANDROID_LOG_VERBOSE, "Tengine", fmt, _ap);
    }
    }
    va_end(_ap);

    return;
#else
    va_list ap;
    char msg[TE_MAX_LOG_LENGTH] = {0};
    int max_len = TE_MAX_LOG_LENGTH;
    int left = max_len;
    char* p = msg;
    int ret;

    /* 第 1 段：时间戳 */
    if (logger->option.print_time)
    {
        time_t t = time(NULL);
        ret = strftime(p, left, "%Y-%m-%d %X ", localtime(&t));
        left -= ret;
        p += ret;
    }

    /* 缓冲区用尽时提前收尾，输出已拼好的部分 */
    if (left <= 1)
    {
        return safety_log(logger, msg);
    }

    /* 第 2 段：级别名 */
    if (logger->option.print_level)
    {
        ret = snprintf(p, left, "%s ", map_table[level]);
        left -= ret;
        p += ret;
    }

    if (left <= 1)
    {
        return safety_log(logger, msg);
    }

    /* 第 3 段：模块前缀 */
    if (logger->option.print_prefix && logger->prefix)
    {
        ret = snprintf(p, left, "%s ", logger->prefix);
        left -= ret;
        p += ret;
    }

    if (left <= 1)
    {
        return safety_log(logger, msg);
    }

    /* 第 4 段：用户消息本体 */
    va_start(ap, fmt);
    vsnprintf(p, left, fmt, ap);
    va_end(ap);

    return safety_log(logger, msg);
#endif
}

static void change_log_level(struct logger* logger, int level)
{
    if (level < 0 || level > LOG_DEBUG)
    {
        return;
    }

    logger->log_level = level;
}

static void set_output_func(struct logger* logger, void (*func)(const char*))
{
    logger->output_func = func;
}

/* 默认输出函数：打到 stderr */
static void output_stderr(const char* msg)
{
    fprintf(stderr, "%s", msg);
}

/*
 * 取全局默认 logger（懒加载单例）：
 * 第一次调用时初始化互斥锁和 logger 的各个字段，之后直接返回。
 */
struct logger* get_default_logger(void)
{
    static int inited = 0;
    static struct logger default_logger;

    if (inited)
        return &default_logger;
    else
        init_mutex(&log_locker);

    lock_mutex(&log_locker);

    if (!inited)
    {
        inited = 1;

        default_logger.prefix = NULL;
        default_logger.log_level = TE_DEFAULT_LOG_LEVEL;

        default_logger.output_func = output_stderr;
        default_logger.log = do_log;
        default_logger.set_log_level = change_log_level;
        default_logger.set_output_func = set_output_func;

        /* 默认不打印时间戳 / 级别 / 前缀，保持输出干净 */
        default_logger.option.print_prefix = 0;
        default_logger.option.print_time = 0;
        default_logger.option.print_level = 0;
    }

    unlock_mutex(&log_locker);

    return &default_logger;
}
