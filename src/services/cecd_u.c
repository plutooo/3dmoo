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

#include "util.h"
#include "handles.h"
#include "mem.h"
#include "arm11.h"

#include "service_macros.h"

u32 lock_handle = 0;

SERVICE_START(cecd_u);

SERVICE_CMD(0x000F0000) //unknown
{
    if (!lock_handle) {
        lock_handle = handle_New(HANDLE_TYPE_EVENT, HANDLE_SUBEVENT_CECDEVENT);
        h = handle_Get(lock_handle);
        h->locked = true;
        h->locktype = LOCK_TYPE_ONESHOT;
    }
    DEBUG("unk F\n");
    RESP(3, lock_handle); // Handle
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x000D0082) //unknown
{
    u32 unk1 = CMD(1);
    u32 size = CMD(2);
    u32 pointer = CMD(4);
    DEBUG("unk D %08x %08x %08x\n", unk1,size,pointer);
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x00120104)   // ???
{
    DEBUG("???_120104 %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    RESP(1, 0);
    return 0;
}

SERVICE_END();
