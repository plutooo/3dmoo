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

#include <stdio.h>

#include "../util.h"

u32 svcControlMemory() {
    const char* op;

    switch(arm11_R(4)) {
    case 1: op = "FREE"; break;
    case 2: op = "RESERVE"; break;
    case 3: op = "COMMIT"; break;
    case 4: op = "MAP"; break;
    case 5: op = "UNMAP"; break;
    case 6: op = "PROTECT"; break;
    case 0x100: op = "REGION APP"; break;
    case 0x200: op = "REGION SYSTEM"; break;
    case 0x300: op = "REGION BASE"; break;
    case 0x1000: op = "LINEAR"; break;
    default: op = "UNKNOWN";
    }

    DEBUG("out_addr=%08x\naddr_in0=%08x\naddr_in1=%08x\nsize=%08x\ntype=%s (%x)\n",
	  arm11_R(0), arm11_R(1), arm11_R(2), arm11_R(3), op, arm11_R(4));

    PAUSE();
    return 0;
}
