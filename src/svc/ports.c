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

u32 svcConnectToPort() {
    u32 handle_out   = arm11_R(0);
    u32 portname_ptr = arm11_R(1);;
    char name[12];

    u32 i;
    for(i=0; i<12; i++) {
	name[i] = mem_Read8(portname_ptr+i);
	if(name[i] == '\0')
	    break;
    }

    if(i == 12 && name[7] != '\0') {
	ERROR("requesting service with missing null-byte\n");
	arm11_Dump();
	PAUSE();
	return 0xE0E0181E;
    }

    DEBUG("portname=%s\n", name);
    PAUSE();
    return 0;
}

u32 svcSendSyncRequest() {
    u32 session = arm11_R(0);

    DEBUG("STUBBED.\n");
    PAUSE();
    return 0;
}
