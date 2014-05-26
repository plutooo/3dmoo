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
#include "svc.h"

#include "mem.h"

#define MAX_NUM_HANDLES 0x1000
#define HANDLES_BASE    0xDEADBABE

#define exitonerror 1

static handleinfo handles[MAX_NUM_HANDLES];
static u32 handles_num;


#define NUM_HANDLE_TYPES ARRAY_SIZE(handle_types)


u32 handle_New(u32 type, u32 subtype)
{
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

handleinfo* handle_Get(u32 handle)
{
    u32 idx = handle - HANDLES_BASE;

    if (idx < handles_num) {
        if (handles[idx].type == HANDLE_TYPE_REDIR)
            return handle_Get(handles[idx].subtype);
        else return &handles[idx];
    }
    return NULL;
}


/* Generic SVC implementations. */
u32 svcSendSyncRequest()
{
    u32 handle = arm11_R(0);
    handleinfo* hi = handle_Get(handle);

    if(hi == NULL) {
        ERROR("handle %08x not found.\n", handle);
        PAUSE();
#ifdef exitonerror
        exit(1);
#else
        return 0;
#endif
    }

    if(hi->type >= NUM_HANDLE_TYPES) {
        // This should never happen.
        ERROR("handle %08x has non-defined type.\n", handle);
        PAUSE();
        exit(1);
    }
    u32 temp;
    bool locked = false;
    // Lookup actual callback in table.
    if (handle_types[hi->type].fnSyncRequest != NULL) {
        temp = handle_types[hi->type].fnSyncRequest(hi, &locked);
        if (locked) {
            u32* handelist = malloc(4);
            *handelist = handle;
            lockcpu(handelist, 1, 1);
        }
        return temp;
    } else {
        ERROR("svcSyncRequest undefined for handle-type \"%s\".\n",
              handle_types[hi->type].name);
        PAUSE();
        exit(1);
    }
}

u32 svcCloseHandle(ARMul_State *state)
{
    u32 handle = arm11_R(0);

    if(handle == HANDLE_CURRENT_PROCESS) {
        printf("Program exited successfully.\n");
        PAUSE();
        exit(0);
    }

    handleinfo* hi = handle_Get(handle);

    if(hi == NULL) {
        ERROR("handle %08x not found.\n", handle);
        PAUSE();
#ifdef exitonerror
        exit(1);
#else
        return 0;
#endif
    }

    if(hi->type >= NUM_HANDLE_TYPES) {
        // This should never happen.
        ERROR("handle %08x has non-defined type.\n", handle);
        PAUSE();
        exit(1);
    }

    // Lookup actual callback in table.
    if(handle_types[hi->type].fnCloseHandle != NULL)
        return handle_types[hi->type].fnCloseHandle(state,hi);

    ERROR("svcCloseHandle undefined for handle-type \"%s\".\n",
          handle_types[hi->type].name);
    PAUSE();
    return 0;
}

u32 svcWaitSynchronization1() //todo timeout
{
    u32 handle = arm11_R(0);
    handleinfo* hi = handle_Get(handle);

    if(hi == NULL) {
        ERROR("handle %08x not found.\n", handle);
        PAUSE();
#ifdef exitonerror
        exit(1);
#else
        return 0;
#endif
    }

    if(hi->type >= NUM_HANDLE_TYPES) {
        // This should never happen.
        ERROR("handle %08x has non-defined type.\n", handle);
        PAUSE();
        exit(1);
    }

    u32 temp;
    bool locked = false;
    // Lookup actual callback in table.
    if (handle_types[hi->type].fnWaitSynchronization != NULL) {
        temp = handle_types[hi->type].fnWaitSynchronization(hi, &locked);
        if (locked) {
            u32* handelist = (u32*)malloc(4);
            *handelist = handle;
            lockcpu(handelist, 1,1);
        }
        return temp;
    } else {
        ERROR("svcCloseHandle undefined for handle-type \"%s\".\n",
              handle_types[hi->type].name);
        PAUSE();
        return 0;
    }

}
u32 svcWaitSynchronizationN() //todo timeout
{
    u32 *handelist;
    u32 nanoseconds1 = arm11_R(0);
    u32 handles = arm11_R(1);
    u32 handlecount = arm11_R(2);
    u32 waitAll = arm11_R(3);
    u32 nanoseconds2 = arm11_R(4);
    bool allunlockde = true;
    for (u32 i = 0; i < handlecount; i++) {
        u32 curhandel = mem_Read32(handles + i * 4);
        handleinfo* hi = handle_Get(curhandel);

        if (hi == NULL) {
            ERROR("handle %08x not found.\n", curhandel);
            PAUSE();
#ifdef exitonerror
            exit(1);
#else
            return 0;
#endif
        }

        if (hi->type >= NUM_HANDLE_TYPES) {
            // This should never happen.
            ERROR("handle %08x has non-defined type.\n", curhandel);
            PAUSE();
            exit(1);
        }

        u32 temp;
        bool locked = false;
        // Lookup actual callback in table.
        if (handle_types[hi->type].fnWaitSynchronization != NULL) {
            temp = handle_types[hi->type].fnWaitSynchronization(hi, &locked);
            if (!locked && waitAll == 0) {
                arm11_SetR(1,i);
                return 0;
            } else {
                allunlockde = false;
            }
        } else {
            ERROR("svcCloseHandle undefined for handle-type \"%s\".\n",
                  handle_types[hi->type].name);
            PAUSE();
            return 0;
        }
    }
    if (waitAll && allunlockde)return 0;
    handelist = malloc(handlecount*4);
    mem_Read((u8*)handelist, handles, handlecount * 4);
    lockcpu(handelist, waitAll, handlecount);
    return 0;
}
