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

u32 cfg_u_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);

    // Read command-id.
    switch(cid) {
    case 0x00020000: //Cfg:SecureInfoGetRegion
        mem_Write32(0xFFFF0084, 0); //no error
        mem_Write8(0xFFFF0088, 2); //europe
        return 1;
    case 0x00010082: //CfgS:GetConfigInfoBlk2
        mem_Write32(0xFFFF0084, 0); //no error
        u32 size = mem_Read32(CPUsvcbuffer + 0x84);
        u32 ID = mem_Read32(CPUsvcbuffer + 0x88);

        u32 pointer = mem_Read32(CPUsvcbuffer + 0x90);
        switch (ID) {
        case 0x000A0002: //Language
            mem_Write8(pointer, 1); //en
            break;
        default:
            DEBUG("GetConfigInfoBlk2 %08X %08X %08X", size, ID, pointer);
            break;
        }
        mem_Write32(0xFFFF0084, 0); //no error
        return 0;
    default:
        break;

    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}
