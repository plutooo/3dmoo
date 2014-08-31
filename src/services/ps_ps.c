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

SERVICE_START(ps_ps);

SERVICE_CMD(0x00020244)
{
    u32 a1 = CMD(1);
    u32 a2 = CMD(2);
    u32 a3 = CMD(3);
    u32 a4 = CMD(4);
    u32 a5 = CMD(5);
    u32 a6 = CMD(6);
    u32 a7 = CMD(7);
    u32 a8 = CMD(8);
    u32 a9 = CMD(9);
    u32 aa = CMD(0xA);
    u32 ab = CMD(0xB);
    u32 ac = CMD(0xC);
    u32 ad = CMD(0xD);

    DEBUG("VerifyRsaSha256 %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", a1, a2, a3, a4, a5, a6, a7, a8, a9, aa, ab, ac, ad);

    RESP(1, 0); // Result this is a patch to allow homebrew
    return 0;
}

SERVICE_END();
