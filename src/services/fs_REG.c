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

SERVICE_START(fs_REG);

SERVICE_CMD(0x04060080)   // CheckHostLoadId
{
    DEBUG("CheckHostLoadId %08x %08x\n", CMD(1), CMD(2));

    RESP(1, 0); // Is OK
    return 0;
}
SERVICE_CMD(0x04040100)   // LoadProgram
{
    DEBUG("LoadProgram %08x %08x %08x %08x --Warning this returns ID as ID but ID on the real hw is different use the new ID for all future actions --\n", CMD(1), CMD(2), CMD(3), CMD(4));

    u32 IDlow = CMD(1);
    u32 IDhigh = CMD(2);

    RESP(2, IDlow); //the ID of the programm for easy access just
    RESP(3, IDhigh); //unknown

    RESP(1, 0); // Done
    return 0;
}

SERVICE_CMD(0x040300C0)   // GetProgramInfo may wants data back in a type 1 resp (sets up a buffer in 0x180)
{
    DEBUG("GetProgramInfo %08x %08x %08x\n", CMD(1), CMD(2), CMD(3));
    RESP(1, 0); // Done

    char tmp[0x100];
    sprintf(tmp, "title/%08x/%08x/a.exheader", CMD(3), CMD(2));

    FILE* a = fopen(tmp, "rb");

    char exhdr[0x800];
    fread(exhdr, 1, 0x800, a);
    fclose(a);
    mem_Write(exhdr, EXTENDED_CMD(1), 0x400);

    return 0;
}

SERVICE_CMD(0x040103C0)   // Register
{
    DEBUG("Register %08x %08x %08x %08x %08x\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5));
    DEBUG("Register %08x %08x %08x %08x %08x\n", CMD(5), CMD(6), CMD(7), CMD(8), CMD(9));
    DEBUG("Register %08x %08x %08x %08x %08x\n", CMD(10), CMD(11), CMD(12), CMD(13), CMD(14));
    DEBUG("Register %08x %08x %08x %08x %08x\n", CMD(15));
    RESP(1, 0); // Done

    return 0;
}

SERVICE_END();
