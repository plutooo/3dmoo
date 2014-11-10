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

SERVICE_START(mic_u);

SERVICE_CMD(0x10042)
{
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x30140)
{
	RESP(1, 0);
	return 0;
}

SERVICE_CMD(0x4f0080)
{
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x80040)
{
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x90000) //GetControl
{
	RESP(1, 0);
	return 0;
}

SERVICE_CMD(0xA0040) //SetBias(u8 input)
{
	RESP(1, 0);
	return 0;
}

SERVICE_CMD(0xd0040)
{
    RESP(1, 0);
    return 0;
}

SERVICE_END();

