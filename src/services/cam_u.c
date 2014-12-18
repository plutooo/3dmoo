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

SERVICE_START(cam_u);

SERVICE_CMD(0x50040)   //unk5
{
    DEBUG("unk5 %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x60040)   //unk6
{
    DEBUG("unk6 %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x80040)   //unk8
{
    DEBUG("unk8 %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x90100)   //unk9
{
    DEBUG("unk9 %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0xa0080)   //unkA
{
    DEBUG("unkA %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0xe0080)   //unkE
{
    DEBUG("unkE %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}


SERVICE_CMD(0x180080)   //unk18
{
    DEBUG("unk18 %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x190080)   //unk19
{
    DEBUG("unk19 %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x1B0080)   //unk1B
{
    DEBUG("unk1B %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x1F00C0)   //unk1F
{
    DEBUG("unk1F %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x200080)   //unk20
{
    DEBUG("unk20 %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x2B0000)   //unk2B
{
    DEBUG("Unknown_2B0000 --todo-- %08x %08x %08x\n", CMD(1), CMD(2), CMD(3));
    RESP(1, 0); // Result

    for(int i = 2; i < 18; i++)
        RESP(i, 0); // calibration data?
    return 0;
}

SERVICE_CMD(0x360000) //unk36
{
    DEBUG("unk36\n");
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x390000) //unk39
{
    DEBUG("Init? --todo-- %08x %08x %08x\n", CMD(1), CMD(2), CMD(3));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x3A0000) //unk3A
{
    DEBUG("unk3A\n");
    RESP(1, 0); // Result
    return 0;
}

SERVICE_END();