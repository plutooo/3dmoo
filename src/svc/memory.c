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
#include "../mem.h"
#include "../arm11/arm11.h"

#define SVCERROR_ALIGN_ADDR     0xE0E01BF1
#define SVCERROR_INVALID_SIZE   0xE0E01BF2
#define SVCERROR_INVALID_PARAMS 0xE0E01BF5

#define CONTROL_OP_NOOP    0
#define CONTROL_OP_FREE    1
#define CONTROL_OP_RESERVE 2
#define CONTROL_OP_COMMIT  3
#define CONTROL_OP_MAP     4
#define CONTROL_OP_UNMAP   5
#define CONTROL_OP_PROTECT 6

#define CONTROL_LINEAR_FLAG 0x1000


u32 svcControlMemory() {
    u32 addr0     = arm11_R(1);
    u32 addr1     = arm11_R(2);
    u32 size      = arm11_R(3);
    u32 operation = arm11_R(4);
    //u32 permissions = arm11_R(5); ????

    const char* op;
    const char* reg;

    switch(operation & 0xFF) {
    case 1: op = "FREE"; break;
    case 2: op = "RESERVE"; break;
    case 3: op = "COMMIT"; break;
    case 4: op = "MAP"; break;
    case 5: op = "UNMAP"; break;
    case 6: op = "PROTECT"; break;
    default: op = "UNKNOWN"; break;
    }

    switch(operation & 0xF00) {
    case 0x000: reg = "REGION NORMAL"; break;
    case 0x100: reg = "REGION APP"; break;
    case 0x200: reg = "REGION SYSTEM"; break;
    case 0x300: reg = "REGION BASE"; break;
    default: reg = "REGION UNKNOWN"; break;
    }

    DEBUG("out_addr=%08x\naddr_in0=%08x\naddr_in1=%08x\n"
          "size=%08x\noperation=%08x\nop=%s (%x)\nreg=%s (%x)\nis_linear=%d\n",
          arm11_R(0), arm11_R(1), arm11_R(2), arm11_R(3),
	      operation,
	      op, operation & 0xFF,
	      reg, operation & 0xF00,
	      !!(operation & 0x1000));
    PAUSE();

    // Check inputs.
    if(addr0 & 0x00000FFF || addr1 & 0x00000FFF) {
	ERROR("got non-aligned addr0/1\n");
	arm11_Dump();
	PAUSE();
	return SVCERROR_ALIGN_ADDR;
    }

    if(size & 0x00000FFF) {
	ERROR("got non-aligned size\n");
	arm11_Dump();
	PAUSE();
	return SVCERROR_INVALID_SIZE;
    }

    if(operation == (CONTROL_LINEAR_FLAG | CONTROL_OP_COMMIT)) {
	if(addr0 == 0) {
	    if(addr1 != 0) {
		ERROR("comitting linear region, if addr0 == 0, then addr1 must be == 0\n");
		arm11_Dump();
		PAUSE();
		return SVCERROR_INVALID_PARAMS;
	    }
	}
	else if(size != 0) {
	    if(addr0 <= 0x14000000 || (addr0+size) > 0x1C000000 || addr1 != 0) {
		ERROR("got invalid [addr0, addr0+size) range, or addr1 == 0\n");
		arm11_Dump();
		PAUSE();
		return SVCERROR_INVALID_PARAMS;
	    }
	}
    }
    else if(operation == CONTROL_OP_FREE) {
	if(addr0 < 0x08000000 || (addr0+size) >= 0x1C000000) {
	    ERROR("trying to free outside heap region\n");
	    arm11_Dump();
	    PAUSE();
	    return SVCERROR_INVALID_PARAMS;
	}
    }
    else {
	if(addr0 < 0x08000000 || (addr0+size) > 0x14000000) {
	    ERROR("got invalid region outside (0x08000000-0x14000000)\n");
	    arm11_Dump();
	    PAUSE();
	    return SVCERROR_INVALID_PARAMS;
	}

	if(operation == CONTROL_OP_MAP || operation == CONTROL_OP_UNMAP) {
	    if(addr1 >= 0x00100000 || (addr1+size) < 0x14000000) {
		ERROR("trying to map forbidden region 0x00100000-0x14000000\n");
		arm11_Dump();
		PAUSE();
		return SVCERROR_INVALID_PARAMS;
	    }
	}
    }

    u32 operationId = operation & 0xFF;

    switch(operationId) {
    default:
	return 0xE0E01BEE;
    case 1:
    case 3:
    case 4:
    case 5:
    case 6:
        break;
    }

    // Do operation.
    if(operationId != CONTROL_OP_FREE) {
        /*if(permissions == 2 || permissions == 3) {
            return 0xE0E01BEE;
        }*/
	
	/*u32 access;
	
        if(GetProcessId() != 1) {
	    access = operation & 0xF00;

	    if(access) {
		access  = access & ~0xF00;
                access |= GetExheaderKernelDescriptor() & 0xF00;
	    }
        }*/

	/*if(operationId != CONTROL_OP_COMMIT || GetProcessId() == 1) {
            if(!sub_FFF682A4(GetUserKProcess(), 1, size)) {
		return 0xC860180A;
	    }
	}
	
	u32 handle_out = 0;*/
	int rc = 0;

	// "Guess" implementation that can be used to map GSP heap
	if(operationId == CONTROL_OP_COMMIT) {
	    DEBUG("here\n");
	    PAUSE();

	    arm11_SetR(1, 0x08000000); // return value is in R1?
	    return mem_AddSegment(addr0, size, NULL) == 0 ? 0 : -1;
	}

        // XXX:
        //rc = sub_FFF741B4(GetUserKProcess()+0x1C, &handle_out, addr0, addr1, size, access, addr_out, 0);
	
	if(rc > 0) {
	    if(operationId == CONTROL_OP_FREE) {
                 //sub_FFF7A0E8(GetUserKProcess(), 1, size);
	    }
	    //*unk_out = handle_out;
	    return 0;
	}
	
	if(operationId == CONTROL_OP_COMMIT) {
            //sub_FFF7A0E8(GetUserKProcess(), 1, size);
	}
	
	return rc;
    }

    return 0;
}
