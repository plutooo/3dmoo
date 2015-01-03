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

#ifdef GDB_STUB
#include "armemu.h"
#include "armdefs.h"

#include "gdb/gdbstub.h"
#include "gdb/gdbstubchelper.h"
#endif

#include "armemu.h"

extern ARMul_State s;

int loader_LoadFile(FILE* fd);
void loader_golbalmemsetup();

u32 g_process_handle;

static int running = 1;
int noscreen = 0;
bool disasm = false;
char* codepath = NULL;

#ifdef MODULE_SUPPORT
u32 modulenum = 0;
char** modulenames = NULL;

u32 overdrivnum = 0;
char** overdrivnames = NULL;

char** main_noloadnumnames = NULL;
u32 main_noloadnum;

extern u32 curprocesshandle;
#endif

u32 main_current_module = 0;

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

bool main_hasgdbarg = false;

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
    handle_Init(); //must be done first

    loader_golbalmemsetup();

    if (argc < 2) {
        printf("Usage:\n");

#ifdef MODULE_SUPPORT
        printf("%s <in.ncch> [-d|-noscreen|-codepatch <code>|-modules <num> <in.ncch>|-overdrivlist <num> <services>|-sdmc <path>|-sysdata <path>|-sdwrite|-slotone|-configsave|-gdbport <port>]\n", argv[0]);
#else
        printf("%s <in.ncch> [-d|-noscreen|-codepatch <code>|-sdmc <path>|-sysdata <path>|-sdwrite|-slotone|-configsave|-gdbport <port>]\n", argv[0]);
#endif

        return 1;
    }

    for (int i = 2; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0))disasm = true;
        else if ((strcmp(argv[i], "-noscreen") == 0))noscreen = true;
        else if ((strcmp(argv[i], "-codepatch") == 0)) {
            i++;
            codepath = malloc(strlen(argv[i])+1);
            strcpy(codepath, argv[i]);
        } else if ((strcmp(argv[i], "-sdmc") == 0)) {
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
            gdbstub_Init(atoi(argv[++i]));
        }
#endif

#ifdef MODULE_SUPPORT
        if ((strcmp(argv[i], "-modules") == 0)) {
            i++;
            modulenum = atoi(argv[i]);
            modulenames = malloc(sizeof(char*)*modulenum);
            i++;
            for (u32 j = 0; j < modulenum; j++) {
                *(modulenames + j) = malloc(strlen(argv[i]));
                strcpy(*(modulenames + j), argv[i]);
                i++;
            }
        }
        if (i >= argc)break;
        if ((strcmp(argv[i], "-overdrivlist") == 0)) {
            i++;
            overdrivnum = atoi(argv[i]);
            overdrivnames = malloc(sizeof(char*)*overdrivnum);
            i++;
            for (u32 j = 0; j < overdrivnum; j++) {
                *(overdrivnames + j) = malloc(strlen(argv[i]));
                strcpy(*(overdrivnames + j), argv[i]);
                i++;
            }
        }
        if ((strcmp(argv[i], "-noloadlist") == 0)) {
            i++;
            main_noloadnum = atoi(argv[i]);
            main_noloadnumnames = malloc(sizeof(char*)*main_noloadnum);
            i++;
            for (u32 j = 0; j < main_noloadnum; j++) {
                *(main_noloadnumnames + j) = malloc(strlen(argv[i]));
                strcpy(*(main_noloadnumnames + j), argv[i]);
                i++;
            }
        }
        if (i >= argc)break;
#endif
        if (i >= argc)
            break;
    }

#ifdef MODULE_SUPPORT
    curprocesshandlelist = malloc(sizeof(u32)*(modulenum + 1));
    ModuleSupport_MemInit(modulenum);
#endif

    if (!main_hasgdbarg)
        gdbstub_Init(0);

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


#ifdef MODULE_SUPPORT
    u32 i;

    for (i = 0; i<modulenum; i++) {
        u32 handzwei = handle_New(HANDLE_TYPE_PROCESS, 0);
        curprocesshandle = handzwei;
        *(curprocesshandlelist + i) = handzwei;

        ModuleSupport_SwapProcessMem(i);

        s.NextInstr = RESUME;

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
    ModuleSupport_SwapProcessMem(modulenum);
#else
    u32 handzwei = handle_New(HANDLE_TYPE_PROCESS, 0);
    curprocesshandle = handzwei;
#endif

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
#ifdef MODULE_SUPPORT
    s.NextInstr = RESUME;
    ModuleSupport_SwapProcessMem(0);
#endif
    // Execute.
    while (running) {
#ifdef MODULE_SUPPORT
        for (main_current_module = 0; main_current_module < modulenum + 1; main_current_module++)
        {
            DEBUG("process:%d\n", main_current_module);
            ModuleSupport_SwapProcessMem(main_current_module);
#endif
            if (!noscreen)
                screen_HandleEvent();
            threads_Execute();
            //FPS_Lock();
            //mem_Dbugdump();
#ifdef MODULE_SUPPORT
        }
#endif
    }


    fclose(fd);
    return 0;
}
