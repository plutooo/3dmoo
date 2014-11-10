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

SERVICE_CMD(0x390000) //uk39
{
    DEBUG("unk39\n");
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x80040)   //uk8
{
    DEBUG("unk8 %08x\n",CMD(1));
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x60040)   //uk6
{
    DEBUG("unk6 %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x50040)   //uk5
{
    DEBUG("unk5 %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_END();