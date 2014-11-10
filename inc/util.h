/*
 * Copyright (C) 2014 - plutoo
 * Copyright (C) 2014 - ichfly
 * Copyright (C) 2014 - Normmatt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#ifndef WINVER
#define WINVER 0x0500   // default to Windows Version 4.0
#endif

#ifdef _WIN32
#include <Windows.h>
#undef ERROR
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifndef u8
typedef uint8_t u8;
#endif
#ifndef s8
typedef int8_t s8;
#endif
#ifndef u16
typedef uint16_t u16;
#endif
#ifndef s16
typedef int16_t s16;
#endif
#ifndef u32
typedef uint32_t u32;
#endif
#ifndef s32
typedef int32_t s32;
#endif
#ifndef u64
typedef uint64_t u64;
#endif
#ifndef s64
typedef int64_t s64;
#endif

// Fix for Microshit compiler
#ifndef __func__
#define __func__ __FUNCTION__
#endif

static int color_red()
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                               &csbiInfo);

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_RED | FOREGROUND_INTENSITY);

    return csbiInfo.wAttributes;
#else
    fprintf(stdout, "\033[0;31m");
    return 0;
#endif
}

static int color_green()
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                               &csbiInfo);

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    return csbiInfo.wAttributes;
#else
    fprintf(stdout, "\033[0;32m");
    return 0;
#endif
}

static int color_teal()
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                               &csbiInfo);

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                            FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    return csbiInfo.wAttributes;
#else
    fprintf(stdout, "\033[0;36m");
    return 0;
#endif
}

static void color_restore(int old)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), old);
#else
    fprintf(stdout, "\033[0m");
#endif
}

#if 1
#define DEBUG(...) do {                                 \
        int old = color_green();                        \
        fprintf(stdout, "%s: ", __func__);              \
        color_restore(old);                             \
        fprintf(stdout, __VA_ARGS__);                   \
} while (0);
#else
#define DEBUG(...)
#endif

#if 1
#define GPUDEBUG(...) do {                                 \
    DEBUG(__VA_ARGS__);                   \
} while (0);
#else
#define GPUDEBUG(...)
#endif

#if 0
#define THREADDEBUG(...) do {                                 \
    DEBUG(__VA_ARGS__);                   \
} while (0);
#else
#define THREADDEBUG(...)
#endif

#if 0
#define LOG(...) do {                                 \
    int old = color_teal();                           \
    fprintf(stdout, __VA_ARGS__);                     \
    color_restore(old);                               \
} while(0);
#else
#define LOG(...)
#endif

#define ERROR(...) do {                                 \
        int old = color_red();                          \
        fprintf(stdout, "%s: ", __func__);              \
        color_restore(old);                             \
        fprintf(stdout, __VA_ARGS__);       			\
    } while(0);

#if 0
#define PAUSE() fgetc(stdin);
#else
#define PAUSE()
#endif

#define ARRAY_SIZE(s) (sizeof(s)/sizeof((s)[0]))

#ifdef _WIN32
#define snprintf sprintf_s
#endif

float f24to32(u32 data, void *dataa);

#endif
