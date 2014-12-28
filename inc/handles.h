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

#define HANDLES_BASE    0x0DADBABE

#define HANDLE_SERV_STAT_CONNECTED         0x1
#define HANDLE_SERV_STAT_CONNECTEDOPEN     0x2
#define HANDLE_SERV_STAT_SYN               0x4
#define HANDLE_SERV_STAT_SYN_IN_PROGRESS   0x8
#define HANDLE_SERV_STAT_REPLY             0x10

#define SERVERFREE                         0x1

#define HANDLE_TYPE_UNK                     0
#define HANDLE_TYPE_PORT                    1
#define HANDLE_TYPE_SERVICE                 2
#define HANDLE_TYPE_EVENT                   3
#define HANDLE_TYPE_MUTEX                   4
#define HANDLE_TYPE_SHAREDMEM               5
#define HANDLE_TYPE_REDIR                   6
#define HANDLE_TYPE_THREAD                  7
#define HANDLE_TYPE_PROCESS                 8
#define HANDLE_TYPE_ARBITER                 9
#define HANDLE_TYPE_FILE                    10
#define HANDLE_TYPE_SEMAPHORE               11
#define HANDLE_TYPE_ARCHIVE                 12
#define HANDLE_TYPE_SESSION                 13
#define HANDLE_TYPE_DIR                     14
#define HANDLE_TYPE_SOCKET                  15
#define HANDLE_TYPE_HTTPCont                16
#define HANDLE_TYPE_SERVICE_UNMOUNTED       17
#define HANDLE_TYPE_SERVICE_SERVER          18
#define HANDLE_TYPE_TIMER                   19
#define HANDLE_TYPE_KResourceLimit          20
#define HANDLE_TYPE_CODESET                 21


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
#define SERVICE_TYPE_MCU_GPU    31
#define SERVICE_TYPE_I2C_LCD    32
#define SERVICE_TYPE_CDC_CSN    33
#define SERVICE_TYPE_PxiFS0     34
#define SERVICE_TYPE_PxiFS1     35
#define SERVICE_TYPE_PxiFS2     36
#define SERVICE_TYPE_PxiFS3     37
#define SERVICE_TYPE_PxiAM9     38
#define SERVICE_TYPE_MIC_U      39
#define SERVICE_TYPE_PxiPS9     40
#define SERVICE_TYPE_mcuPLS     41
#define SERVICE_TYPE_csndSND    42
#define SERVICE_TYPE_aci        43
#define SERVICE_TYPE_acu        44
#define SERVICE_TYPE_nims       45
#define SERVICE_TYPE_ssl_c      46
#define SERVICE_TYPE_http_c     47
#define SERVICE_TYPE_frd_a      48
#define SERVICE_TYPE_soc_u      49
#define SERVICE_TYPE_am_app     50
#define SERVICE_TYPE_nim_aoc    51
#define SERVICE_TYPE_apt_a      52
#define SERVICE_TYPE_y2r_u      53
#define SERVICE_TYPE_pix_dev    54
#define SERVICE_TYPE_IR_RST     55
#define SERVICE_TYPE_PTM_PLAY   56
#define SERVICE_TYPE_CAM_U      57
#define SERVICE_TYPE_LDR_RO     58

#define HANDLE_SUBEVENT_USER          0
#define HANDLE_SUBEVENT_APTMENUEVENT  1
#define HANDLE_SUBEVENT_APTPAUSEEVENT 2
#define HANDLE_SUBEVENT_CSNDEVENT 3
#define HANDLE_SUBEVENT_CECDEVENT 4

#define HANDLE_MUTEX_APTMUTEX 2

#define LOCK_TYPE_ONESHOT 0
#define LOCK_TYPE_STICKY  1
#define LOCK_TYPE_PULSE   2
#define LOCK_TYPE_MAX     2

#define MEM_TYPE_GSP_0             0
#define MEM_TYPE_HID_0             1
#define MEM_TYPE_APT_SHARED_FONT   2
#define MEM_TYPE_ALLOC             3
#define MEM_TYPE_HID_SPVR_0        4
#define MEM_TYPE_APT_S_SHARED_FONT 5
#define MEM_TYPE_CSND              6


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

//main.c
#ifdef MODULE_SUPPORT
u32 overdrivnum;
char** overdrivnames;
#endif

// handles.c
handleinfo* handle_Get(u32 handle);
u32 handle_New(u32 type, uintptr_t subtype);
int handle_free(u32 handle);
void handle_Init();

#ifdef MODULE_SUPPORT
u32 *curprocesshandlelist;
#endif

u32 g_process_handle;

//handles.h
u32 wrapWaitSynchronizationN(u32 nanoseconds1, u32 handles_ptr, u32 handles_count, u32 wait_all, u32 nanoseconds2, u32 out);
u32 handle_wrapWaitSynchronization1(u32 handle);

// services/srv.c
u32 services_SyncRequest(handleinfo* h, bool *locked);
u32 services_WaitSynchronization(handleinfo* h, bool *locked);

// svc/syn.c
u32 mutex_WaitSynchronization(handleinfo* h, bool *locked);
u32 mutex_SyncRequest(handleinfo* h, bool *locked);

// svc/ports.c
u32 port_SyncRequest(handleinfo* h, bool *locked);

//syscalls/events.c
u32 event_WaitSynchronization(handleinfo* h, bool *locked);

u32 file_SyncRequest(handleinfo* h, bool *locked);
u32 dir_SyncRequest(handleinfo* h, bool *locked);
u32 dir_CloseHandle(ARMul_State *state, u32 handle);

//arm11/threads.c
u32 thread_SyncRequest(handleinfo* h, bool *locked);
u32 thread_CloseHandle(ARMul_State *state, u32 handle);
u32 thread_WaitSynchronization(handleinfo* h, bool *locked);

// svc/syn.c
u32 semaphore_WaitSynchronization(handleinfo* h, bool *locked);
u32 semaphore_SyncRequest(handleinfo* h, bool *locked);

//services/file.c
u32 file_SyncRequest(handleinfo* h, bool *locked);
u32 file_CloseHandle(ARMul_State *state, u32 handle);
u32 file_WaitSynchronization(handleinfo* h, bool *locked);

u32 nop_SyncRequest(handleinfo* h, bool *locked);

u32 svc_unmountWaitSynchronization(handleinfo* h, bool *locked);
u32 svc_unmountSyncRequest(handleinfo* h, bool *locked);

u32 svc_serverSyncRequest(handleinfo* h, bool *locked);
u32 svc_serverWaitSynchronization(handleinfo* h, bool *locked);

u32 timer_WaitSynchronization(handleinfo* h, bool *locked);

u32 process_WaitSynchronization(handleinfo* h, bool *locked);

static struct {
    const char* name;
    u32(*fnSyncRequest)(handleinfo* h, bool *locked);
    u32(*fnCloseHandle)(ARMul_State *state, u32 handle);
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
        &services_WaitSynchronization
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
        &process_WaitSynchronization
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
        &file_CloseHandle,
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
        &nop_SyncRequest,
        NULL,
        NULL
    },
    {
        "session",
        NULL,
        NULL,
        NULL
    },
    {
        "dir",
        &dir_SyncRequest,
        &dir_CloseHandle,
        NULL
    },
    {
        "socket",
        NULL,
        NULL,
        NULL
    },
    {
        "httpcont",
        NULL,
        NULL,
        NULL
    },
    {
        "UNMOUNTED",
        &svc_unmountSyncRequest,
        NULL,
        &svc_unmountWaitSynchronization
    },
    {
        "SERVICE_SERVER",
        &svc_serverSyncRequest,
        NULL,
        &svc_serverWaitSynchronization
    },
    {
        "timer",
        NULL,
        NULL,
        &timer_WaitSynchronization
    },
    {
        "KResourceLimit",
        NULL,
        NULL,
        NULL
    },
    {
        "CODESET",
        NULL,
        NULL,
        NULL
    }
};