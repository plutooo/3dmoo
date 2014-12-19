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

#include "util.h"
#include "handles.h"
#include "mem.h"
#include "arm11.h"

#include "config.h"

#include "service_macros.h"

#include "config.h"

typedef struct {
    u8 BlkID[4];
    u8 offset[4];
    u8 size[2];
    u8 flags[2];
} config_block;

u32 Read32(uint8_t p[4])
{
    u32 temp = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
    return temp;
}

static const u8 stereo_camera_settings[32] =
{
    0x00, 0x00, 0x78, 0x42, 0x00, 0x80, 0x90, 0x43, 0x9A, 0x99, 0x99, 0x42, 0xEC, 0x51, 0x38, 0x42,
    0x00, 0x00, 0x20, 0x41, 0x00, 0x00, 0xA0, 0x40, 0xEC, 0x51, 0x5E, 0x42, 0x5C, 0x8F, 0xAC, 0x41,
};

u32 getconfigfromNAND(u32 size, u32 id, u32 pointer,u32 filter)
{
    if (config_usesys) {
        char p[0x200];
        snprintf(p, 256, "%s/0001001700000000/config", config_sysdataoutpath);
        FILE* fd = fopen(p, "rb");
        u16 numb;
        u16 unk;
        fread(&numb, 1, 2, fd);
        fread(&unk, 1, 2, fd);
        int i;
        for (i = 0; i < numb; i++) {
            config_block conf;
            fread(&conf, 1, sizeof(conf), fd);
            if (id == Read32(conf.BlkID)) {
                u16 asize = (conf.size[0]) | (conf.size[1] << 8);
                u16 flags = (conf.flags[0]) | (conf.flags[1] << 8);
                u16 readsize = size < asize ? size : asize;
                DEBUG("found %04x %04x\n", asize,flags);
                if (filter & flags) {
                    if (size <= 4) {
                        for (int j = 0; j < readsize; j++) {
                            mem_Write8(pointer + j, conf.offset[j]);
                        }
                    } else {
                        u8 abuffer[0x8000]; //max size
                        long temp = ftell(fd);
                        fseek(fd, Read32(conf.offset), SEEK_SET);
                        fread(abuffer, 1, readsize, fd);
                        for (int j = 0; j < readsize; j++) {
                            mem_Write8(pointer + j, abuffer[j]);
                        }
                        fseek(fd, temp, SEEK_SET);
                    }
                    break;
                } else {
                    DEBUG("filtered out\n");
                }
            }
        }
        if (numb == i) {
            DEBUG("error not found\n");
        }
        fclose(fd);
    } else {
        switch (id) {
        case 0x00050005: //Stereo camera settings?
            mem_Write(stereo_camera_settings, pointer, sizeof(stereo_camera_settings));
            break;
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
        }
    }

    return 0;
}


SERVICE_START(cfg_u);

SERVICE_CMD(0x00010082)   // GetConfigInfoBlk2
{
    u32 size    = CMD(1);
    u32 id      = CMD(2);
    u32 pointer = CMD(4);

    DEBUG("GetConfigInfoBlk2 %08x %08x %08x\n", size, id, pointer);

    RESP(1, getconfigfromNAND(size, id, pointer, 0x2)); // Result
    return 0;
}

SERVICE_CMD(0x00020000)   // SecureInfoGetRegion
{
    DEBUG("SecureInfoGetRegion\n");

    RESP(1, 0); // Result
    RESP(2, config_region); // 2=EUROPE 1=USA
    return 0;
}

SERVICE_CMD(0x00030040)   // GenHashConsoleUnique
{
    DEBUG("GenHashConsoleUnique %08x\n", CMD(1));

    RESP(1, 0); // Result
    RESP(2, 0x33646D6F ^ (CMD(1) & 0xFFFFF)); //3dmooSHA
    RESP(3, 0x6F534841 ^ (CMD(1) & 0xFFFFF));
    return 0;
}

SERVICE_CMD(0x00040000)   // GetRegionCanadaUSA
{
    DEBUG("GetRegionCanadaUSA\n");

    RESP(1, 0); // Result
    RESP(2, 0); // USA
    return 0;
}

SERVICE_CMD(0x00050000)   // GetSystemModel
{
    DEBUG("GetSystemModel\n");

    RESP(1, 0); // Result
    RESP(2, 3); // 0=3DS, 1=3DSXL, 3=2DS
    return 0;
}

SERVICE_END();