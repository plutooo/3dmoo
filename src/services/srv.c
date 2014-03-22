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
#include <stdlib.h>

#include "../util.h"
#include "../handles.h"

u32 apt_u_SyncRequest();

static struct {
    const char* name;
    u32 subtype;
    u32 handle;
    u32 (*fnSyncRequest)();
} services[] = {
    // Services are declared here.
    {
	"APT:U",
	SERVICE_TYPE_APT_U,
	0,
	&apt_u_SyncRequest
    }
};


u32 srv_InitHandle() {
    // Create a handle for srv: port.
    arm11_SetR(1, handle_New(HANDLE_TYPE_PORT, PORT_TYPE_SRV));

    // Create handles for all services.
    u32 i;
    for(i=0; i<ARRAY_SIZE(services); i++) {
	services[i].handle = handle_New(HANDLE_TYPE_SERVICE, services[i].subtype);
    }

    return 0;
}

u32 srv_SyncRequest() {
    u32 cid = mem_Read32(0xFFFF0080);

    // Read command-id.
    switch(cid) {

    case 0x10002:
	DEBUG("srv_RegisterClient\n");

	// XXX: check +4, flags?
	PAUSE();
	break;

    case 0x50100:
	DEBUG("srv_GetServiceHandle\n");

        struct {
            char name[8];
            uint32_t name_len;
            uint32_t unk2;
       } req;

	// Read rest of command header
	mem_Read(&req, 0xFFFF0084, sizeof(req));

	if(req.name[7] != '\0') {
	    ERROR("Missing null-byte in name.\n");
	    arm11_Dump();
	    exit(1);
	}

	u32 i;
	for(i=0; i<ARRAY_SIZE(services); i++) {
	    // Find service in list.
	    if(strcmp(req.name, services[i].name) == 0) {

		// Write result.
		mem_Write32(0xFFFF0084, 0);

		// Write handle_out.
		mem_Write32(0xFFFF008C, services[i].handle);

		return 0;
	    }
	}

	ERROR("Unimplemented service: %s\n", req.name);
	arm11_Dump();
	exit(1);

    default:
	ERROR("Unimplemented command %08x in \"srv:\"\n", cid);
	arm11_Dump();
	exit(1);
    }

    return 0;
}
