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

extern ARMul_State s;
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
u32 cdc_csn_SyncRequest();
u32 PxiFS0_SyncRequest();
u32 PxiFS1_SyncRequest();
u32 PxiFS2_SyncRequest();
u32 PxiFS3_SyncRequest();
u32 Pxiam9_SyncRequest();
u32 mic_u_SyncRequest();
u32 Pxips9_SyncRequest();
u32 mcuPLS_SyncRequest();
u32 csnd_SND_SyncRequest();
u32 ac_i_SyncRequest();
u32 ac_u_SyncRequest();
u32 nim_s_SyncRequest();
u32 ssl_c_SyncRequest();
u32 http_c_SyncRequest();
u32 frd_a_SyncRequest();
u32 soc_u_SyncRequest();
u32 am_app_SyncRequest();
u32 nim_aoc_SyncRequest();
u32 apt_a_SyncRequest();
u32 y2r_u_SyncRequest();
u32 pix_dev_SyncRequest();
u32 ir_rst_SyncRequest();
u32 ptm_play_SyncRequest();
u32 cam_u_SyncRequest();
u32 ldr_ro_SyncRequest();

static size_t _strnlen(const char* p, size_t n)
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
    },
    {
        "cdc:CSN",
        SERVICE_TYPE_CDC_CSN,
        0,
        &cdc_csn_SyncRequest
    },
    {
        "PxiFS0",
        SERVICE_TYPE_PxiFS0,
        0,
        &PxiFS0_SyncRequest
    },
    {
        "PxiFS1",
        SERVICE_TYPE_PxiFS1,
        0,
        &PxiFS1_SyncRequest
    },
    {
        "PxiFSB",
        SERVICE_TYPE_PxiFS2,
        0,
        &PxiFS2_SyncRequest
    },
    {
        "PxiFSR",
        SERVICE_TYPE_PxiFS3,
        0,
        &PxiFS3_SyncRequest
    },
    {
        "pxi:ps9",
        SERVICE_TYPE_PxiPS9,
        0,
        &Pxips9_SyncRequest
    },
    {
        "pxi:am9",
        SERVICE_TYPE_PxiAM9,
        0,
        &Pxiam9_SyncRequest
    },
    {
        "mic:u",
        SERVICE_TYPE_MIC_U,
        0,
        &mic_u_SyncRequest
    },
    {
        "mcu::PLS",
        SERVICE_TYPE_mcuPLS,
        0,
        &mcuPLS_SyncRequest
    },
    {
        "csnd:SND",
        SERVICE_TYPE_csndSND,
        0,
        &csnd_SND_SyncRequest
    },
    {
        "ac:i",
        SERVICE_TYPE_aci,
        0,
        &ac_i_SyncRequest
    },
    {
        "ac:u",
        SERVICE_TYPE_acu,
        0,
        &ac_u_SyncRequest
    },
    {
        "nim:s",
        SERVICE_TYPE_nims,
        0,
        &nim_s_SyncRequest
    },
    {
        "ssl:C",
        SERVICE_TYPE_ssl_c,
        0,
        &ssl_c_SyncRequest
    },
    {
        "http:C",
        SERVICE_TYPE_http_c,
        0,
        &http_c_SyncRequest
    },
    {
        "frd:a",
        SERVICE_TYPE_frd_a,
        0,
        &frd_a_SyncRequest
    },
    {
        "soc:U",
        SERVICE_TYPE_soc_u,
        0,
        &soc_u_SyncRequest
    },
    {
        "am:app",
        SERVICE_TYPE_soc_u,
        0,
        &am_app_SyncRequest
    },
    {
        "nim:aoc",
        SERVICE_TYPE_nim_aoc,
        0,
        &nim_aoc_SyncRequest
    },
    {
        "APT:A",
        SERVICE_TYPE_apt_a,
        0,
        &apt_a_SyncRequest
    },
    {
        "y2r:u",
        SERVICE_TYPE_y2r_u,
        0,
        &y2r_u_SyncRequest
    },
    {
        "pxi:dev",
        SERVICE_TYPE_pix_dev,
        0,
        &pix_dev_SyncRequest
    },
    {
        "ir:rst",
        SERVICE_TYPE_IR_RST,
        0,
        &ir_rst_SyncRequest
    },
    {
        "ptm:play",
        SERVICE_TYPE_PTM_PLAY,
        0,
        &ptm_play_SyncRequest
    },
    {
        "cam:u",
        SERVICE_TYPE_CAM_U,
        0,
        &cam_u_SyncRequest
    },
    {
        "ldr:ro",
        SERVICE_TYPE_LDR_RO,
        0,
        &ldr_ro_SyncRequest
    }
};


#define NUM_HANDLE_TYPES ARRAY_SIZE(handle_types)


#define MAX_ownservice 0x100
typedef struct {
    char* name;
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

    if (h->subtype == SERVICE_DIRECT) {
        if (h->misc[0] & HANDLE_SERV_STAT_ACKING) {
            mem_Write(h->misc_ptr[0], arm11_ServiceBufferAddress() + 0x80, 0x80); //todo 
            h->misc[0] &= ~(HANDLE_SERV_STAT_ACKING | HANDLE_SERV_STAT_SYNCING);
            *locked = false;
            return 0;
        } else {
            IPC_debugprint(arm11_ServiceBufferAddress() + 0x80);
            if (!(h->misc[0] & HANDLE_SERV_STAT_SYNCING)) 
                mem_Read(h->misc_ptr[0], arm11_ServiceBufferAddress() + 0x80, 0x80);
            h->misc[0] |= HANDLE_SERV_STAT_SYNCING | HANDLE_SERV_STAT_AKTIVE_REQ;
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

    h->misc[0] = 0x10; //there are 0x10 events we know 2 non of them are used here
    h->misc[1] = 0x10;

    return 0;
}

void srv_InitGlobal()
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
    u32 cid = mem_Read32(arm11_ServiceBufferAddress() + 0x80);

    // Read command-id.
    switch(cid) {

    case 0x10002:
        DEBUG("srv_Initialize\n");

        // XXX: check +4, flags?
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        PAUSE();
        return 0;

    case 0x20000:
        DEBUG("srv_GetProcSemaphore\n");

        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        mem_Write32(arm11_ServiceBufferAddress() + 0x88, 0); //done in sm 4.4
        mem_Write32(arm11_ServiceBufferAddress() + 0x8C, eventhandle);
        return 0;

        char names[9];
    case 0x000400C0:
        DEBUG("srv_UnRegisterService --todo--\n");

        // Read rest of command header
        mem_Read((u8*)&req, arm11_ServiceBufferAddress() + 0x84, sizeof(req));

        memcpy(names, req.name, 8);
        names[8] = '\0';

        DEBUG("name=%s, namelen=%u\n", names, req.name_len);

        return 0;

    case 0x00030100:
        DEBUG("srv_registerService\n");

        // Read rest of command header
        mem_Read((u8*)&req, arm11_ServiceBufferAddress() + 0x84, sizeof(req));

        memcpy(names, req.name, 8);
        names[8] = '\0';

        DEBUG("name=%s, namelen=%u, unk=0x%x\n", names, req.name_len,
              req.unk2);


        ownservice[ownservice_num].name = calloc(9,sizeof(u8));
        memcpy(ownservice[ownservice_num].name, req.name, sizeof(req.name));

        ownservice[ownservice_num].handle = handle_New(HANDLE_TYPE_SERVICE_UNMOUNTED, SERVICE_DIRECT);

        handleinfo* oldhi = handle_Get(ownservice[ownservice_num].handle);

        if (oldhi == NULL) {
            ERROR("getting handle.\n");
            return 0x0;
        }

        oldhi->misc[0] = 0; //numb of connected todo more than one

        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        mem_Write32(arm11_ServiceBufferAddress() + 0x8C, ownservice[ownservice_num].handle); //return handle
        ownservice_num++;
        return 0;

    case 0x50100:
        DEBUG("srv_GetServiceHandle\n");

        // Read rest of command header
        mem_Read((u8*)&req, arm11_ServiceBufferAddress() + 0x84, sizeof(req));

        memcpy(names, req.name, 8);
        names[8] = '\0';

        DEBUG("name=%s, namelen=%u, unk=0x%x\n", names, req.name_len,
              req.unk2);
        PAUSE();

#ifdef MODULE_SUPPORT
        bool overdr = false;
        for (u32 i = 0; i < overdrivnum; i++) {
            if (memcmp(req.name, *(overdrivnames + i), _strnlen(*(overdrivnames + i), 8)) == 0)overdr = true;
        }

        if (!overdr) {
            for (u32 i = 0; i < ownservice_num; i++) {
                if (memcmp(req.name, ownservice[i].name, _strnlen(ownservice[i].name, 8)) == 0) {

                    u32 newhand = handle_New(HANDLE_TYPE_SERVICE, SERVICE_DIRECT);

                    handleinfo* newhi = handle_Get(newhand);
                    if (newhi == NULL) {
                        ERROR("getting handle.\n");
                        return 0x0;
                    }
                    newhi->misc[0] = HANDLE_SERV_STAT_TAKEN | HANDLE_SERV_STAT_INITING; //init

                    newhi->misc_ptr[0] = malloc(0x200);

                    handleinfo* oldhi = handle_Get(ownservice[i].handle);

                    if (oldhi == NULL) {
                        ERROR("getting handle.\n");
                        return 0x0;
                    }

                    oldhi->misc[0]++;

                    oldhi->misc_ptr[1] = 
                    oldhi->misc[1] = newhand;


                    // Write handle_out.
                    mem_Write32(arm11_ServiceBufferAddress() + 0x8C, newhand);

                    // Write result.
                    mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0);

                    wrapWaitSynchronizationN(0xFFFFFFFF, arm11_ServiceBufferAddress() + 0x8C, 1, false, 0xFFFFFFFF, 0); //workaround todo fixme

                    s.NumInstrsToExecute = 0; //this will make it wait a round so the server has time to take the service
                    return 0;
                }
            }
        }
#endif

        u32 i;
        for(i=0; i<ARRAY_SIZE(services); i++) {
            // Find service in list.
            if(memcmp(req.name, services[i].name, _strnlen(services[i].name, 8)) == 0) {


                // Write result.
                mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0);

                // Write handle_out.
                mem_Write32(arm11_ServiceBufferAddress() + 0x8C, services[i].handle);

                return 0;
            }
        }

        ERROR("Unimplemented service: %s\n", req.name);
        arm11_Dump(); //HANDLE_SERV_STAT_INITING
        //exit(1);
        mem_Write32(arm11_ServiceBufferAddress() + 0x8C, 0xDEADBABE);
        return 0;

    case 0x00080100:
        DEBUG("srv_GetHandle\n");

        // Read rest of command header
        mem_Read((u8*)&req, arm11_ServiceBufferAddress() + 0x84, sizeof(req));

        memcpy(names, req.name, 8);
        names[8] = '\0';

        DEBUG("name=%s, namelen=%u, unk=0x%x\n", names, req.name_len,
              req.unk2);
        PAUSE();

        // Write result.
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0);

        mem_Write32(arm11_ServiceBufferAddress() + 0x8C, 0x1234);

        ERROR("Unimplemented handle: %s\n", req.name);
        arm11_Dump();
        return 0;


    case 0x90040: // EnableNotificationType
        DEBUG("srv_EnableNotificationType\n");

        u32 type = mem_Read32(arm11_ServiceBufferAddress() + 0x84);
        DEBUG("STUBBED, type=%x\n", type);

        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0);
        return 0;

    case 0xa0040: // DisableNotificationType
        DEBUG("srv_DisableNotificationType\n");

        type = mem_Read32(arm11_ServiceBufferAddress() + 0x84);
        DEBUG("STUBBED, type=%x\n", type);

        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;

    case 0xB0000: // GetNotificationType
        DEBUG("srv_GetNotificationType\n");
        //mem_Dbugdump();
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //worked
        mem_Write32(arm11_ServiceBufferAddress() + 0x88, 0); //type
        return 0;

    default:
        ERROR("Unimplemented command %08x in \"srv:\"\n", cid);
        arm11_Dump();
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0xFFFFFFFF); //worked
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

    if (replyTarget) //respond
    {
        IPC_debugprint(arm11_ServiceBufferAddress() + 0x80);
        handleinfo* h2 = handle_Get(replyTarget);
        if (h2 == NULL) {
            ERROR("handle not there");
        }

        eventhandle = h2->misc[0];
        h2 = handle_Get(eventhandle);
        if (h2 == NULL) {
            ERROR("handle not there");
        }
        if (h2->misc[0] & HANDLE_SERV_STAT_AKTIVE_REQ)
        {
            h2->misc[0] &= ~(HANDLE_SERV_STAT_AKTIVE_REQ);
            if (h2->misc[0] & HANDLE_SERV_STAT_SYNCING) {
                mem_Read(h2->misc_ptr[0], arm11_ServiceBufferAddress() + 0x80, 0x80); //todo 
                h2->misc[0] |= HANDLE_SERV_STAT_ACKING;
            }
        }
        else
        {
            ERROR("failded to repl\n");
        }
    }
#ifdef MODULE_SUPPORT
    for (u32 i = 0; i < handleCount; i++) {
        DEBUG("%08x\n", mem_Read32(handles+i*4));

        handleinfo* h = handle_Get(mem_Read32(handles + i * 4));
        if (h == NULL) {
            ERROR("getting handle.\n");
            continue;
        }

        handleinfo* h2 = handle_Get(replyTarget);
        if (h2 == NULL) {
            continue;
        }


        if (h->type == HANDLE_TYPE_SERVICE) {
            h->misc[0] |= HANDLE_SERV_STAT_WAITING;
            h->misc[1] = curprocesshandle;
            h->misc[2] = threads_GetCurrentThreadHandle();
        }

        h->locked = true;
    }
    wrapWaitSynchronizationN(0xFFFFFFFF,handles,handleCount,false,0xFFFFFFFF,0);
#else

    for (u32 i = 0; i < handleCount; i++) {
        DEBUG("%08x\n", mem_Read32(handles + i * 4));
    }
#endif
    /*wrapWaitSynchronizationN(0xFFFFFFFF, handles, handleCount, 0, 0xFFFFFFFF,0);



    //feed module data here
    switch (times) {
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
        break;
    case 7:
        RESP(0, 0x00130042);
        RESP(1, 0x0);
        RESP(2, 0x0);
        RESP(3, handle_New(HANDLE_TYPE_EVENT, 0));
        break;
    default:
        RESP(0, 0x000C0000);
        break;
    }*/

    //times++;
    //RESP(0, 0x00010800);

    //feed end


    return 0;
}
u32 svcAcceptSession()
{
    u32 session = arm11_R(0);
    u32 old_port = arm11_R(1);

    handleinfo* newhi = handle_Get(old_port);
    if (newhi == NULL) {
        ERROR("getting handle.\n");
        return 0x0;
    }


    u32 newhand = handle_New(HANDLE_TYPE_SERVICE_SERVER, SERVICE_DIRECT);

    handleinfo* newhi2 = handle_Get(newhand);
    if (newhi2 == NULL) {
        ERROR("getting handle.\n");
        return 0x0;
    }
    newhi2->misc[0] = newhi->misc[1];

    //unlock
    handleinfo* anewhi = handle_Get(newhi->misc[1]);
    if (anewhi == NULL) {
        ERROR("getting handle.\n");
        return 0x0;
    }
    anewhi->misc[0] |= HANDLE_SERV_STAT_OPENING;
    DEBUG("AcceptSession %08x %08x\n", session, newhi->misc[1]);



    arm11_SetR(1, newhand);
    return 0;
}
u32 services_WaitSynchronization(handleinfo* h, bool *locked)
{
    if (h->misc[0] & HANDLE_SERV_STAT_OPENING)
    {
        *locked = false;
        h->misc[0] &= ~(HANDLE_SERV_STAT_OPENING | HANDLE_SERV_STAT_INITING);
        return 0;
    }
    if (h->misc[0] & HANDLE_SERV_STAT_SYNCING) {
        mem_Write(h->misc_ptr[0], arm11_ServiceBufferAddress() + 0x80, 0x80);
        *locked = false;
        h->misc[0] &= ~HANDLE_SERV_STAT_SYNCING;
    } else*locked = true;
    return 0;
}
u32 svc_unmountWaitSynchronization(handleinfo* h, bool *locked)
{
    if (h->misc[0]) {
        h->misc[0]--;
        *locked = 0;
        return 0;
    }
    else {
        *locked = 1;
        return 0;
    }
}
u32 svc_unmountSyncRequest(handleinfo* h, bool *locked)
{
    if (h->misc[0]) {
        h->misc[0]--;
        *locked = 0;
        return 0;
    }
    else {
        *locked = 1;
        return 0;
    }
}
u32 svc_serverSyncRequest(handleinfo* h, bool *locked)
{
    handleinfo* hi = handle_Get(h->misc[0]);
    if (hi == NULL) {
        *locked = 1;
        return 0;
    }
    if (hi->misc[0] & HANDLE_SERV_STAT_SYNCING) {
        mem_Write(hi->misc_ptr[0], arm11_ServiceBufferAddress() + 0x80, 0x80); //todo 
        *locked = 0;
        return 0;
    }
    else {
        *locked = 1;
        return 0;
    }
}
u32 svc_serverWaitSynchronization(handleinfo* h, bool *locked)
{
    handleinfo* hi = handle_Get(h->misc[0]);
    if (hi == NULL) {
        *locked = 1;
        return 0;
    }
    if (hi->misc[0] & HANDLE_SERV_STAT_SYNCING) {
        mem_Write(hi->misc_ptr[0], arm11_ServiceBufferAddress() + 0x80, 0x80); //todo 
        *locked = 0;
        return 0;
    }
    else {
        *locked = 1;
        return 0;
    }
}
