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

    bool active;
    u8* handellist;
    u32 waitall;
    u32 handellistcount;
    u32 ownhand;
} thread;

u32 threads_New(u32 hand);
bool islocked(u32 t);
u32 threads_Count();
u32 threads_getcurrenthandle();
void threads_Switch(/*u32 from,*/ u32 to);
u32 svcCreateThread();
void lockcpu(u32* handelist, u32 waitAll, u32 count);

#endif