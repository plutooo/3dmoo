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

s32 svcGetResourceLimitCurrentValues()
{
    u32 values_ptr = arm11_R(0);
    u32 handleResourceLimit = arm11_R(1);
    u32 names_ptr = arm11_R(2);
    u32 nameCount = arm11_R(3);
    for (u32 i = 0; i < nameCount; i++) {
        u32 temp = mem_Read32(names_ptr + i*4);
        switch (temp) {
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

s32 svcCreateResourceLimit()
{

    DEBUG("CreateResourceLimit\n");

    u32 handle = handle_New(HANDLE_TYPE_KResourceLimit, 0);

    arm11_SetR(1, handle);

    return 0;
}
s32 svcSetResourceLimitValues()
{
    u32 handle = arm11_R(0);
    u32 LimitableResources = arm11_R(1);
    u32 vals = arm11_R(2);
    u32 size =arm11_R(3);
    DEBUG("ResourceLimitCurrentValues %08x %08x %08x %08x\n", handle, LimitableResources, vals, size);
    
    for (int i = 0; i < size; i++)
    {
        printf("%08X -> %016X\n", mem_Read32(i * 4 + LimitableResources), (u64)(mem_Read32(i * 8 + vals) | (mem_Read32(i * 8 + vals + 4) << 32)));
    }
    return 0;
}
s32 svcGetSystemInfo() //this is part of the resource limmits
{
    u32 SystemInfoType = arm11_R(1);
    u32 param = arm11_R(2);
    DEBUG("svcGetSystemInfo %08x %08x\n", SystemInfoType, param);
    switch (SystemInfoType)
    {
        case 26:
            arm11_SetR(1, 5);
            arm11_SetR(2, 0);
            return 0;
        default:
            ERROR("unknown svcGetSystemInfo %08x %08x", SystemInfoType, param);
            arm11_SetR(1, 0);
            arm11_SetR(2, 0);
            return -1;

    }
}
s32 svcKernelSetState() //this is part of the resource limmits
{
    u32 type = arm11_R(0);
    u32 param0 = arm11_R(1);
    u32 param1 = arm11_R(2);
    u32 param2 = arm11_R(3);
    DEBUG("KernelSetState %08x %08x %08x %08x\n", type, param0, param1, param2);
    return 0;
}