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

#include "service_macros.h"

u8* CSND_sharedmem = NULL;
u32 CSND_sharedmemsize = 0;
u32 CSND_mutex = 0;
u32 CSND_offset0;
u32 CSND_offset1;
u32 CSND_offset2;
u32 CSND_offset3;

SERVICE_START(csnd_SND);
SERVICE_CMD(0x00010140)   // Initialize
{
    CSND_sharedmemsize = CMD(1);
    CSND_offset0 = CMD(2);
    CSND_offset1 = CMD(3);
    CSND_offset2 = CMD(4);
    CSND_offset3 = CMD(5);
    DEBUG("Initialize %08X %08X %08X %08X %08X\n", CSND_sharedmemsize, CSND_offset0, CSND_offset1, CSND_offset2, CSND_offset3);
    if (CSND_sharedmem)free(CSND_sharedmem);
    CSND_sharedmem = (u8*)malloc(CSND_sharedmemsize);

    // Init some event handles.
    if (!CSND_mutex)
    {
        CSND_mutex = handle_New(HANDLE_TYPE_EVENT, HANDLE_SUBEVENT_CSNDEVENT);
        handleinfo* h = handle_Get(CSND_mutex);
        h->locked = true;
        h->locktype = LOCK_TYPE_ONESHOT;
    }

    RESP(1, 0); // Result
    RESP(2, 0x4000000); // fix
    RESP(3, CSND_mutex); // mutex
    RESP(4, handle_New(HANDLE_TYPE_SHAREDMEM, MEM_TYPE_CSND)); // Result

    return 0;
}
SERVICE_CMD(0x00050000)
{
    DEBUG("unknown 0x00050000\n");
    RESP(1, 0x0); // Result
    RESP(2, 0x0000000F); // bitmask of ?engines that are read to use we have 4?
    return 0;
}
SERVICE_END();
