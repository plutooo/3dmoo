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
#include "threads.h"


u32 svcCreateAddressArbiter()//(ref uint output)
{
    arm11_SetR(1, handle_New(HANDLE_TYPE_ARBITER, 0));
    return 0;
}

u32 svcArbitrateAddress()//(uint arbiter, uint addr, uint type, uint value)
{
    u32 arbiter   = arm11_R(0);
    u32 addr      = arm11_R(1);
    u32 type      = arm11_R(2);
    s32 value     = arm11_R(3);
    u32 val2      = arm11_R(4);
    u32 val3      = arm11_R(5);

    DEBUG("handle=%08x addr=%08x type=%08x value=%08x timeout=%08x,%08x\n",
          arbiter, addr, type, value, val2, val3);

    if(addr & 3) {
        ERROR("Unaligned addr.\n");
        return 0xD8E007F1;
    }

    handleinfo* hi = handle_Get(arbiter);
    if(hi == NULL || hi->type != HANDLE_TYPE_ARBITER) {
        ERROR("Invalid arbiter handle.\n");
        return 0xD8E007F7;
    }

    thread* p;
    s32 i;
    u32 val_addr;

    switch(type) {
    case 0: // Free
        // Negative value means resume all threads
        if(value < 0) {
            while(p = threads_ArbitrateHighestPrioThread(arbiter, addr))
                threads_ResumeArbitratedThread(p);
            return 0;
        }

        // Resume first N threads
        for(i=0; i<value; i++) {
            if(p = threads_ArbitrateHighestPrioThread(arbiter, addr))
                threads_ResumeArbitratedThread(p);
            else break;
        }

        return 0;

    case 1: // Acquire
        // If (signed cmp) value >= *addr, the thread is locked and added to wait-list.
        // Otherwise, thread keeps running and returns 0.
        if(value >= (s32)mem_Read32(addr))
            threads_SetCurrentThreadArbitrationSuspend(arbiter, addr);

        // XXX: Return error if read failed.
        return 0;

    case 3: // Acquire Timeout
        if(value >= (s32)mem_Read32(addr))
            threads_SetCurrentThreadArbitrationSuspend(arbiter, addr);

        // XXX: Add timeout based on val2,3
        return 0;

    case 2: // Acquire Decrement
        val_addr = mem_Read32(addr) - 1;
        mem_Write32(addr, val_addr);

        if(value >= (s32)val_addr)
            threads_SetCurrentThreadArbitrationSuspend(arbiter, addr);

        return 0;

    case 4: // Acquire Decrement Timeout
        val_addr = mem_Read32(addr) - 1;
        mem_Write32(addr, val_addr);

        if(value >= (s32)val_addr)
            threads_SetCurrentThreadArbitrationSuspend(arbiter, addr);

        // XXX: Add timeout based on val2,3
        return 0;

    default:
        ERROR("Invalid arbiter type %d\n", type);
        return 0xD8E093ED;
    }

    return 0;
}
