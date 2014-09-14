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

#ifdef GDB_STUB
#include "armemu.h"
#include "gdbstub.h"
#include "armdefs.h"
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
#ifdef modulesupport
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

#ifdef GDB_STUB

void *
createThread_gdb(void(*thread_function)(void *data),
void *thread_data)
{
    u32 ThreadId;
    HANDLE *new_thread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        (LPTHREAD_START_ROUTINE) thread_function,       // thread function name
        thread_data,          // argument to thread function 
        0,                      // use default creation flags 
        &ThreadId);   // returns the thread identifier 

    return new_thread;
}

void
joinThread_gdb(void *thread_handle) {
    return;//todo
}

#endif

#ifdef GDB_STUB
static void stall_cpu(void *instance) 
{
    arm_stall = true;
    s.NumInstrsToExecute = 0;
}
static void unstall_cpu(void *instance) 
{
    arm_stall = false;
}
static u32 read_cpu_reg(void *instance, u32 reg_num)
{
    if (reg_num == 0x10)return s.Cpsr;
#ifdef impropergdb
    if (reg_num == 0xF)
    {
        if (s.NextInstr == PRIMEPIPE)return arm11_R(reg_num);
        else return arm11_R(reg_num) - 4;
    }
#else
    if (reg_num == 0xF)
    {
        if (s.NextInstr == PRIMEPIPE)
            return arm11_R(reg_num);
        return arm11_R(reg_num) + 4;
    }
#endif
    return arm11_R(reg_num);
}
static void set_cpu_reg(void *instance, u32 reg_num, u32 value) 
{
    arm11_SetR(reg_num, value);

}
static void install_post_exec_fn(void *instance,void(*ex_fn)(void *, u32 adr, int thumb),void *fn_data) 
{
    //armcpu_t *armcpu = (armcpu_t *)instance;
    s.post_ex_fn = ex_fn;
    s.post_ex_fn_data = fn_data;
}
static void remove_post_exec_fn(void *instance) 
{
    s.post_ex_fn = NULL;
}
static u16 gdb_prefetch16(void *data, u32 adr) {
    return mem_Read16(adr);
}

static u32 gdb_prefetch32(void *data, u32 adr) {
    return mem_Read32(adr);
}

static u8 gdb_read8(void *data, u32 adr) {
    return mem_Read8(adr);
}

static u16 gdb_read16(void *data, u32 adr) {
    return mem_Read16(adr);
}

static u32 gdb_read32(void *data, u32 adr) {
    return mem_Read32(adr);
}

static void gdb_write8(void *data, u32 adr, u8 val) {
    mem_Write8(adr,val);
}

static void gdb_write16(void *data, u32 adr, u16 val) {
    mem_Write16(adr, val);
}

static void gdb_write32(void *data, u32 adr, u32 val) {
    mem_Write32(adr, val);
}
#endif

int main(int argc, char* argv[])
{
    atexit(AtExit);
    if (argc < 2) {
        printf("Usage:\n");
#ifdef modulesupport
        printf("%s <in.ncch> [-d|-noscreen|-codepatch <code>|-modules <num> <in.ncch>|-overdrivlist <num> <services>]\n", argv[0]);
#else
        printf("%s <in.ncch> [-d|-noscreen|-codepatch <code>]\n", argv[0]);
#endif
        return 1;
    }

    //disasm = (argc > 2) && (strcmp(argv[2], "-d") == 0);
    //noscreen =    (argc > 2) && (strcmp(argv[2], "-noscreen") == 0);

    for (int i = 2; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0))disasm = true;
        if ((strcmp(argv[i], "-noscreen") == 0))noscreen = true;
        if ((strcmp(argv[i], "-codepatch") == 0)) {
            i++;
            codepath = malloc(strlen(argv[i]));
            strcpy(codepath, argv[i]);
            i++;
        }
#ifdef GDB_STUB
        if ((strcmp(argv[i], "-gdbport") == 0))
        {
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


#ifdef modulesupport
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
#ifdef modulesupport
    curprocesshandlelist = malloc(sizeof(u32)*(modulenum + 1));
    mem_init(modulenum);
#endif


    signal(SIGINT, AtSig);

    if (!noscreen)
        screen_Init();
    hid_spvr_init();
    hid_user_init();
    initDSP();
    mcu_GPU_init();
    initGPU();
    srv_InitGlobal();


    arm11_Init();

#ifdef modulesupport
    int i;
    for (i = 0; i<modulenum; i++) {
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
#ifdef GDB_STUB
    if (global_gdb_port)
    {
        gdb_stub = createStub_gdb(global_gdb_port,
            &gdb_memio,
            &gdb_base_memory_iface);
        if (gdb_stub == NULL) {
            DEBUG("Failed to create ARM9 gdbstub on port %d\n",
                global_gdb_port);
            exit(-1);
        }
        activateStub_gdb(gdb_stub, &gdb_ctrl_iface);
    }
#endif
    // Execute.
    while (running) {
        if (!noscreen)
            screen_HandleEvent();

#ifdef modulesupport
        int k;
        for (k = 0; k <= modulenum; k++) {
            swapprocess(k);
            DEBUG("process %X\n",k);
#endif
            threads_Execute();
#ifdef modulesupport
        }
#endif

        if (!noscreen)
            screen_RenderGPU();

        FPS_Lock();
        //mem_Dbugdump();
    }


    fclose(fd);
    return 0;
}
