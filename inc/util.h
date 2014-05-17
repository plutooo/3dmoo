/*
 * Copyright (C) 2014 - plutoo
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

#include <stdint.h>
#include <stdbool.h>

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

#if 1
#define DEBUG(...) do {                          \
    fprintf(stdout, "%s: ", __func__);       \
    fprintf(stdout, __VA_ARGS__);           \
} while (0);
#else
#define DEBUG(...)
#endif

#define ERROR(...) do { \
	fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
	fprintf(stderr, __VA_ARGS__);			\
    } while(0);

#if 0
#define PAUSE() fgetc(stdin);
#else
#define PAUSE()
#endif

#define ARRAY_SIZE(s) (sizeof(s)/sizeof((s)[0]))

#endif