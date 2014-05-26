/*
* Copyright (C) 2014 - Normmatt
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

#ifndef _THREADS_H_
#define _THREADS_H_

typedef enum {
    RUNNING,
    STOPPED,
    WAITING
} thread_state;

typedef struct {
    u32  r[13];
    u32 sp;
    u32 lr;
    u32 pc;
    u32 cpsr;
    u32 fpu_r[32];
    u32 fpscr;
    u32 fpexc;
    u32 mode;
    u32 r15;

    thread_state state;

    u32* wait_list;
    u32  wait_list_size;
    bool wait_all;

    u32 handle;
    s32 priority;
} thread;


u32  threads_New(u32 handle);
bool threads_IsThreadActive(u32 id);
void threads_Execute();
u32  threads_Count();
u32  threads_GetCurrentThreadHandle();
void threads_StopThread(u32 threadid);
void threads_StopCurrentThread();
u32  threads_FindIdByHandle(u32 handle);
void threads_SaveContextCurrentThread();
void threads_SetCurrentThreadWaitList(u32* wait_list, bool wait_all, u32 num);

u32 svcGetThreadPriority();
u32 svcSetThreadPriority();
u32 svcGetThreadId();
u32 svcCreateThread();

#endif
