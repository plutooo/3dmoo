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

#ifdef _WIN32
static int _wincolors[] = {
    0,                                                          // black
    FOREGROUND_RED   | FOREGROUND_INTENSITY,                    // red
    FOREGROUND_GREEN | FOREGROUND_INTENSITY,                    // green
    FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY,   // yellow
    FOREGROUND_BLUE  | FOREGROUND_INTENSITY,                    // blue
    FOREGROUND_BLUE  | FOREGROUND_RED | FOREGROUND_INTENSITY,   // magenta
    FOREGROUND_BLUE  | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // cyan
    FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY, // white
};
#endif

static set_color(int color)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),
        &csbiInfo);

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
        _wincolors[color]);

    return csbiInfo.wAttributes;
#else
    fprintf(stdout, "\033[0;%dm", color + 30);
    return 0;
#endif
}

static int color_red() { return set_color(1); }
static int color_green() { return set_color(2); }
static int color_yellow() { return set_color(3); }
static int color_teal()  { return set_color(6); }

static void color_restore(int old)
{
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), old);
#else
    fprintf(stdout, "\033[0m");
#endif
}

#define MESSAGE(color,...) do {         \
        int old = set_color(color);     \
        fprintf(stdout, __VA_ARGS__);   \
        color_restore(old);             \
} while (0)

#define MSG(...) MESSAGE(3,__VA_ARGS__)

#ifndef DISABLE_DEBUG
#define DEBUG(...) do {                                 \
        int old = color_green();                        \
        fprintf(stdout, "%s: ", __func__);              \
        color_restore(old);                             \
        fprintf(stdout, __VA_ARGS__);                   \
} while (0)
#else
#define DEBUG(...) ((void)0)
#endif

#ifndef DISABLE_DEBUG
#define GPUDEBUG(...) do {                                 \
    DEBUG(__VA_ARGS__);                   \
} while (0)
#else
#define GPUDEBUG(...) ((void)0)
#endif

#ifndef DISABLE_DEBUG
#define THREADDEBUG(...) do {                                 \
    DEBUG(__VA_ARGS__);                   \
} while (0)
#else
#define THREADDEBUG(...) ((void)0)
#endif

#ifndef DISABLE_DEBUG
#define LOG(...) do {                                 \
    int old = color_teal();                           \
    fprintf(stdout, __VA_ARGS__);                     \
    color_restore(old);                               \
} while(0)
#else
#define LOG(...) ((void)0)
#endif

#define ERROR(...) do {                                 \
        int old = color_red();                          \
        fprintf(stdout, "%s: ", __func__);              \
        color_restore(old);                             \
        fprintf(stdout, __VA_ARGS__);       			\
    } while(0)

#if 0
#define PAUSE() fgetc(stdin);
#else
#define PAUSE() ((void)0)
#endif

#define ARRAY_SIZE(s) (sizeof(s)/sizeof((s)[0]))

#ifdef _WIN32
#define snprintf sprintf_s
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#else
#define fseek64 fseeko
#define ftell64 ftello
#endif


float f24to32(u32 data, void *dataa);

#endif
