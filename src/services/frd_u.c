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


SERVICE_START(frd_u);

SERVICE_CMD(0x00010000)   // ???
{
    DEBUG("???_10000 %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    RESP(1, -1);
    return 0;
}

SERVICE_CMD(0x00080000)   //GetMyPresence
{
    DEBUG("GetMyPresence %08x,%08x\n",
          EXTENDED_CMD(0), EXTENDED_CMD(1));

    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x00110080)   // ???
{
    DEBUG("???_110080 %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00120042)   // ???
{
    DEBUG("???_120042 %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00170042)   // ???
{
    DEBUG("???_170042 %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00200002)   //???
{
	DEBUG("???_200002 %08x,%08x\n",
		EXTENDED_CMD(0), EXTENDED_CMD(1));

	RESP(1, -1); // Result
	return 0;
}

SERVICE_CMD(0x00210040)   // ???
{
    DEBUG("???_210040 %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00270040)   // ???
{
    DEBUG("???_270040 %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00320042)   //SetClientSdkVersion
{
    DEBUG("SetClientSdkVersion %08x\n", EXTENDED_CMD(0));

    RESP(1, 0); // Result
    return 0;
}

SERVICE_END();
