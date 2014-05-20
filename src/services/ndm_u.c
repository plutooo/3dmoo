/*
 * Copyright (C) 2014 - plutoo
 * Copyright (C) 2014 - ichfly
 * Copyright (C) 2014 - Normmatt
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


#define CPUsvcbuffer 0xFFFF0000

u32 ndm_u_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);

    // Read command-id.
    switch(cid) {
    case 0x00060040: //SuspendDaemons
        mem_Write32(CPUsvcbuffer + 0x84, 0); //worked
        return 0;
    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}
