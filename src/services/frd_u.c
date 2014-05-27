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
#include "handles.h"
#include "mem.h"
#include "arm11.h"

u32 lock_handle;
u32 event_handles[2];

u32 frd_u_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);

    // Read command-id.
    switch(cid) {
    case 0x00320042: //SetClientSdkVersion
        mem_Write32(0xFFFF0084, 0); //no error
        return 0;
        break;
    case 0x00080000: //GetMyPresence
        mem_Write32(0xFFFF0084, 0); //no error
        u32 wtfs = mem_Read32(0xFFFF0180);
        u32 pointer = mem_Read32(0xFFFF0184);
        DEBUG("GetMyPresence %08X,%08X\n", wtfs, pointer);
        return 0;
        break;
    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}
