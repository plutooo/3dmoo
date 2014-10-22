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

SERVICE_START(cfg_s);

SERVICE_CMD(0x00010082)   // GetConfigInfoBlk2
{
    u32 size = CMD(1);
    u32 id = CMD(2);
    u32 pointer = CMD(4);

    DEBUG("GetConfigInfoBlk2 %08x %08x %08x\n", size, id, pointer);

    /*u32 getconfigfromNAND(u32 size, u32 id, u32 pointer, u32 filter)
    switch (id) {
    case 0x00070001:// Sound Mode?
        mem_Write8(pointer, 0);
        break;
    case 0x000A0002: // Language
        mem_Write8(pointer, 1); // 1=English
        break;
    case 0x000B0000: // CountryInfo
        mem_Write8(pointer, 1); //?
        mem_Write8(pointer + 1, 1); //?
        mem_Write8(pointer + 2, 1); //?
        mem_Write8(pointer + 3, 78); // 78=Germany	Country code, same as DSi/Wii country codes. Value 0xff is invalid.
        break;
    default:
        ERROR("Unknown id %08x\n", id);
        break;
    }*/

    RESP(1, getconfigfromNAND(size,id,pointer,0x2)); // Result
    return 0;
}

SERVICE_CMD(0x00020000)   // SecureInfoGetRegion this is mirrored in all cfg change all if you change this
{
    DEBUG("SecureInfoGetRegion\n");

    RESP(1, 0); // Result
    RESP(2, 1); // 1 = USA 2=Europe
    return 0;
}

SERVICE_CMD(0x00050000)   // GetSystemModel
{
    DEBUG("GetSystemModel\n");

    RESP(1, 0); // Result
    RESP(2, 0); // 0=3DS, 1=3DSXL, 3=2DS
    return 0;
}

SERVICE_CMD(0x04050000)   // GetLocalFriendCodeSeed
{
    DEBUG("GetLocalFriendCodeSeed\n");
    RESP(1, 0); // Result
    RESP(2, 0x11223344); // LocalFriendCodeSeed lower word
    RESP(3, 0x55667788); // LocalFriendCodeSeed upper  word
    return 0;
}

SERVICE_CMD(0x04070000)   // SecureInfoGetByte101
{
    DEBUG("SecureInfoGetByte101\n");
    RESP(1, 0); // Result
    RESP(2, 0x11223344); // u8 value loaded from SecureInfo offset 0x101, from the already-loaded SecureInfo buffer.
    return 0;
}

SERVICE_END();
