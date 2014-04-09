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

typedef struct {
    u32  r[18];
    bool active;
} thread;

#define MAX_THREADS 32

static thread threads[MAX_THREADS];
static u32    num_threads;


u32 threads_New()
{
    if(num_threads == MAX_THREADS) {
        ERROR("Too many threads..\n");
        arm11_Dump();
        PAUSE();
        exit(1);
    }

    threads[num_threads].active = true;
    return num_threads++;
}

void threads_Switch(u32 from, u32 to)
{
    if(from >= num_threads || to >= num_threads) {
        ERROR("Trying to switch nonexisting threads..\n");
        arm11_Dump();
        exit(1);
    }

    if(!threads[from].active || !threads[to].active) {
        ERROR("Trying to switch nonactive threads..\n");
        arm11_Dump();
        exit(1);
    }

    DEBUG("Thread switch %d->%d\n", from, to);
    arm11_SaveContext(threads[from].r);
    arm11_LoadContext(threads[to].r);
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

    arm11_SetR(1, handle_New(0, 0)); // r1 = handle_out
    return 0;
}
