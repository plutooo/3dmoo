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

#include "util.h"
#include "arm11.h"
#include "mem.h"
#include "handles.h"


u32 svcCreateAddressArbiter()//(ref uint output)
{
    arm11_SetR(1, handle_New(HANDLE_TYPE_Arbiter, 0));
    return 0;
}

u32 svcArbitrateAddress()//(uint arbiter, uint addr, uint type, uint value)
{
    u32 arbiter   = arm11_R(0);
    u32 addr      = arm11_R(1);
    u32 type      = arm11_R(2);
    u32 low_limit = arm11_R(3);
    u32 val2      = arm11_R(4);
    u32 val3      = arm11_R(5);

    DEBUG("handle=%08x addr=%08x type=%08x low_limit=%08x timeout=%08x,%08x\n",
          arbiter, addr, type, low_limit, val2, val3);

    switch(type) {
    case 0:
        mem_Write32(addr, type); // this is wrong
        break;
    case 1: // USER
    case 2: // KERNEL
        mem_Write32(addr, type); // this is wrong
        break;
    case 3: // USER
    case 4: // KERNEL
        mem_Write32(addr, type); // this is wrong
        break;
    default:
        ERROR("invalid type %d\n", type);
        return -1;
    }

    return 0;
}
