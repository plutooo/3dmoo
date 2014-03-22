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
u32 event_handles[2];

u32 apt_u_SyncRequest() {
    u32 cid = mem_Read32(0xFFFF0080);

    // Read command-id.
    switch(cid) {
    case 0x10040:
	(void) 0;

	u32 flags = mem_Read32(0xFFFF0084);
	DEBUG("apt_u_GetLockHandle, flags=%08x\n", flags);
	PAUSE();

	mem_Write32(0xFFFF0084, 0); // result

	// XXX: fix handle
	lock_handle = handle_New(HANDLE_TYPE_UNK, 0);
	mem_Write32(0xFFFF0094, lock_handle); // lock_handle
	return 0;

    case 0x20080:
	(void) 0;

	u32 app_id = mem_Read32(0xFFFF0084);
	DEBUG("apt_u_RegisterApp, app_id=%08x\n", app_id);
	PAUSE();

	// XXX: fix handles
	event_handles[0] = handle_New(HANDLE_TYPE_UNK, 0);
	event_handles[1] = handle_New(HANDLE_TYPE_UNK, 0);

	mem_Write32(0xFFFF008C, event_handles[0]); // some event handles
	mem_Write32(0xFFFF0090, event_handles[1]); // some event handles

	mem_Write32(0xFFFF0084, 0); // result
	return 0;

    case 0x30040:
	(void) 0;

	u32 unk = mem_Read32(0xFFFF0084);
	DEBUG("apt_u_Enable, unk=%08x\n", unk);
	PAUSE();

	mem_Write32(0xFFFF0084, 0);
	return 0;

    case 0x430040:
	(void) 0;

	app_id = mem_Read32(0xFFFF0084);
	DEBUG("apt_u_NotifyToWait, app_id=%08x\n", app_id);
	PAUSE();

	mem_Write32(0xFFFF0084, 0);
	return 0;
    }

    ERROR("NOT IMPLEMENTED\n");
    arm11_Dump();
    PAUSE();
    return 0;
}
