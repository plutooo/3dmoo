/*
* Copyright (C) 2014 - plutoo
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "arm11.h"
#include "mem.h"
#include "handles.h"

u32 svcCreateAddressArbiter()//(uint arbiter, ref uint output)
{
    DEBUG("Create AddressArbiter %08X \r\n", arm11_R(0));
    u32* listpointer = malloc(8);
    *listpointer = 0;
    *(listpointer + 1) = 0;
    u32 newhandelu = handle_New(HANDLE_TYPE_Arbiter, listpointer);
    arm11_SetR(1, newhandelu);
    arm11_Dump();
    return 0;
}
findinarb(u32* list,u32 unk)
{
    u32 curr;
    do
    {
        curr = *list++;
    } while (curr != 0);
    return curr;
}
/*
type 0 is set
type 1 is add
type 2 if *addr > val sub 1
type 3 exist
type 4 exist
*/

u32 svcArbitrateAddress()//(uint arbiter, uint addr, uint type, uint value)
{
    u32 arbiter = arm11_R(0);
    u32 addr = arm11_R(1);
    u32 type = arm11_R(2);
    u32 val = arm11_R(3);
    u32 val2 = arm11_R(4);
    u32 val3 = arm11_R(5);
    handleinfo* hi = handle_Get(arbiter);
    u32 size = *(u32*)hi->subtype * 4;
    if (findinarb(hi->subtype, val) == 0) //if not in list add
    {
        hi->subtype = realloc(hi->subtype, size + 0x4 * 3);
        *(u32*)((u32)hi->subtype + size + 4) = addr;
        *(u32*)((u32)hi->subtype + size + 0x4 + 4) = 0;
        (*(u32*)hi->subtype)++;
        size += 4;
    }
    for (int i = 4; i < size + 0x4; i += 4)
    {
        switch (type)
        {
        case 0: //set
            mem_Write32(*(u32*)((u32)hi->subtype + i), val);
            break;
        case 1:
            mem_Write32(*(u32*)((u32)hi->subtype + i), mem_Read32(*(u32*)((u32)hi->subtype + i) + val));
            break;
        default:
            DEBUG("unknown ArbitrateAddress type %08X",type);
            return 0;
        }
    }
    DEBUG("ArbitrateAddress %08X %08X %08X %08X %08X %08X current %08X", arbiter, addr, type, val, val2, val3, mem_Read32(addr));
    //arm11_Dump();
    //mem_Write32(addr, type); //stfu
    return 0;
}