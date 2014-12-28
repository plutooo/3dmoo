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
    DEBUG("LoadProgram %08x %08x %08x %08x\n", CMD(1), CMD(2), CMD(3), CMD(4));

    RESP(1, 0); // Done
    return 0;
}

SERVICE_CMD(0x040300C0)   // GetProgramInfo may wants data back in a type 1 resp (sets up a buffer in 0x180)
{
    DEBUG("GetProgramInfo %08x %08x %08x\n", CMD(1), CMD(2), CMD(3));
    RESP(1, 0); // Done

    FILE* a = fopen("title/00040130/00008002/content/00000011.app.exheader","rb");

    char exhdr[0x800];
    fread(exhdr, 1, 0x800, a);
    fclose(a);
    mem_Write(exhdr, EXTENDED_CMD(1), 0x400);

    return 0;
}

SERVICE_END();
