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

SERVICE_START(boss_u);

SERVICE_CMD(0x00010082)   //???
{
	u32 unk1 = CMD(1);
	u32 unk2 = CMD(2);
	u32 unk3 = CMD(3);
	u32 unk4 = CMD(4);
	u32 unk5 = CMD(5);
	u8  unk6 = CMD(6);
	u32 unk7 = CMD(7);
	DEBUG("???_10082 %08x %08x %08x %08x %08x %02x %08x\n", unk1, unk2, unk3, unk4, unk5, unk6, unk7);

	RESP(1, 0); // Result
	return 0;
}

SERVICE_CMD(0x00100102)   // ???
{
    DEBUG("???_100102 %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x002700c2)   // ???
{
    DEBUG("???_2700c2 %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));

    RESP(1, 0);
    return 0;
}

SERVICE_END();
