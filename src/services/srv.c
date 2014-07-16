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
#include "handles.h"
#include "mem.h"

#include "service_macros.h"


u32 apt_u_SyncRequest();
u32 gsp_gpu_SyncRequest();
u32 hid_user_SyncRequest();
u32 fs_user_SyncRequest();
u32 am_u_SyncRequest();
u32 ns_s_SyncRequest();
u32 ndm_u_SyncRequest();
u32 cfg_u_SyncRequest();
u32 ptm_u_SyncRequest();
u32 frd_u_SyncRequest();
u32 ir_u_SyncRequest();
u32 dsp_dsp_SyncRequest();
u32 cecd_u_SyncRequest();
u32 boss_u_SyncRequest();
u32 ptm_s_SyncRequest();
u32 pdn_d_SyncRequest();
u32 cdc_DSP_SyncRequest();
u32 fs_ldr_SyncRequest();
u32 PxiPM_SyncRequest();
u32 fs_REG_SyncRequest();
u32 cfg_i_SyncRequest();
u32 cfg_nor_SyncRequest();
u32 hid_SPVR_SyncRequest();
u32 am_sys_SyncRequest();
u32 boss_P_SyncRequest();
u32 ps_ps_SyncRequest();
u32 cfg_s_SyncRequest();
u32 apt_s_SyncRequest();
u32 pdn_g_SyncRequest();
u32 mcu_GPU_SyncRequest();
u32 i2c_LCD_SyncRequest();

#define CPUsvcbuffer 0xFFFF0000


#ifndef _WIN32
static size_t strnlen(const char* p, size_t n)
{
    const char* q = p;

    if(n == 0)
        return 0;

    while(*q != '\0') {
        q++;
        n--;

        if(n == 0)
            break;
    }

    return q-p;
}
#endif

static struct {
    const char* name;
    u32 subtype;
    u32 handle;
    u32 (*fnSyncRequest)();
} services[] = {
    //HLE Services are declared here.
    {
        "APT:U",
        SERVICE_TYPE_APT_U,
        0,
        &apt_u_SyncRequest
    },
    {
        "APT:S",
        SERVICE_TYPE_APT_S,
        0,
        &apt_s_SyncRequest
    },
    {
        "gsp::Gpu",
        SERVICE_TYPE_GSP_GPU,
        0,
        &gsp_gpu_SyncRequest
    },
    {
        "hid:USER",
        SERVICE_TYPE_HID_USER,
        0,
        &hid_user_SyncRequest
    },
    {
        "fs:USER",
        SERVICE_TYPE_FS_USER,
        0,
        &fs_user_SyncRequest
    },
    {
        "am:u",
        SERVICE_TYPE_AM_USER,
        0,
        &am_u_SyncRequest
    },
    {
        "ns:s",
        SERVICE_TYPE_NINSHELL_S,
        0,
        &ns_s_SyncRequest
    },
    {
        "ndm:u",
        SERVICE_TYPE_NDM_USER,
        0,
        &ndm_u_SyncRequest
    },
    {
        "cfg:u",
        SERVICE_TYPE_CFG_USER,
        0,
        &cfg_u_SyncRequest
    },
    {
        "ptm:u",
        SERVICE_TYPE_PTM_USER,
        0,
        &ptm_u_SyncRequest
    },
    {
        "frd:u",
        SERVICE_TYPE_FRD_USER,
        0,
        &frd_u_SyncRequest
    },
    {
        "ir:USER",
        SERVICE_TYPE_IR_USER,
        0,
        &ir_u_SyncRequest
    },
    {
        "dsp::DSP",
        SERVICE_TYPE_DSP_DSP,
        0,
        &dsp_dsp_SyncRequest
    },
    {
        "cecd:u",
        SERVICE_TYPE_CECD_U,
        0,
        &cecd_u_SyncRequest
    },
    {
        "boss:U",
        SERVICE_TYPE_BOSS_U,
        0,
        &boss_u_SyncRequest
    },
    {
        "ptm:s",
        SERVICE_TYPE_PTM_SYSTEM,
        0,
        &ptm_s_SyncRequest
    },
    {
        "pdn:d",
        SERVICE_TYPE_PDN_D,
        0,
        &pdn_d_SyncRequest
    },
    {
        "cdc:DSP",
        SERVICE_TYPE_cdc_DSP,
        0,
        &cdc_DSP_SyncRequest
    },
    {
        "fs:LDR",
        SERVICE_TYPE_fs_ldr,
        0,
        &fs_ldr_SyncRequest
    },
    {
        "PxiPM",
        SERVICE_TYPE_PxiPM,
        0,
        &PxiPM_SyncRequest
    },
    {
        "fs:REG",
        SERVICE_TYPE_fs_REG,
        0,
        &fs_REG_SyncRequest
    },
    {
        "cfg:i",
        SERVICE_TYPE_cfg_i,
        0,
        &cfg_i_SyncRequest
    },
    {
        "cfg:nor",
        SERVICE_TYPE_cfg_nor,
        0,
        &cfg_nor_SyncRequest
    },
    {
        "hid:SPVR",
        SERVICE_TYPE_hid_SPVR,
        0,
        &hid_SPVR_SyncRequest
    },
    {
        "am:sys",
        SERVICE_TYPE_am_sys,
        0,
        &am_sys_SyncRequest
    },
    {
        "boss:P",
        SERVICE_TYPE_boss_P,
        0,
        &boss_P_SyncRequest
    },
    {
        "ps:ps",
        SERVICE_TYPE_ps_ps,
        0,
        &ps_ps_SyncRequest
    },
    {
        "cfg:s",
        SERVICE_TYPE_CFG_S,
        0,
        &cfg_s_SyncRequest
    },
    {
        "pdn:g",
        SERVICE_TYPE_PDN_G,
        0,
        &pdn_g_SyncRequest
    },
    {
        "mcu::GPU",
        SERVICE_TYPE_MCU_GPU,
        0,
        &mcu_GPU_SyncRequest
    },
    {
        "i2c::LCD",
        SERVICE_TYPE_I2C_LCD,
        0,
        &i2c_LCD_SyncRequest
    }
};


#define NUM_HANDLE_TYPES ARRAY_SIZE(handle_types)


#define MAX_ownservice 0x100
typedef struct {
    const char* name;
    u32 pid;
    u32 handle;
    s32 threadwaitlist;
} S_ownservice;

static S_ownservice ownservice[MAX_ownservice];
static u32 ownservice_num;




u32 services_SyncRequest(handleinfo* h, bool *locked)
{
    u32 i;

    // Lookup which requested service in table.
    for(i=0; i<ARRAY_SIZE(services); i++) {
        if(services[i].subtype == h->subtype)
            return services[i].fnSyncRequest();
    }

    if (h->subtype == SERVICE_DIRECT)
    {
        if (h->misc[0] & HANDLE_SERV_STAT_ACKING)
        {
            mem_Write(h->misc_ptr[0], CPUsvcbuffer + 0x80, 0x200);
            h->misc[0] &= ~(HANDLE_SERV_STAT_ACKING | HANDLE_SERV_STAT_SYNCING);
            *locked = false;
            return 0;
        }
        else
        {
            if (!(h->misc[0] & HANDLE_SERV_STAT_SYNCING)) mem_Read(h->misc_ptr[0], CPUsvcbuffer + 0x80, 0x200);
            h->misc[0] |= HANDLE_SERV_STAT_SYNCING;
            *locked = true;
            return 0;
        }
    }

    ERROR("invalid handle.\n");
    arm11_Dump();
    PAUSE();
    return 0;
}

u32 eventhandle;

u32 srv_InitHandle()
{
    // Create a handle for srv: port.
    arm11_SetR(1, handle_New(HANDLE_TYPE_PORT, PORT_TYPE_SRV));
    eventhandle = handle_New(HANDLE_TYPE_SEMAPHORE, 0);


    handleinfo* h = handle_Get(eventhandle);
    if (h == NULL) {
        DEBUG("failed to get newly created semaphore\n");
        PAUSE();
        return -1;
    }

    h->locked = true;

    h->misc[0] = 0; //there are 0x10 events we know 2 non of them are used here
    h->misc[1] = 0x10;

    return 0;
}

u32 srv_InitGlobal()
{
    // Create handles for all services.
    u32 i;
    for(i=0; i<ARRAY_SIZE(services); i++) {
        services[i].handle = handle_New(HANDLE_TYPE_SERVICE, services[i].subtype);
    }
}

struct {
    char name[8];
    uint32_t name_len;
    uint32_t unk2;
} req;

u32 srv_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);

    // Read command-id.
    switch(cid) {

    case 0x10002:
        DEBUG("srv_Initialize\n");

        // XXX: check +4, flags?
        PAUSE();
        return 0;

    case 0x20000:
        DEBUG("srv_EnableNotification\n");

        mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
        mem_Write32(CPUsvcbuffer + 0x88, 0); //done in sm 4.4
        mem_Write32(CPUsvcbuffer + 0x8C, eventhandle);
        return 0;

        char names[9];
    case 0x00030100:
        DEBUG("srv_RegisterService\n");

        // Read rest of command header
        mem_Read((u8*)&req, 0xFFFF0084, sizeof(req));

        memcpy(names, req.name, 8);
        names[8] = '\0';

        DEBUG("name=%s, namelen=%u, unk=0x%x\n", names, req.name_len,
            req.unk2);


        ownservice[ownservice_num].name = malloc(9);
        memcpy(ownservice[ownservice_num].name,req.name , 9);

        ownservice[ownservice_num].handle = handle_New(HANDLE_TYPE_SERVICE, SERVICE_DIRECT);

        handleinfo* hi = handle_Get(ownservice[ownservice_num].handle);
        if (hi == NULL) {
            ERROR("getting handle.\n");
            return 0x0;
        }
        hi->misc[0] = HANDLE_SERV_STAT_TAKEN; //init

        hi->misc_ptr[0] = malloc(0x200);

        mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
        mem_Write32(CPUsvcbuffer + 0x8C, ownservice[ownservice_num].handle); //return handle
        ownservice_num++;
        return 0;

    case 0x50100:
        DEBUG("srv_GetServiceHandle\n");

        // Read rest of command header
        mem_Read((u8*) &req, 0xFFFF0084, sizeof(req));

        memcpy(names, req.name, 8);
        names[8] = '\0';

        DEBUG("name=%s, namelen=%u, unk=0x%x\n", names, req.name_len,
              req.unk2);
        PAUSE();

        u32 i;

        bool overdr = false;
        for (int i = 0; i < overdrivnum; i++)
        {
            if (memcmp(req.name, *(overdrivnames + i), strnlen(*(overdrivnames + i), 8)) == 0)overdr = true;
        }
        if (!overdr)
        {
            for (int i = 0; i < ownservice_num; i++)
            {
                if (memcmp(req.name, ownservice[i].name, strnlen(ownservice[i].name, 8)) == 0) {

                    // Write result.
                    mem_Write32(0xFFFF0084, 0);

                    // Write handle_out.
                    mem_Write32(0xFFFF008C, ownservice[i].handle);

                    return 0;
                }
            }
        }
        for(i=0; i<ARRAY_SIZE(services); i++) {
            // Find service in list.
            if(memcmp(req.name, services[i].name, strnlen(services[i].name, 8)) == 0) {

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

    case 0x90040: // EnableNotificationType
        DEBUG("srv_EnableNotificationType\n");

        u32 type = mem_Read32(0xFFFF0084);
        DEBUG("STUBBED, type=%x\n", type);

        mem_Write32(CPUsvcbuffer + 0x84, 0);
        return 0;

    case 0xB0000: // GetNotificationType
        DEBUG("srv_GetNotificationType\n");
        //mem_Dbugdump();
        mem_Write32(CPUsvcbuffer + 0x84, 0); //worked
        mem_Write32(CPUsvcbuffer + 0x88, 0xFFFF); //type 
        return 0;
        
    default:
        ERROR("Unimplemented command %08x in \"srv:\"\n", cid);
        arm11_Dump();
        mem_Write32(CPUsvcbuffer + 0x84, 0xFFFFFFFF); //worked
        return 0;
        //exit(1);
    }

    return 0;
}

u32 times = 0;
u32 svcReplyAndReceive()
{

    s32 index = arm11_R(0);
    u32 handles = arm11_R(1);
    u32 handleCount = arm11_R(2);
    u32 replyTarget = arm11_R(3);
    DEBUG("svcReplyAndReceive %08x %08x %08x %08x\n", index, handles, handleCount, replyTarget);
#ifdef modulesupport
    for (u32 i = 0; i < handleCount; i++)
    {
        DEBUG("%08x\n", mem_Read32(handles+i*4));

        handleinfo* h = handle_Get(eventhandle);
        if (h == NULL) {
            PAUSE();
            return -1;
        }

        if (h->type == HANDLE_TYPE_SERVICE)
        {
            h->misc[0] |= HANDLE_SERV_STAT_WAITING;
            h->misc[1] = curprocesshandle;
            h->misc[2] = threads_GetCurrentThreadHandle();
        }
    }
#endif
    for (u32 i = 0; i < handleCount; i++)
    {
        DEBUG("%08x\n", mem_Read32(handles + i * 4));
    }
    wrapWaitSynchronizationN(0xFFFFFFFF, handles, handleCount, 0, 0xFFFFFFFF,0);



    //feed module data here
    switch (times)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        RESP(0, 0x00160042);
        RESP(1, 0x0);
        RESP(2, 0x0);
        RESP(3, 0x12345);
    case 7:
        RESP(0, 0x00130042);
        RESP(1, 0x0);
        RESP(2, 0x0);
        RESP(3, handle_New(HANDLE_TYPE_EVENT, 0));
        break;
    default:
        RESP(0, 0x000C0000);
    }

    //feed end

    times++;

    return 1;
}
u32 svcAcceptSession()
{
    s32 session = arm11_R(0);
    u32 port = arm11_R(1);
    arm11_SetR(1, handle_New(HANDLE_TYPE_SESSION, port));
    DEBUG("AcceptSession %08x %08x\n", session, port);
    return 0;
}
u32 services_WaitSynchronization(handleinfo* h, bool *locked)
{
    if (h->misc[0] & HANDLE_SERV_STAT_SYNCING)
    {
        mem_Write(h->misc_ptr[0], CPUsvcbuffer + 0x80, 0x200);
        *locked = false;
    }
    else*locked = true;
    return 0;
}