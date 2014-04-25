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
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <SDL.h>

#include "util.h"
#include "arm11.h"
#include "screen.h"
#include "SrvtoIO.h"

#include "handles.h"

int loader_LoadFile(FILE* fd);

u32 curprocesshandle;

static int running = 1;
int noscreen = 0;

#define FPS  60
#define interval 1000 / FPS // The unit of interval is ms.
u32 NextTick;

void AtSig(int t)
{
    running = 0;
    screen_Free();
    exit(1);
}

void AtExit()
{
    arm11_Dump();
    if(!noscreen)
        screen_Free();

    printf("\nEXIT\n");
}

void FPS_Lock(void)
{
    if (NextTick > SDL_GetTicks()) {
        u32 delay = NextTick - SDL_GetTicks();
        //fprintf(stderr,"delay = %08X\n", delay);
        SDL_Delay(delay);
    }
    NextTick = SDL_GetTicks() + interval;
    return;
}




int main(int argc, char* argv[])
{
    atexit(AtExit);
    if(argc < 2) {
        printf("Usage:\n");
        printf("%s <in.ncch> [-d|-noscreen]\n", argv[0]);
        return 1;
    }

    bool disasm = (argc > 2) && (strcmp(argv[2], "-d") == 0);
    noscreen =    (argc > 2) && (strcmp(argv[2], "-noscreen") == 0);

    FILE* fd = fopen(argv[1], "rb");
    if(fd == NULL) {
        perror("Error opening file");
        return 1;
    }

    signal(SIGINT, AtSig);

    if(!noscreen)
        screen_Init();

    arm11_Init();
    u32 hand = handle_New(HANDLE_TYPE_THREAD, 0);
    threads_New(hand);
    u32 handzwei = handle_New(HANDLE_TYPE_PROCESS, 0);
    curprocesshandle = handzwei;
    hid_user_init();
    if(!noscreen)
        initGPU();

    // Load file.
    if(loader_LoadFile(fd) != 0) {
        fclose(fd);
        return 1;
    }

    // Execute.
    while(running) {
        if(!noscreen)
            screen_HandleEvent();

        /*for (int i = 0; i < 0x20000; i++) { // todo
            uint32_t pc = arm11_R(15);

            if (disasm) {
                printf("[%08x] ", pc);
                arm11_Disasm32(pc);
            }

            arm11_Step();
        }*/

        for (int i = 0; i < 60; i++) {
            for (int t = 0; t < threads_Count(); t++) {
                //int from = t == 0 ? threads_Count() - 1 : t - 1;
                if (islocked(t))continue;
                threads_Switch(/*from,*/ t);
                arm11_Run(0x8000000 / 60);
            }
        }

        if (!noscreen)
            screen_RenderGPU();

        FPS_Lock();
    }


    fclose(fd);
    return 0;
}
