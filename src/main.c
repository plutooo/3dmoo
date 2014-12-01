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
#include "gdbstub.h"
#include "armdefs.h"
#include "gdbstubchelper.h"
volatile bool arm_stall = false;
#endif

extern ARMul_State s;

int loader_LoadFile(FILE* fd);

u32 curprocesshandle;

#ifdef GDB_STUB
u32 global_gdb_port = 0;
gdbstub_handle_t gdb_stub;
struct armcpu_memory_iface *gdb_memio;
struct armcpu_memory_iface gdb_base_memory_iface;
struct armcpu_ctrl_iface gdb_ctrl_iface;
#endif

static int running = 1;
int noscreen = 0;
bool disasm = false;
char* codepath = NULL;

#ifdef MODULE_SUPPORT
u32 modulenum = 0;
char** modulenames = NULL;

u32 overdrivnum = 0;
char** overdrivnames = NULL;
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

#ifdef MODULE_SUPPORT
        printf("%s <in.ncch> [-d|-noscreen|-codepatch <code>|-modules <num> <in.ncch>|-overdrivlist <num> <services>|-sdmc <path>|-sysdata <path>|-sdwrite|-slotone|-configsave|-gdbport <port>]\n", argv[0]);
#else
        printf("%s <in.ncch> [-d|-noscreen|-codepatch <code>|-sdmc <path>|-sysdata <path>|-sdwrite|-slotone|-configsave|-gdbport <port>]\n", argv[0]);
#endif

        return 1;
    }

    //disasm = (argc > 2) && (strcmp(argv[2], "-d") == 0);
    //noscreen =    (argc > 2) && (strcmp(argv[2], "-noscreen") == 0);

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
        } else if ((strcmp(argv[i], "-sdwrite") == 0))config_slotone = true;
        else if ((strcmp(argv[i], "-slotone") == 0))config_sdmcwriteable = true;
        else if ((strcmp(argv[i], "-configsave") == 0))config_nand_cfg_save = true;

#ifdef GDB_STUB
        if ((strcmp(argv[i], "-gdbport") == 0)) {
            i++;
            global_gdb_port = atoi(argv[i]);
            if (global_gdb_port < 1 || global_gdb_port > 65535) {
                DEBUG("ARM9 GDB stub port must be in the range 1 to 65535\n");
                exit(-1);
            }
            gdb_ctrl_iface.stall = stall_cpu;
            gdb_ctrl_iface.unstall = unstall_cpu;
            gdb_ctrl_iface.read_reg = read_cpu_reg;
            gdb_ctrl_iface.set_reg = set_cpu_reg;
            gdb_ctrl_iface.install_post_ex_fn = install_post_exec_fn;
            gdb_ctrl_iface.remove_post_ex_fn = remove_post_exec_fn;

            gdb_base_memory_iface.prefetch16 = gdb_prefetch16;
            gdb_base_memory_iface.prefetch32 = gdb_prefetch32;
            gdb_base_memory_iface.read32 = gdb_read32;
            gdb_base_memory_iface.write32 = gdb_write32;
            gdb_base_memory_iface.read16 = gdb_read16;
            gdb_base_memory_iface.write16 = gdb_write16;
            gdb_base_memory_iface.read8 = gdb_read8;
            gdb_base_memory_iface.write8 = gdb_write8;




        }
#endif
        if (i >= argc)break;


#ifdef MODULE_SUPPORT
        if ((strcmp(argv[i], "-modules") == 0)) {
            i++;
            modulenum = atoi(argv[i]);
            modulenames = malloc(sizeof(char*)*modulenum);
            i++;
            for (int j = 0; j < modulenum; j++) {
                *(modulenames + j) = malloc(strlen(argv[i]));
                strcpy(*(modulenames + j), argv[i]);
                i++;
            }
        }
        if (i >= argc)break;
        if ((strcmp(argv[i], "-overdrivlist") == 0)) {
            i++;
            overdrivnum = atoi(argv[i]);
            overdrivnames = malloc(sizeof(char*)*modulenum);
            i++;
            for (int j = 0; j < modulenum; j++) {
                *(overdrivnames + j) = malloc(strlen(argv[i]));
                strcpy(*(overdrivnames + j), argv[i]);
                i++;
            }
        }
        if (i >= argc)break;
#endif

    }

#ifdef MODULE_SUPPORT
    curprocesshandlelist = malloc(sizeof(u32)*(modulenum + 1));
    ModuleSupport_MemInit(modulenum);
#endif

    signal(SIGINT, AtSig);

    if (!noscreen)
        screen_Init();
    hid_spvr_init();
    hid_user_init();
    initDSP();
    mcu_GPU_init();
    gpu_Init();
    srv_InitGlobal();


    arm11_Init();

#ifdef MODULE_SUPPORT
    int i;

    for (i = 0; i<modulenum; i++) {
        u32 handzwei = handle_New(HANDLE_TYPE_PROCESS, 0);
        curprocesshandle = handzwei;
        *(curprocesshandlelist + i) = handzwei;

        ModuleSupport_SwapProcessMem(i);

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

#ifdef GDB_STUB
    if (global_gdb_port) {
        gdb_stub = createStub_gdb(global_gdb_port,
                                  &gdb_memio,
                                  &gdb_base_memory_iface);
        if (gdb_stub == NULL) {
            DEBUG("Failed to create ARM9 gdbstub on port %d\n",
                  global_gdb_port);
            exit(-1);
        }
        activateStub_gdb(gdb_stub, &gdb_ctrl_iface);
    } else {
        gdb_memio = malloc(sizeof(struct armcpu_memory_iface));
        gdb_memio->prefetch16 = gdb_prefetch16;
        gdb_memio->prefetch32 = gdb_prefetch32;
        gdb_memio->read32 = gdb_read32;
        gdb_memio->write32 = gdb_write32;
        gdb_memio->read16 = gdb_read16;
        gdb_memio->write16 = gdb_write16;
        gdb_memio->read8 = gdb_read8;
        gdb_memio->write8 = gdb_write8;
    }
#endif
    // Execute.
    while (running) {
        if (!noscreen)
            screen_HandleEvent();

#ifdef MODULE_SUPPORT
        int k;
        for (k = 0; k <= modulenum; k++) {
            ModuleSupport_SwapProcess(k);
            DEBUG("Process %X\n",k);
        }
#else
        threads_Execute();
#endif

        //FPS_Lock();
        //mem_Dbugdump();
    }


    fclose(fd);
    return 0;
}
