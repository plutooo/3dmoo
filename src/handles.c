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

#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "handles.h"

#define MAX_NUM_HANDLES 0x1000
#define HANDLES_BASE    0xDEADBABE

static struct {
    bool taken;
    u32  type;
} handles[MAX_NUM_HANDLES];

static u32 handles_num;


u32 handle_New(u32 type) {
    if(handles_num == MAX_NUM_HANDLES) {
	ERROR("not enough handles..\n");
	arm11_Dump();
	exit(1);
    }

    handles[handles_num].taken = true;
    handles[handles_num].type  = type;

    return HANDLES_BASE + handles_num++;
}
