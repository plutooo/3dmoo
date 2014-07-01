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
#include "screen.h"
#include "gpu.h"

#include "handles.h"

#include "init.h"

#include <signal.h>
#include <SDL.h>

int loader_LoadFile(FILE* fd);

u32 curprocesshandle;

static int running = 1;
int noscreen = 0;
bool disasm = false;
#ifdef modulesupport
u32 modulenum = 0;
char** modulenames = NULL;
#endif

#define FPS  60
#define interval 1000 / FPS // The unit of interval is ms.
u32 NextTick;

void AtSig(int t)
{
    running = 0;
    screen_Free();
    exit(1);
}

void AtExit(void){
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
    if (argc < 2) {
        printf("Usage:\n");
#ifdef modulesupport
        printf("%s <in.ncch> [-d|-noscreen|-modules <num> <in.ncch>]\n", argv[0]);
#else
        printf("%s <in.ncch> [-d|-noscreen]\n", argv[0]);
#endif
        return 1;
    }

    //disasm = (argc > 2) && (strcmp(argv[2], "-d") == 0);
    //noscreen =    (argc > 2) && (strcmp(argv[2], "-noscreen") == 0);

    for (int i = 2; i < argc; i++)
    {
        if ((strcmp(argv[i], "-d") == 0))disasm = true;
        if ((strcmp(argv[i], "-noscreen") == 0))noscreen = true;
#ifdef modulesupport
        if ((strcmp(argv[i], "-modules") == 0))
        {
            i++;
            modulenum = atoi(argv[i]);
            modulenames = malloc(sizeof(char*)*modulenum);
            i++;
            for (int j = 0; j < modulenum; j++)
            {
                *(modulenames + j) = malloc(strlen(argv[i]));
                strcpy(*(modulenames + j), argv[i]);
                i++;
            }
        }
#endif
    }

    curprocesshandlelist = malloc(sizeof(u32)*(modulenum + 1));
    mem_init(modulenum);


    signal(SIGINT, AtSig);

    if (!noscreen)
        screen_Init();
    hid_user_init();
    initDSP();
    mcu_GPU_init();
    initGPU();


    arm11_Init();

#ifdef modulesupport
    int i;
    for (i = 0;i<modulenum;i++)
    {
        u32 handzwei = handle_New(HANDLE_TYPE_PROCESS, 0);
        curprocesshandle = handzwei;
        *(curprocesshandlelist + i) = handzwei;
        swapprocess(i);
        u32 hand = handle_New(HANDLE_TYPE_THREAD, 0);
        threads_New(hand);

        // Load file.
        FILE* fd = fopen(*(modulenames + i), "rb");
        if (fd == NULL) {
            perror("Error opening file");
            return 1;
        }
        if (loader_LoadFile(fd) != 0) {
            fclose(fd);
            return 1;
        }
    }

    u32 handzwei = handle_New(HANDLE_TYPE_PROCESS, 0);
    *(curprocesshandlelist + modulenum) = handzwei;
    swapprocess(modulenum);
#else
    u32 handzwei = handle_New(HANDLE_TYPE_PROCESS, 0);
    curprocesshandle = handzwei;
#endif

    FILE* fd = fopen(argv[1], "rb");
    if (fd == NULL) {
        perror("Error opening file");
        return 1;
    }

    u32 hand = handle_New(HANDLE_TYPE_THREAD, 0);
    threads_New(hand);



    // Load file.
    if (loader_LoadFile(fd) != 0) {
        fclose(fd);
        return 1;
    }


    // Execute.
    while (running) {
        if (!noscreen)
            screen_HandleEvent();

#ifdef modulesupport
        int k;
        for (k = 0; k <= modulenum; k++) {
            swapprocess(k);
#endif
            threads_Execute();
#ifdef modulesupport
        }
#endif

        if (!noscreen)
            screen_RenderGPU();

        FPS_Lock();
    }


    fclose(fd);
    return 0;
}
