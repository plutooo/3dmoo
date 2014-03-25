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

#include "util.h"
#include "mem.h"
#include "arm11.h"

#define SVCERROR_ALIGN_ADDR        0xE0E01BF1
#define SVCERROR_INVALID_SIZE      0xE0E01BF2
#define SVCERROR_INVALID_PARAMS    0xE0E01BF5
#define SVCERROR_INVALID_OPERATION 0xE0E01BEE

#define CONTROL_OP_FREE    1
#define CONTROL_OP_RESERVE 2
#define CONTROL_OP_COMMIT  3
#define CONTROL_OP_MAP     4
#define CONTROL_OP_UNMAP   5
#define CONTROL_OP_PROTECT 6

#define CONTROL_GSP_FLAG 0x10000


u32 svcControlMemory() {
    u32 perm  = arm11_R(0);
    u32 addr0 = arm11_R(1);
    u32 addr1 = arm11_R(2);
    u32 size  = arm11_R(3);
    u32 op    = arm11_R(4);

    const char* ops;
    switch(op & 0xFF) {
    case 1: ops = "FREE"; break;
    case 2: ops = "RESERVE"; break;
    case 3: ops = "COMMIT"; break;
    case 4: ops = "MAP"; break;
    case 5: ops = "UNMAP"; break;
    case 6: ops = "PROTECT"; break;
    default: ops = "UNDEFINED"; break;
    }

    const char* perms;
    switch(perm) {
    case 0: perms = "--"; break;
    case 1: perms = "-R"; break;
    case 2: perms = "W-"; break;
    case 3: perms = "WR"; break;
    case 0x10000000: perms = "DONTCARE"; break;
    default: perms = "UNDEFINED";
    }

    DEBUG("op=%s %s (%x), addr0=%x, addr1=%x, size=%x, perm=%s (%x)\n",
	  ops, op & CONTROL_GSP_FLAG ? "GSP" : "", op,
	  addr0, addr1, size, perms, perm);
    PAUSE();

    if(addr0 & 0xFFF)
	return SVCERROR_ALIGN_ADDR;
    if(addr1 & 0xFFF)
	return SVCERROR_ALIGN_ADDR;
    if(size & 0xFFF)
	return SVCERROR_INVALID_SIZE;

    if(op == 0x10003) { // FFF680A4
	if(addr0 == 0) { // FFF680C4
	    if(addr1 != 0)
		return SVCERROR_INVALID_PARAMS;
	}
	else if(size == 0) { // FFF680D0
	    if(addr0 < 0x14000000)
		return SVCERROR_INVALID_PARAMS;
	    if((addr0+size) >= 0x1C000000)
		return SVCERROR_INVALID_PARAMS;
	    if(addr1 != 0)
		return SVCERROR_INVALID_PARAMS;
	}
	else {
	    if(addr0 < 0x14000000)
		return SVCERROR_INVALID_PARAMS;
	    if(addr0 >= 0x1C000000)
		return SVCERROR_INVALID_PARAMS;
	    if(addr1 != 0)
		return SVCERROR_INVALID_PARAMS;
	}
    }
    else if(op == 1) {
	if(size == 0) { // FFF68110
	    if(addr0 < 0x08000000) // FFF68130
		return SVCERROR_INVALID_PARAMS;
	    if(addr0 <= 0x1C000000)
		return SVCERROR_INVALID_PARAMS;
	}
	else {
	    if(addr0 < 0x08000000)
		return SVCERROR_INVALID_PARAMS;
	    if((addr0+size) <= 0x1C000000)
		return SVCERROR_INVALID_PARAMS;
	}
    }
    else {
	if(size == 0) { // FFF68148
	    if(addr0 < 0x08000000)
		return SVCERROR_INVALID_PARAMS;
	    if(addr0 >= 0x14000000)
		return SVCERROR_INVALID_PARAMS;
	}
	else {
	    if(addr0 < 0x08000000)
		return SVCERROR_INVALID_PARAMS;
	    if((addr0+size) >= 0x14000000)
		return SVCERROR_INVALID_PARAMS;
	}

	if(op == 4 || op == 5) { // FFF680E8
	    if(size == 0) {
		if(addr1 < 0x100000) // FFF681CC
		    return SVCERROR_INVALID_PARAMS;
		if(addr1 >= 0x14000000)
		    return SVCERROR_INVALID_PARAMS;
	    }
	    if(addr1 < 0x100000)
		return SVCERROR_INVALID_PARAMS;
	    
	    if((addr1+size) >= 0x14000000)
		return SVCERROR_INVALID_PARAMS;
	}
    }

    // ????
    switch(op & 0xff) {
    case 1: case 3: case 4: case 5: case 6:
	break;
    default:
	return SVCERROR_INVALID_OPERATION;
    }

    if(size == 0)
	return 0;

    //kprocess = *0xFFFF9004;
    //*(SP+0x10) = kprocess + 0x1c;

    // ???
    /*
    u32 flags = outaddr & 0xff;
    if(flags != 1) {
	if(perms != 0 && perms != 1 && perms != 2 && perms != 3)
	    return SVCERROR_INVALID_OPERATION;
    }
    */

    if(op == 0x10003) {
	DEBUG("Mapping GSP heap..\n");
	arm11_SetR(1, 0x08000000); // outaddr is in R1
	return mem_AddSegment(0x08000000, size, NULL);
    }

    DEBUG("STUBBED!\n");
    PAUSE();

    /*
    // FFF6824C
    r11 = outaddr & 0xFFFFFF;
    is_ldr = GetKProcessID() == 1 ? 0xFFFFFFFF : 0;
    r2 = r2 & r11;
    if(r2 & 0xF00) {
    r2 = *(kprocess + 0xa0);
    r11 = (r11 & 0xFFFFF0FF) | (r2 & 0xF00);
    }
    if(flags == 3 && !is_ldr) {
    if(sub_FFF72828(*r10, 1, r5) == 0)
    return 0xC860180A;
    }
    s32 rc = sub_FFF741B4(*(SP+16), (returnval in r1) SP+12, r4, r6, r5, r11, r7);
    if(rc < 0) {
    //FFF682F8
    if(flags == 1)
    sub_FFF7A0E8(*r10, 1, r5);
    }
    if(flags == 3)
    sub_FFF7A0E8(*r10, 1, r5);
    */

    return -1;
}

u32 svcMapMemoryBlock() {
    u32 handle     = arm11_R(0);
    u32 addr       = arm11_R(1);
    u32 my_perm    = arm11_R(2);
    u32 other_perm = arm11_R(3);

    DEBUG("STUBBED\nhandle=%x, addr=%08x, my_perm=%x, other_perm=%x\n",
	  handle, addr, my_perm, other_perm);
    PAUSE();
    return 0;
}
