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
    u32 newhandelu = handle_New(HANDLE_TYPE_Arbiter, 0);
    arm11_SetR(1, newhandelu);
    return 0;
}
u32 svcArbitrateAddress()//(uint arbiter, uint addr, uint type, uint value)
{
    u32 arbiter = arm11_R(0);
    u32 addr = arm11_R(1);
    u32 type = arm11_R(2);
    u32 val = arm11_R(3);
    u32 val2 = arm11_R(4);
    u32 val3 = arm11_R(5);

    DEBUG("ArbitrateAddress %08X %08X %08X %08X %08X %08X ", arbiter, addr, type, val, val2, val3);

    mem_Write32(addr, type); //stfu
    return 0;
}