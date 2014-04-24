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

#define HANDLE_TYPE_UNK       0
#define HANDLE_TYPE_PORT      1
#define HANDLE_TYPE_SERVICE   2
#define HANDLE_TYPE_EVENT     3
#define HANDLE_TYPE_MUTEX     4
#define HANDLE_TYPE_SHAREDMEM 5
#define HANDLE_TYPE_REDIR     6
#define HANDLE_TYPE_THREAD    7
#define HANDLE_TYPE_PROCESS   8

#define PORT_TYPE_SRV         0

#define SERVICE_TYPE_APT_U    0
#define SERVICE_TYPE_GSP_GPU  1
#define SERVICE_TYPE_HID_USER 3
#define SERVICE_TYPE_FS_USER  4

#define HANDLE_SUBEVENT_USER          0
#define HANDLE_SUBEVENT_APTMENUEVENT  1
#define HANDLE_SUBEVENT_APTPAUSEEVENT 2

#define HANDLE_MUTEX_APTMUTEX 2

#define LOCK_TYPE_ONESHOT 0
#define LOCK_TYPE_STICKY  1
#define LOCK_TYPE_PULSE   2
#define LOCK_TYPE_MAX   2

#define MEM_TYPE_GSP_0   0


#define HANDLE_CURRENT_THREAD  0xFFFF8000
#define HANDLE_CURRENT_PROCESS 0xFFFF8001

typedef struct {
    bool taken;
    u32  type;
    u32  subtype;
    bool locked;
    u32  locktype;
    u32 process;
    u32 thread;
} handleinfo;

handleinfo* handle_Get(u32 handle);
u32 handle_New(u32 type, u32 subtype);

u32 curprocesshandle;

// services/srv.c
u32 services_SyncRequest(handleinfo* h, bool *locked);

// svc/syn.c
u32 mutex_WaitSynchronization(handleinfo* h, bool *locked);
u32 mutex_SyncRequest(handleinfo* h, bool *locked);

// svc/ports.c
u32 port_SyncRequest(handleinfo* h, bool *locked);

//syscalls/events.c
u32 Event_WaitSynchronization(handleinfo* h, bool *locked);

static struct {
    char* name;
    u32(*fnSyncRequest)(handleinfo* h, bool *locked);
    u32(*fnCloseHandle)(handleinfo* h);
    u32(*fnWaitSynchronization)(handleinfo* h, bool *locked);

} handle_types[] = {
    {
        "misc",
        NULL,
        NULL,
        NULL
    },
    {
        "port",
        &port_SyncRequest,
        NULL,
        NULL
    },
    {
        "service",
        &services_SyncRequest,
        NULL,
        NULL
    },
    {
        "event",
        NULL,
        NULL,
        &Event_WaitSynchronization
    },
    {
        "mutex",
        &mutex_SyncRequest,
        NULL,
        &mutex_WaitSynchronization
    },
    {
        "mem",
        NULL,
        NULL,
        NULL
    }
};