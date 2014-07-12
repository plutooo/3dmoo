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

u8 HIDsharedbuffSPVR[0x2000];

#define CPUsvcbuffer 0xFFFF0000

u32 memhandel2;




void hid_spvr_init()
{
    memhandel2 = handle_New(HANDLE_TYPE_SHAREDMEM, MEM_TYPE_HID_SPVR_0);
}

SERVICE_START(hid_SPVR);

SERVICE_CMD(0xA0000) { //GetIPCHandles
    RESP(1, 0); // Result
    RESP(2, 0xDEADF00D); // Unused
    RESP(3, memhandel2);
    RESP(4, handle_New(HANDLE_TYPE_UNK, 0));
    RESP(5, handle_New(HANDLE_TYPE_UNK, 0));
    RESP(6, handle_New(HANDLE_TYPE_UNK, 0));
    RESP(7, handle_New(HANDLE_TYPE_UNK, 0));
    RESP(8, handle_New(HANDLE_TYPE_UNK, 0));
    return 0;
}

SERVICE_CMD(0x110000) { //EnableAccelerometer
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x130000) { //EnableGyroscopeLow
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x150000) {
    DEBUG("GetGyroscopeLowRawToDpsCoefficient (not working yet)\n");

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x160000) {
    DEBUG("GetGyroscopeLowCalibrateParam (not working yet)\n");

    RESP(1, 0); // Result
    return 0;
}

SERVICE_END();
