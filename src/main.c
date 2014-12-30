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

#include "config.h"
#include "armemu.h"

#ifdef GDB_STUB
#include "gdb/gdbstub.h"
#endif

extern ARMul_State s;

int loader_LoadFile(FILE* fd);

// Holds handle for the process that is being executed.
u32 g_process_handle;

static int running = 1;
int noscreen = 0;
bool disasm = false;
char* codepath = NULL;

#define FPS  60
#define interval 1000 / FPS // The unit of interval is ms.
u32 NextTick;


void AtSig(int t)
{
    running = 0;
    screen_Free();
    exit(1);
}

void AtExit(void)
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
        SDL_Delay(delay);
    }
    NextTick = SDL_GetTicks() + interval;
    return;
}

int main(int argc, char* argv[])
{
#ifndef WIN32
    setlinebuf(stdout);
    setlinebuf(stderr);
#else
    //char outBuf[4096];
    //char errBuf[4096];

    //setvbuf(stdout, outBuf, _IOLBF, sizeof(outBuf));
    //setvbuf(stderr, errBuf, _IOLBF, sizeof(errBuf));
#endif
    atexit(AtExit);

    handle_Init();

    if (argc < 2) {
        printf("Usage:\n");
        printf("%s <in.ncch> [-d|-noscreen|-codepatch <code>|-sdmc <path>|-sysdata <path>|-sdwrite|-slotone|-configsave|-gdbport <port>]\n", argv[0]);
        return 1;
    }

    for (int i = 2; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0))disasm = true;
        else if ((strcmp(argv[i], "-noscreen") == 0))noscreen = true;
        else if ((strcmp(argv[i], "-sdmc") == 0)) {
            i++;
            strcpy(config_sdmc_path, argv[i]);
            config_has_sdmc = true;
        } else if ((strcmp(argv[i], "-sysdata") == 0)) {
            i++;
            strcpy(config_sysdataoutpath, argv[i]);
            config_usesys = true;
        }
        else if ((strcmp(argv[i], "-sdwrite") == 0))config_sdmcwriteable = true;
        else if ((strcmp(argv[i], "-slotone") == 0))config_slotone = true;
        else if ((strcmp(argv[i], "-configsave") == 0))config_nand_cfg_save = true;
        else if ((strcmp(argv[i], "-region=EU") == 0))config_region = 2;
        else if ((strcmp(argv[i], "-region=USA") == 0))config_region = 1;
        else if ((strcmp(argv[i], "-region=JP") == 0))config_region = 0;

#ifdef GDB_STUB
        if ((strcmp(argv[i], "-gdbport") == 0)) {
            global_gdb_port = atoi(argv[++i]);
            if (global_gdb_port < 1 || global_gdb_port > 65535)
            {
                ERROR("gdb port out of range!\n");
                exit(-1);
            }
        }
#endif
        if (i >= argc)
            break;
    }

#ifdef GDB_STUB
    gdbstub_Init(global_gdb_port);
#endif

    signal(SIGINT, AtSig);

    if (!noscreen)
        screen_Init();

    // Services that need some init.
    hid_spvr_init();
    hid_user_init();
    mcu_GPU_init();
    dsp_Init();
    gpu_Init();
    srv_InitGlobal();

    arm11_Init();

    g_process_handle = handle_New(HANDLE_TYPE_PROCESS, 0);

    FILE* fd = fopen(argv[1], "rb");
    if (fd == NULL) {
        perror("Error opening file");
        return 1;
    }

    threads_New(handle_New(HANDLE_TYPE_THREAD, 0));

    // Load file.
    if (loader_LoadFile(fd) != 0) {
        fclose(fd);
        return 1;
    }

    // Execute.
    while (running) {
        if (!noscreen)
            screen_HandleEvent();

        threads_Execute();
        //FPS_Lock();
        //mem_Dbugdump();
    }


    fclose(fd);
    return 0;
}
