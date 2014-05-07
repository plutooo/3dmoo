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

#include "util.h"
#include "handles.h"
#include "mem.h"
#include "arm11.h"

u32 event_handles;

#define CPUsvcbuffer 0xFFFF0000

ir_u_init()
{
    event_handles = handle_New(HANDLE_TYPE_EVENT, 0);
}

u32 ir_u_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);

    // Read command-id.
    switch(cid) {
    case 0x000c0000: //GetConnectionStatusEvent
        mem_Write32(CPUsvcbuffer + 0x8C, event_handles);
        mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
        return 0;
        break;
    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}
