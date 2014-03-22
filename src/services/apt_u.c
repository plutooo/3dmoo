/*
 * Copyright (C) 2014 - plutoo
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

#include "../util.h"
#include "../handles.h"
#include "../mem.h"
#include "../arm11/arm11.h"

u32 lock_handle;

u32 apt_u_SyncRequest() {
    u32 cid = mem_Read32(0xFFFF0080);
    u32 flags;

    // Read command-id.
    switch(cid) {
    case 0x10040:
	flags = mem_Read32(0xFFFF0084);
	DEBUG("apt_u::GetLockHandle, flags=%08x\n", flags);
	PAUSE();

	mem_Write32(0xFFFF0084, 0);

	lock_handle = handle_New(HANDLE_TYPE_UNK, 0);
	mem_Write32(0xFFFF0094, lock_handle);
	return 0;
    }

    ERROR("NOT IMPLEMENTED\n");
    arm11_Dump();
    PAUSE();
    return 0;
}
