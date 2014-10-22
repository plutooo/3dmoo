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

u32 getconfigfromNAND(u32 size, u32 id, u32 pointer, u32 filter);

SERVICE_START(cfg_i);

SERVICE_CMD(0x00020000)   // SecureInfoGetRegion this is mirrored in all cfg change all if you change this
{
    DEBUG("SecureInfoGetRegion\n");

    RESP(1, 0); // Result
    RESP(2, 1); // 1 = USA 2=Europe
    return 0;
}

SERVICE_CMD(0x00010082)   // GetConfigInfoBlk8
{
    u32 size = CMD(1);
    u32 id = CMD(2);
    u32 pointer = CMD(4);

    DEBUG("GetConfigInfoBlk8 %08x %08x %08x\n", size, id, pointer);

    RESP(1, getconfigfromNAND(size, id, pointer, 0x8)); // Result
    return 0;
}
SERVICE_CMD(0x00040000)
{

    DEBUG("GetRegionCanadaUSA\n");

    RESP(2, 0); // return byte

    RESP(1, 1); // Result is Canada or USA
    return 0;
}
SERVICE_CMD(0x08010082)   // GetConfigInfoBlk4
{
    u32 size = CMD(1);
    u32 id = CMD(2);
    u32 pointer = CMD(4);

    DEBUG("GetConfigInfoBlk4 %08x %08x %08x\n", size, id, pointer);

    RESP(1, getconfigfromNAND(size, id, pointer, 0x4)); // Result
    return 0;
}
SERVICE_END();
