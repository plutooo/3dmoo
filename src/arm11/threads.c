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

#include "util.h"
#include "handles.h"
#include "arm11.h"

#include "armdefs.h"
#include "armemu.h"
#include "mem.h"

#include "threads.h"

#ifdef modulesupport
thread** threadsproc;
u32*    num_threadsproc;
u32     current_proc = 0;
#endif

thread threads[MAX_THREADS];
static u32    num_threads = 0;
static s32    current_thread = 0;
static u32    reschedule = 0;

//#define PROPER_THREADING

#define THREAD_ID_OFFSET 0xC

#ifdef modulesupport
void threadmod_init(u32 modulenum)
{
    u32 i;
    threadsproc = (thread **)malloc(sizeof(thread *)*(modulenum + 1));
    for (i = 0; i < (modulenum + 1); i++)
    {
        *(threadsproc + i) = (thread *)malloc(sizeof(thread)*(MAX_THREADS));
        memset(*(threadsproc + i), 0, sizeof(thread)*(MAX_THREADS));
    }
    num_threadsproc = (thread **)malloc(sizeof(u32*)*(modulenum + 1));
    memset(num_threadsproc, 0, sizeof(u32*)*(modulenum + 1));
}
void threadmodswapprocess(u32 newproc)
{
    threads_SaveContextCurrentThread();
    memcpy(*(threadsproc + current_proc), threads, sizeof(thread)*(MAX_THREADS)); //save maps
    *(num_threadsproc + current_proc) = num_threads;

    memcpy(threads, *(threadsproc + newproc), sizeof(thread)*(MAX_THREADS)); //save maps
    current_thread = 0;
    num_threads = *(num_threadsproc + newproc);
    curprocesshandle = *(curprocesshandlelist + newproc);
    current_proc = newproc;
    arm11_LoadContext(&threads[0]);
}

#endif

u32 threads_New(u32 handle)
{
    if(num_threads == MAX_THREADS) {
        ERROR("Too many threads..\n");
        arm11_Dump();
        PAUSE();
        exit(1);
    }

    threads[num_threads].priority = 50;
    threads[num_threads].handle = handle;
    threads[num_threads].state = RUNNING;
    threads[num_threads].wait_list = NULL;
    threads[num_threads].wait_list_size = 0;

    return num_threads++;
}

// Returns true if given thread is ready to execute.
bool threads_IsThreadActive(u32 id)
{
    u32 i;
    bool ret;

    switch(threads[id].state) {
    case RUNNING:
        return true;

    case STOPPED:
        return false;

    case WAITING_ARB:
        DEBUG("Thread is %d is stuck in arbitration.\n", id);
        return false;

    case WAITING_SYNC:
        DEBUG("Wait-list for thread %d:\n", id);

        if(threads[id].wait_all) {
            ret = true;

            for(i=0; i<threads[id].wait_list_size; i++) {
                u32 handle = threads[id].wait_list[i];

                handleinfo* hi = handle_Get(handle);
                if(hi == NULL)
                    continue;

                bool is_waiting = false;
                handle_types[hi->type].fnWaitSynchronization(hi, &is_waiting);

                DEBUG("    %08x, type=%s, waiting=%s\n", handle, handle_types[hi->type].name,
                      is_waiting ? "true" : "false");

                if(is_waiting) 
                    ret = false;
            }

            if (ret)
            {
                threads[id].r[1] = threads[id].wait_list_size;
                threads[id].state = RUNNING;
            }
            return ret;
        }
        else {
            ret = false;

            for(i=0; i<threads[id].wait_list_size; i++) {
                u32 handle = threads[id].wait_list[i];

                handleinfo* hi = handle_Get(handle);
                if(hi == NULL)
                    continue;

                bool is_waiting = false;
                handle_types[hi->type].fnWaitSynchronization(hi, &is_waiting);

                DEBUG("    %08x, type=%s, waiting=%s\n", handle, handle_types[hi->type].name,
                      is_waiting ? "true" : "false");

                if(!ret && !is_waiting) {
                    threads[id].r[1] = i;
                    threads[id].state = RUNNING;
                    ret = true;
                }
            }
            return ret;
        }
    }

    return false;
}

u32 threads_NextIdToBeDeleted()
{
    u32 i;

    for (i=0; i<threads_Count(); i++) {
        if (threads[i].state == STOPPED)
            return i;
    }
    return -1;
}

void threads_RemoveZombies()
{
    u32 id;
    u32 i;

    while ((id = threads_NextIdToBeDeleted()) != -1) {
        for(i=id; i<threads_Count(); i++)
            threads[i] = threads[i + 1];

        num_threads--;
    }
}

void threads_Switch(/*u32 from,*/ u32 to)
{
    u32 from = current_thread;

    if (from == to) {
        DEBUG("Trying to switch to current thread..\n");
        return;
    }

    if(threads[to].state == STOPPED) {
        ERROR("Trying to switch to a stopped thread..\n");
        arm11_Dump();
        exit(1);
    }

    if (current_thread != -1)
    {
        DEBUG("Thread switch %d->%d (%08X->%08X)\n", from, to, threads[from].handle, threads[to].handle);
        arm11_SaveContext(&threads[from]);
    }

    arm11_LoadContext(&threads[to]);
    current_thread = to;
}

u32 line = 0;

void threads_DoReschedule()
{
#ifdef PROPER_THREADING
    u32 t;
    u32 cur_prio = 0;
    u32 next_thread = 0;

    //threads_SaveContextCurrentThread();
    threads_RemoveZombies();

    for (t = 0; t < threads_Count(); t++) {
        if (t == current_thread) continue;

        if (!threads_IsThreadActive(t)) {
            DEBUG("Skipping thread %d..\n", t);
            continue;
        }

        if (threads[t].priority >= cur_prio)
        {
            cur_prio = threads[t].priority;
            next_thread = t;
        }
    }

    threads_Switch(next_thread);
#endif
}

void threads_Execute() {

#ifdef PROPER_THREADING
    if (reschedule)
    {
        threads_DoReschedule();
        reschedule = 0;
    }

    sendGPUinterall(2);
    line++;
    if (line == 400)
    {
        sendGPUinterall(3);
        line = 0;
    }

    arm11_Run(0x7FFFFFFF);
#else
    u32 t;

    for (t = 0; t < threads_Count(); t++) {
        sendGPUinterall(2);
        line++;
        if (line == 400)
        {
            sendGPUinterall(3);
            line = 0;
        }
        if (!threads_IsThreadActive(t)) {
            DEBUG("Skipping thread %d..\n", t);
            continue;
        }

        threads_Switch(t);

        //arm11_Run(11172); //process one line

        arm11_Run(0x7FFFFFFF);

    }

    threads_SaveContextCurrentThread();
    threads_RemoveZombies();
#endif
}

void threads_Reschedule()
{
    reschedule = 1;
}

u32 threads_Count()
{
    return num_threads;
}

u32 threads_GetCurrentThreadHandle()
{
    return threads[current_thread].handle;
}

void threads_StopThread(u32 threadid)
{
    threads[threadid].state = STOPPED;
}

void threads_StopCurrentThread()
{
    threads_StopThread(current_thread);
}

u32 threads_FindIdByHandle(u32 handle)
{
    u32 i;

    if (handle == 0xffff8000)
        return current_thread;

    for(i=0; i<threads_Count(); i++) {
        if (threads[i].handle == handle)
            return i;
    }
    return -1;
}

void threads_SaveContextCurrentThread()
{
    if (current_thread != -1) {
        arm11_SaveContext(&threads[current_thread]);

        if (num_threads > 1)
            current_thread = -1;
    }
}

extern ARMul_State s;

void threads_SetCurrentThreadWaitList(u32* wait_list, bool wait_all, u32 num)
{
    if(threads[current_thread].wait_list != NULL)
        free(threads[current_thread].wait_list);

    threads[current_thread].wait_list = wait_list;
    threads[current_thread].state = WAITING_SYNC;
    threads[current_thread].wait_all = wait_all;
    threads[current_thread].wait_list_size = num;

    s.NumInstrsToExecute = 0;
}

// Sets current thread into arbitration suspend.
void threads_SetCurrentThreadArbitrationSuspend(u32 arbiter, u32 addr)
{
    // This should never happen.
    if(threads[current_thread].state != RUNNING) {
        ERROR("Warning: arbiting non-running thread!\n");
    }

    threads[current_thread].state      = WAITING_ARB;
    threads[current_thread].arb_addr   = addr;
    threads[current_thread].arb_handle = arbiter;

    s.NumInstrsToExecute = 0;
}

void threads_ResumeArbitratedThread(thread* t)
{
    t->arb_handle = 0;
    t->arb_addr = 0;

    t->state = RUNNING;
}

// Returns highest prio thread waiting for arbitration.
thread* threads_ArbitrateHighestPrioThread(u32 arbiter, u32 addr)
{
    thread* ret = NULL;
    s32 highest_prio = 0x80;
    u32 i;

    for(i=0; i<threads_Count(); i++) {
        if(threads[i].state != WAITING_ARB)
            continue;
        if(threads[i].arb_handle != arbiter)
            continue;
        if(threads[i].arb_addr != addr)
            continue;

        if(threads[i].priority <= highest_prio) {
            ret = &threads[i];
            highest_prio = threads[i].priority;
        }
    }

    return ret;
}

// -- Syscall implementations --

u32 svcGetThreadPriority()
{
    u32 out = arm11_R(0);
    u32 hand = arm11_R(1);
    s32 prio = 0;

    u32 threadid = threads_FindIdByHandle(hand);

    if (threadid != -1) {
        DEBUG("Thread Priority : %d\n", threads[threadid].priority);
        prio = threads[threadid].priority;
    }

    arm11_SetR(1, prio); // r1 = prio out

    return 0;
}

u32 svcSetThreadPriority()
{
    u32 hand = arm11_R(0);
    s32 prio = arm11_R(1);

    u32 threadid = threads_FindIdByHandle(hand);

    if (threadid != -1) {
        DEBUG("Thread Priority : %d -> %d\n", threads[threadid].priority, prio);
        threads[threadid].priority = prio;
    }

    return 0;
}

u32 svcGetThreadId()
{
    u32 handle = arm11_R(1);

    if (handle == 0xffff8000)
        return THREAD_ID_OFFSET + current_thread;
    else {
        DEBUG("svcGetThreadId not supported\n");
        return 0;
    }
}

u32 svcCreateThread()
{
    u32 prio = arm11_R(0);
    u32 ent_pc = arm11_R(1);
    u32 ent_r0 = arm11_R(2);
    u32 ent_sp = arm11_R(3);
    u32 cpu  = arm11_R(4);

    DEBUG("entrypoint=%08x, r0=%08x, sp=%08x, prio=%x, cpu=%x\n",
          ent_pc, ent_r0, ent_sp, prio, cpu);

    u32 hand = handle_New(HANDLE_TYPE_THREAD, 0);
    u32 numthread = threads_New(hand);

    threads[numthread].priority = prio;
    threads[numthread].r[0] = ent_r0;
    threads[numthread].sp = ent_sp;
    threads[numthread].r15 = ent_pc;
    threads[numthread].cpsr = 0x1F; //usermode
    threads[numthread].mode = RESUME;

    arm11_SetR(1, hand); // r1 = handle_out

    return 0;
}

// --- Thread handle callbacks ---

u32 thread_CloseHandle(ARMul_State *state, handleinfo* h)
{
    u32 id = threads_FindIdByHandle(h->handle);
    if (id == -1)
        return -1;

    threads_StopThread(id);
    state->NumInstrsToExecute = 0;
    return 0;
}

u32 thread_SyncRequest(handleinfo* h, bool *locked)
{
    u32 cid = mem_Read32(arm11_ServiceBufferAddress() + 0x80);

    switch (cid) {
    default:
        break;
    }

    ERROR("STUBBED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}

u32 thread_WaitSynchronization(handleinfo* h, bool *locked)
{
    DEBUG("waiting for thread to unlock..\n");
    PAUSE();

    *locked = h->locked;

    return 0;
}
