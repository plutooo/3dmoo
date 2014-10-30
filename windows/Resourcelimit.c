/*
* Copyright (C) 2014 - ichfly
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

#include "util.h"
#include "arm11.h"
#include "mem.h"
#include "handles.h"
#include "threads.h"

s32 svcGetResourceLimitCurrentValues(u32 values_ptr,u32 handleResourceLimit,u32 names_ptr,u32 nameCount)
{
    for (int i = 0; i < nameCount; i++)
    {
        u32 temp = mem_Read32(names_ptr + i*4);
        switch (temp)
        {
        case 1: //GetUsingMemorySize
            mem_Write32(values_ptr + i * 8, 0x0);
            mem_Write32(values_ptr + i * 8 + 4, 0x0);
            break;
        default:
            DEBUG("unknown ResourceLimitCurrentValues %08x",temp);
            break;
        }
    }
    return 0;
}