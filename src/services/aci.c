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

SERVICE_START(ac_i);

SERVICE_END();

SERVICE_START(ac_u);

SERVICE_CMD(0xD0000) //GetWifiStatus
{
    DEBUG("GetWifiStatus\n");

    RESP(1, 0); // Result
    RESP(2, 0); // Output connection type: 0 = none, 1 = Internet
    return 0;
}

SERVICE_CMD(0x300004) //RegisterDisconnectEvent
{
	DEBUG("RegisterDisconnectEvent\n");

	RESP(1, 0); // Result
	return 0;
}

SERVICE_CMD(0x3e0042) //IsConnected
{
    DEBUG("IsConnected\n");

    RESP(1, 0); // Result
    RESP(2, 0); // 0=Not Connected
    return 0;
}

SERVICE_CMD(0x400042) //SetClientVersion
{
    DEBUG("SetClientVersion\n");

    RESP(1, 0); // Result
    return 0;
}

SERVICE_END();