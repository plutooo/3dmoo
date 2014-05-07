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

u32 lock_handle;
u32 event_handles[2];

#define CPUsvcbuffer 0xFFFF0000

u32 dsp_dsp_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);

    // Read command-id.
    switch(cid) {
    case 0x001100c2: //LoadComponent
    {
                         u32 size = mem_Read32(CPUsvcbuffer + 0x84);
                         u32 unk1 = mem_Read32(CPUsvcbuffer + 0x88);
                         u32 unk2 = mem_Read32(CPUsvcbuffer + 0x8C);
                         u32 buffer = mem_Read32(CPUsvcbuffer + 0x94);
                         DEBUG("LoadComponent %08X %08X %08X %08X", size, buffer, unk1, unk2);
                         mem_Write32(CPUsvcbuffer + 0x84, 0x0); //error out
                         mem_Write32(CPUsvcbuffer + 0x88, 0x1); //loaded
                         return 0;
                         break;
    }
    case 0x00150082: //RegisterInterruptEvents
    {
                         u32 param0 = mem_Read32(CPUsvcbuffer + 0x84);
                         u32 param1 = mem_Read32(CPUsvcbuffer + 0x88);
                         u32 eventhandle = mem_Read32(CPUsvcbuffer + 0x94);
                         DEBUG("RegisterInterruptEvents %08X %08X %08X", param0, param1, eventhandle);
                         mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
                         return 0;
                         break;
    }
    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}
