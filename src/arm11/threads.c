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

#include "util.h"
#include "handles.h"
#include "arm11.h"

#include "armdefs.h"
#include "armemu.h"

#include "threads.h"


#define MAX_THREADS 32

static thread threads[MAX_THREADS];
static u32    num_threads = 0;

u32 threads_New(u32 hand)
{
    if(num_threads == MAX_THREADS) {
        ERROR("Too many threads..\n");
        arm11_Dump();
        PAUSE();
        exit(1);
    }
    threads[num_threads].ownhand = hand;
    threads[num_threads].active = true;
    threads[num_threads].handellist = 0;
    return num_threads++;
}
bool islocked(u32 t)
{
    if (threads[t].delete)
    {
        return true;
    }
    else if (threads[t].active)
    {
        return false;
    }
    else
    {
        bool allunlockde = true;
        for (int i = 0; i < threads[t].handellistcount; i++)
        {
            handleinfo* hi = handle_Get(*(u32*)(threads[t].handellist + 4 * i));
            u32 temp;
            bool locked = false;
            // Lookup actual callback in table.
            temp = handle_types[hi->type].fnWaitSynchronization(hi, &locked);
            if (!locked && threads[t].waitall == 0)
            {
                threads[t].r[1] = i;
                threads[t].active = true;
                return false;
            }
            else
            {
                allunlockde = false;
            }
        }
        if (allunlockde)
        {
            threads[t].r[1] = threads[t].handellistcount;
            threads[t].active = true;
            return false;
        }
        return true;
    }
}
u32 threads_Count()
{
    return num_threads;
}
u32 currentthread = 0;
u32 threads_getcurrenthandle()
{
    return threads[currentthread].ownhand;
}
void threads_removethread(u32 threadid)
{
    threads[threadid].delete = true;
    /*for (int i = threadid; i < threads_Count(); i++)
    {
        threads[i] = threads[i + 1];
    }
    num_threads--;*/
}
void threads_removecurrent()
{
    threads_removethread(currentthread);
}
u32 threads_find(u32 handle)
{
    for (int i = 0; i < threads_Count(); i++)
    {
        if (threads[i].ownhand == handle)
        {
            return i;
        }
    }
    return -1;
}
u32 threads_NextToBeDeleted()
{
    for (int i = 0; i < threads_Count(); i++)
    {
        if (threads[i].delete)
        {
            return i;
        }
    }
    return -1;
}
void threads_Remove()
{
    int id;
    while ((id = threads_NextToBeDeleted()) != -1)
    {
        for (int i = id; i < threads_Count(); i++)
        {
            threads[i] = threads[i + 1];
        }
        num_threads--;
    }
}
void threads_Switch(/*u32 from,*/ u32 to)
{
    u32 from = currentthread;
    if (from == to) {
        DEBUG("Trying to switch to current thread..\n");
        return;
    }

    /*if(from >= num_threads || to >= num_threads) {
        ERROR("Trying to switch nonexisting threads..\n");
        arm11_Dump();
        exit(1);
    }*/

    if(!threads[to].active) {
        ERROR("Trying to switch nonactive threads..\n");
        arm11_Dump();
        exit(1);
    }

    DEBUG("Thread switch %d->%d\n", from, to);

    if (from < num_threads)
        arm11_SaveContext(&threads[from]);

    arm11_LoadContext(&threads[to]);
    currentthread = to;
}

u32 svcCreateThread()
{
    u32 prio = arm11_R(0);
    u32 ent_pc = arm11_R(1);
    u32 ent_r0 = arm11_R(2);
    u32 ent_sp = arm11_R(3);
    u32 cpu  = arm11_R(4);

    DEBUG("entrypoint=%08x, r0=%08x, sp=%08x, prio=%x, cpu=%x",
          ent_pc, ent_r0, ent_sp, prio, cpu);

    u32 hand = handle_New(HANDLE_TYPE_THREAD, 0);
    u32 numthread = threads_New(hand);

    threads[numthread].r[0] = ent_r0;
    threads[numthread].sp = ent_sp;
    threads[numthread].r15 = ent_pc;
    threads[numthread].cpsr = 0x1F; //usermode
    threads[numthread].mode = RESUME;

    arm11_SetR(1, hand); // r1 = handle_out

    return 0;
}
extern ARMul_State s;
void lockcpu(u32* handelist, u32 waitAll,u32 count)
{
    if (threads[currentthread].handellist != 0) free(threads[currentthread].handellist);
    threads[currentthread].handellist = (u8*)handelist;
    threads[currentthread].active = false;
    threads[currentthread].waitall = waitAll;
    threads[currentthread].handellistcount = count;
    s.NumInstrsToExecute = 0;
}

u32 thread_CloseHandle(ARMul_State *state, handleinfo* h)
{
    u32 id = threads_find(h->handle);
    if (id == -1) return -1;

    threads_removethread(id);
    state->NumInstrsToExecute = 0;
}

u32 thread_SyncRequest(handleinfo* h, bool *locked)
{
    u32 cid = mem_Read32(0xFFFF0080);
    switch (cid)
    {
    default:
        break;
    }
    ERROR("STUBBED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
}

u32 thread_WaitSynchronization(handleinfo* h, bool *locked)
{
    DEBUG("waiting for thread to unlock..\n");
    PAUSE();

    *locked = h->locked;

    return 0;
}