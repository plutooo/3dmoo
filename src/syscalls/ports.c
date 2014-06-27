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
#include "arm11.h"
#include "mem.h"
#include "handles.h"

// services/srv.c
u32 srv_InitHandle();
u32 srv_SyncRequest();




u32 err_f_InitHandle()
{
    arm11_SetR(1, handle_New(HANDLE_TYPE_PORT, PORT_TYPE_err_f));
    arm11_Dump();
    return 0;
}
u32 err_f_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);

    // Read command-id.
    switch (cid) {

    default:
        ERROR("Unimplemented command %08x in \"err:f\"\n", cid);
        //arm11_Dump();
        //exit(1);
    }

    return 0;
}

static struct {
    const char* name;
    u32 subtype;
    u32 (*fnInitHandle)();
    u32 (*fnSyncRequest)();
} ports[] = {
    // Ports are declared here.
    {
        "srv:",
        PORT_TYPE_SRV,
        &srv_InitHandle,
        &srv_SyncRequest
    },
    {
        "err:f",
        PORT_TYPE_err_f,
        &err_f_InitHandle,
        &err_f_SyncRequest
    },
};

u32 svcConnectToPort()
{
    //u32 handle_out   = arm11_R(0);
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

    for(i=0; i<ARRAY_SIZE(ports); i++) {
        if(strcmp(name, ports[i].name) == 0) {
            DEBUG("Found port %s!\n", name);
            return ports[i].fnInitHandle();
        }
    }

    DEBUG("Port %s: NOT IMPLEMENTED!\n", name);
    arm11_Dump();
    PAUSE();
    return 0;
}

u32 port_SyncRequest(handleinfo* h, bool *locked)
{
    u32 i;

    for(i=0; i<ARRAY_SIZE(ports); i++) {
        if(h->subtype == ports[i].subtype)
            return ports[i].fnSyncRequest();
    }

    // This should never happen.
    ERROR("svcCloseHandle undefined for port-type \"%x\".\n",
          h->subtype);
    arm11_Dump();
    PAUSE();
    return 0;
}
u32 svcCreatePort()
{
    u32 portServer = arm11_R(0);
    u32 portClient = arm11_R(1);
    u32 name = arm11_R(2);
    u32 maxSessions = arm11_R(3);
    DEBUG("CreatePort %08x %08x %08x %08x\n", portServer, portClient, name, maxSessions);
    char b[0x100];
    mem_Read(b, name, 0x100);
    DEBUG("%s\n",b);
    DEBUG("Stumb\n");
    return 0;
}