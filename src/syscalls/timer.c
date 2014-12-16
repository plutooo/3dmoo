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



u32 svcCreateTimer()
{
    u32 handleorigin = arm11_R(0);
    u32 type = arm11_R(1);
    u32 handle = handle_New(HANDLE_TYPE_TIMER, 0);

    handleinfo* h = handle_Get(handle);
    if(h == NULL) {
        DEBUG("failed to get newly created Event\n");
        PAUSE();
        return -1;
    }
    if(type > LOCK_TYPE_MAX) {
        DEBUG("unknown event type\n");
        PAUSE();
        return -1;
    }

    h->locked = true;
    h->locktype = type;
    arm11_SetR(1, handle); // handle_out

    DEBUG("handleoriginal=%x, resettype=%x.\n", handleorigin, type);
    PAUSE();
    return 0;
}

u32 svcSetTimer()
{
    u32 timer = arm11_R(0);
    u64 initial = arm11_R(1) | ((u64)arm11_R(2) << 32);
    u64 interval = arm11_R(3) | ((u64)arm11_R(4) << 32);
    DEBUG("SetTimer --todo-- %08x %08x %08x\n", timer, initial, interval);
    return 0;
}

u32 timer_WaitSynchronization(handleinfo* h, bool *locked)
{
    handleinfo* hi = handle_Get(h->misc[0]);
    if(hi == NULL) {
        *locked = 1;
        return 0;
    }
    if(hi->misc[0] & HANDLE_SERV_STAT_SYNCING) {
        mem_Write(hi->misc_ptr[0], arm11_ServiceBufferAddress() + 0x80, 0x80); //todo 
        *locked = 0;
        return 0;
    }
    else {
        *locked = 0;
        return 0;
    }
}
