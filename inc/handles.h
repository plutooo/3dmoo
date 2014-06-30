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

#include "armdefs.h"

#define HANDLE_TYPE_UNK       0
#define HANDLE_TYPE_PORT      1
#define HANDLE_TYPE_SERVICE   2
#define HANDLE_TYPE_EVENT     3
#define HANDLE_TYPE_MUTEX     4
#define HANDLE_TYPE_SHAREDMEM 5
#define HANDLE_TYPE_REDIR     6
#define HANDLE_TYPE_THREAD    7
#define HANDLE_TYPE_PROCESS   8
#define HANDLE_TYPE_ARBITER   9
#define HANDLE_TYPE_FILE      10
#define HANDLE_TYPE_SEMAPHORE 11
#define HANDLE_TYPE_ARCHIVE   12
#define HANDLE_TYPE_SESSION   13


#define PORT_TYPE_SRV         0
#define PORT_TYPE_err_f       1

#define SERVICE_TYPE_APT_U       0
#define SERVICE_TYPE_GSP_GPU     1
#define SERVICE_TYPE_HID_USER    3
#define SERVICE_TYPE_FS_USER     4
#define SERVICE_TYPE_AM_USER     5
#define SERVICE_TYPE_NINSHELL_S  6
#define SERVICE_TYPE_NDM_USER    7
#define SERVICE_TYPE_CFG_USER    8
#define SERVICE_TYPE_PTM_USER    9
#define SERVICE_TYPE_FRD_USER   10
#define SERVICE_TYPE_IR_USER    11
#define SERVICE_TYPE_DSP_DSP    12
#define SERVICE_TYPE_CECD_U     13
#define SERVICE_TYPE_BOSS_U     14
#define SERVICE_DIRECT          15
#define SERVICE_TYPE_PTM_SYSTEM 16
#define SERVICE_TYPE_PDN_D      17
#define SERVICE_TYPE_cdc_DSP    18
#define SERVICE_TYPE_fs_ldr     19
#define SERVICE_TYPE_PxiPM      20
#define SERVICE_TYPE_fs_REG     21
#define SERVICE_TYPE_cfg_i      22
#define SERVICE_TYPE_cfg_nor    23
#define SERVICE_TYPE_hid_SPVR   24
#define SERVICE_TYPE_am_sys     25
#define SERVICE_TYPE_boss_P     26
#define SERVICE_TYPE_ps_ps      27
#define SERVICE_TYPE_CFG_S      28
#define SERVICE_TYPE_APT_S      29
#define SERVICE_TYPE_PDN_G      30
#define SERVICE_TYPE_MCU_GPU      31
#define SERVICE_TYPE_I2C_LCD      32

#define HANDLE_SUBEVENT_USER          0
#define HANDLE_SUBEVENT_APTMENUEVENT  1
#define HANDLE_SUBEVENT_APTPAUSEEVENT 2

#define HANDLE_MUTEX_APTMUTEX 2

#define LOCK_TYPE_ONESHOT 0
#define LOCK_TYPE_STICKY  1
#define LOCK_TYPE_PULSE   2
#define LOCK_TYPE_MAX     2

#define MEM_TYPE_GSP_0           0
#define MEM_TYPE_HID_0           1
#define MEM_TYPE_APT_SHARED_FONT 2
#define MEM_TYPE_ALLOC 3


#define HANDLE_CURRENT_THREAD  0xFFFF8000
#define HANDLE_CURRENT_PROCESS 0xFFFF8001

typedef struct {
    bool taken;
    u32  type;
    uintptr_t  subtype;

    bool locked;
    u32  locktype;
    u32  process;
    u32  thread;
    u32  handle;

    u32  misc[4];
    void* misc_ptr[4];
} handleinfo;

// handles.c
handleinfo* handle_Get(u32 handle);
u32 handle_New(u32 type, u32 subtype);

#ifdef modulesupport
u32 *curprocesshandlelist;
#endif
u32 curprocesshandle;

// services/srv.c
u32 services_SyncRequest(handleinfo* h, bool *locked);

// svc/syn.c
u32 mutex_WaitSynchronization(handleinfo* h, bool *locked);
u32 mutex_SyncRequest(handleinfo* h, bool *locked);

// svc/ports.c
u32 port_SyncRequest(handleinfo* h, bool *locked);

//syscalls/events.c
u32 event_WaitSynchronization(handleinfo* h, bool *locked);

u32 file_SyncRequest(handleinfo* h, bool *locked);

//arm11/threads.c
u32 thread_SyncRequest(handleinfo* h, bool *locked);
u32 thread_CloseHandle(ARMul_State *state, handleinfo* h);
u32 thread_WaitSynchronization(handleinfo* h, bool *locked);

// svc/syn.c
u32 semaphore_WaitSynchronization(handleinfo* h, bool *locked);
u32 semaphore_SyncRequest(handleinfo* h, bool *locked);

static struct {
    char* name;
    u32(*fnSyncRequest)(handleinfo* h, bool *locked);
    u32(*fnCloseHandle)(ARMul_State *state, handleinfo* h);
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
        &event_WaitSynchronization
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
    },
    {
        "redir",
        NULL,
        NULL,
        NULL
    },
    {
        "thread",
        &thread_SyncRequest,
        &thread_CloseHandle,
        &thread_WaitSynchronization
    },
    {
        "process",
        NULL,
        NULL,
        NULL
    },
    {
        "arbiter",
        NULL,
        NULL,
        NULL
    },
    {
        "file",
        &file_SyncRequest,
        NULL,
        NULL
    },
    {
        "semaphore",
        &semaphore_SyncRequest,
        NULL,
        &semaphore_WaitSynchronization
    },
    {
        "archive",
        NULL,
        NULL,
        NULL
    },
    {
        "session",
        NULL,
        NULL,
        NULL
    }
};
