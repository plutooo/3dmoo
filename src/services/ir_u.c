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


u32 ir_event_handle;


void ir_u_init() {
    ir_event_handle = handle_New(HANDLE_TYPE_EVENT, 0);
}

SERVICE_START(ir_u);

SERVICE_CMD(0x000c0000) { //GetConnectionStatusEvent
    DEBUG("GetConnectionStatusEvent\n");

    RESP(1, 0); // Result
    RESP(1, ir_event_handle); // Event handle
    return 0;
}

SERVICE_CMD(0x00180182) { //InitializeIrnopShared
    u32 unk1 = CMD(1);
    u32 unk2 = CMD(2);
    u32 unk3 = CMD(3);
    u32 unk4 = CMD(4);
    u32 unk5 = CMD(5);
    u8  unk6 = CMD(6);
    u32 unk7 = CMD(7);
    DEBUG("InitializeIrnopShared %08x %08x %08x %08x %08x %02x %08x", unk1, unk2, unk3, unk4, unk5, unk6, unk7);

    RESP(1, 0); // Result
    return 0;
}

SERVICE_END();
