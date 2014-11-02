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

SERVICE_START(am_u);

SERVICE_CMD(0x00080000)   // TitleIDListGetTotal3
{
    DEBUG("TitleIDListGetTotal3 - STUBBED -\n");
    RESP(2, 0); // we have 0 Total titles
    RESP(1, 0);
    return 0;
}
SERVICE_CMD(0x00090082)   // GetTitleIDList3
{
    DEBUG("TitleIDListGetTotal3 - STUBBED -\n");
    RESP(2, 0); // we have 0 titles on that medium
    RESP(1, 0);
    return 0;
}
SERVICE_CMD(0x00010040)   //TitleIDListGetTotal
{
    DEBUG("TitleIDListGetTotal (%08x) - STUBBED -\n", CMD(1));
    RESP(2, 0); // we have 0 titles on that medium
    RESP(1, 0); // we have 0 titles on that medium
    return 0;
}
SERVICE_CMD(0x00020082)   //GetTitleIDList
{
    DEBUG("GetTitleIDList (%08x %08x %08x) - STUBBED -\n", CMD(1), CMD(2), CMD(4));
    RESP(1, 0); // we have 0 titles on that medium
    return 0;
}

SERVICE_CMD(0x000B0040)   //unk
{
    DEBUG("unk B (%08x) - STUBBED -\n", CMD(1));
    RESP(1, 0);
    return 0;
}
SERVICE_CMD(0x00130040)   // ??
{
    DEBUG("??_00130040 - STUBBED -\n");
    RESP(1, 0);
    return 0;
}

SERVICE_END();
