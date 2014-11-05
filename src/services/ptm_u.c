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


SERVICE_START(ptm_u);

SERVICE_CMD(0x70000)   //GetBatteryLevel
{
    DEBUG("GetTotalStepCountGetBatteryLevel\n");
    RESP(1, 0); // Result
    RESP(2, 5); // battery level out of 5
    return 0;
}

SERVICE_CMD(0x80000)   //GetBatteryChargeState
{
    DEBUG("GetBatteryChargeState\n");
    RESP(1, 0); // Result
    RESP(2, 0); // 0 = not charging, 1 = charging
    return 0;
}

SERVICE_CMD(0xC0000)   //GetTotalStepCount
{
    DEBUG("GetTotalStepCount\n");
    RESP(1, 0); // Result
    RESP(2, 0); //this is a pc it is not taking steps
    return 0;
}
SERVICE_CMD(0xB00C2)   //GetStepHistory 
{
    u32 unk1 = CMD(1);
    u32 unk2 = CMD(2);
    u32 unk3 = CMD(3);
    u32 unk4 = CMD(4);
    u32 pointer = CMD(5);
    DEBUG("GetStepHistory %08x %08x %08x %08x %08x --todo--\n", unk1, unk2, unk3, unk4, pointer);
    RESP(1, 0); // Result
    //RESP(2, 0); //this is a pc it is not taking steps
    return 0;
}

SERVICE_END();
