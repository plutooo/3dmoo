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

#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"

#define MAX_NUM_HANDLES 0x1000
#define HANDLES_BASE    0xDEADBABE

static handleinfo handles[MAX_NUM_HANDLES];
static u32 handles_num;

static struct {
    char* name;
    u32   (*fnSyncRequest)(handleinfo* h);
    u32   (*fnCloseHandle)(handleinfo* h);

} handle_types[] = {
    {	"misc",
	NULL,
	NULL
    },
    {	"port",
	&port_SyncRequest,
	NULL
    },
    {	"service",
	&services_SyncRequest,
	NULL
    },
    {	"event",
	NULL,
	NULL
    },
    {	"mutex",
	NULL,
	NULL
    }
};

#define NUM_HANDLE_TYPES ARRAY_SIZE(handle_types)


u32 handle_New(u32 type, u32 subtype) {
    if(handles_num == MAX_NUM_HANDLES) {
	ERROR("not enough handles..\n");
	arm11_Dump();
	exit(1);
    }

    handles[handles_num].taken    = true;
    handles[handles_num].type     = type;
    handles[handles_num].subtype  = subtype;
    handles[handles_num].locked   = false;
    handles[handles_num].locktype = LOCK_TYPE_STICKY;

    return HANDLES_BASE + handles_num++;
}

handleinfo* handle_Get(u32 handle) {
    u32 idx = handle - HANDLES_BASE;

    if(idx < handles_num)
	return &handles[idx];

    return NULL;
}


/* Generic SVC implementations. */
u32 svcSendSyncRequest() {
    u32 handle = arm11_R(0);
    handleinfo* hi = handle_Get(handle);

    if(hi == NULL) {
	ERROR("handle %08x not found.\n", handle);
	PAUSE();
	exit(1);
    }

    if(hi->type >= NUM_HANDLE_TYPES) {
	// This should never happen.
	ERROR("handle %08x has non-defined type.\n", handle);
	PAUSE();
	exit(1);
    }

    // Lookup actual callback in table.
    if(handle_types[hi->type].fnSyncRequest != NULL)
	return handle_types[hi->type].fnSyncRequest(hi);

    ERROR("svcSyncRequest undefined for handle-type \"%s\".\n",
	  handle_types[hi->type].name);
    PAUSE();
    exit(1);
}

u32 svcCloseHandle() {
    u32 handle = arm11_R(0);
    handleinfo* hi = handle_Get(handle);

    if(hi == NULL) {
	ERROR("handle %08x not found.\n", handle);
	PAUSE();
	exit(1);
    }

    if(hi->type >= NUM_HANDLE_TYPES) {
	// This should never happen.
	ERROR("handle %08x has non-defined type.\n", handle);
	PAUSE();
	exit(1);
    }

    // Lookup actual callback in table.
    if(handle_types[hi->type].fnCloseHandle != NULL)
	return handle_types[hi->type].fnCloseHandle(hi);

    ERROR("svcCloseHandle undefined for handle-type \"%s\".\n",
	  handle_types[hi->type].name);
    PAUSE();
    return 0;
}
