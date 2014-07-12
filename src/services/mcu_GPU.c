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


u32 mcumutex = 0;

void mcu_GPU_init()
{
    mcumutex = handle_New(HANDLE_TYPE_MUTEX, 0);
    handleinfo* h = handle_Get(mcumutex);
    if (h == NULL) {
        DEBUG("failed to get newly created semaphore\n");
        PAUSE();
        return -1;
    }
    h->locked = false;
}

SERVICE_START(mcu_GPU);

SERVICE_CMD(0x000d0000) { //unk 0xD
    DEBUG("unk 0xD\n");

    RESP(3, mcumutex);

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x00090000) { //unk 0x9
    DEBUG("unk 0x9\n");

    RESP(2, 0); // return //u8

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x000a0000) { //unk 0xa
    DEBUG("unk 0xa\n");

    RESP(2, 0); // return //u8

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x00010000) { //unk 0x1
    DEBUG("unk 0x1\n");

    RESP(3, 0); // return //u8
    RESP(2, 0); // return //u8

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x00030000) { //unk 0x3
    DEBUG("unk 0x3\n");

    RESP(2, 0); // return //u8

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x00040040) { //unk 0x4
    DEBUG("unk 0x4 %02x\n",(u8)CMD(1));

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x000e0000) { //unk 0xE
    DEBUG("unk 0xE\n");

    RESP(2, 0x02000000); // return //u32

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x00020080) { //unk 0x2
    DEBUG("unk 0x2 %02x  %02x\n", (u8)CMD(1), (u8)CMD(2));

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x000B0040) { //unk 0xB
    DEBUG("unk 0xB %02x\n", (u8)CMD(1));

    RESP(1, 0); // Result
    return 0;
}


SERVICE_END();
