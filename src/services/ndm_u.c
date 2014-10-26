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

#include "service_macros.h"

SERVICE_START(ndm_u);

SERVICE_CMD(0x60040)   //SuspendDaemons
{
    DEBUG("SuspendDaemons %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x140040)   //OverrideDefaultDaemons
{
    DEBUG("OverrideDefaultDaemons %08x\n",CMD(1));
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x00080040)//DisableWifiUsage
{
    DEBUG("DisableWifiUsage %08x\n", CMD(1));
    RESP(1, 0); // Result
    return 0;
}
SERVICE_END();
