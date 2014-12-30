/*
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
#include "mem.h"
#include "handles.h"
#include "arm11.h"
#include "gpu.h"

extern u32 modulenum;


static u32 Read32(uint8_t p[4])
{
    u32 temp = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
    return temp;
}

u32 svcCreateCodeSet() //Result CreateCodeSet(Handle* handle_out, struct CodeSetInfo, u32 code_ptr, u32 ro_ptr, u32 data_ptr) 
{
    u32 CodeSetInfo = arm11_R(1);
    u32 code_ptr = arm11_R(2);
    u32 ro_ptr = arm11_R(3);
    u32 data_ptr = arm11_R(0);

    DEBUG("CreateCodeSet code=%08x ro=%08x data=%08x --stub--\n", code_ptr, ro_ptr, data_ptr);

    char name[9];
    mem_Read(name, CodeSetInfo, 8);

    name[8] = '\0';

    printf("name: %s\n", name);
    CodeSetInfo += 8;
    printf("unk %08x %08x\n", mem_Read32(CodeSetInfo), mem_Read32(CodeSetInfo + 4));
    CodeSetInfo += 8;

    u32 textsize = mem_Read32(CodeSetInfo + 4);
    printf("text: %08x (%08x)\n", mem_Read32(CodeSetInfo), textsize);
    CodeSetInfo += 8;

    u32 rosize = mem_Read32(CodeSetInfo + 4);
    printf("ro: %08x (%08x)\n", mem_Read32(CodeSetInfo), rosize);
    CodeSetInfo += 8;

    u32 datasize = mem_Read32(CodeSetInfo + 4);
    printf("data: %08x (%08x)\n", mem_Read32(CodeSetInfo), datasize);
    CodeSetInfo += 8;

    u32 rooffset = mem_Read32(CodeSetInfo);
    u32 dataoffset = mem_Read32(CodeSetInfo + 4);
    printf("offsets: ro=%08x data=%08x\n", rooffset, dataoffset);
    CodeSetInfo += 8;

    u32 dbsize = mem_Read32(CodeSetInfo);
    printf("bss+data size:%08x\n", dbsize);
    CodeSetInfo += 4;

    printf("pid?:%02x%02x%02x%02x%02x%02x%02x%02x\n", mem_Read8(CodeSetInfo), mem_Read8(CodeSetInfo + 1), mem_Read8(CodeSetInfo + 2), mem_Read8(CodeSetInfo + 3), mem_Read8(CodeSetInfo + 4), mem_Read8(CodeSetInfo + 5), mem_Read8(CodeSetInfo + 6), mem_Read8(CodeSetInfo + 7));
    CodeSetInfo += 8;

    printf("unk %08x %08x\n", mem_Read32(CodeSetInfo), mem_Read32(CodeSetInfo + 4));

    u32 handle = handle_New(HANDLE_TYPE_CODESET, 0);

    handleinfo* h = handle_Get(handle);

    if (h == NULL) {
        DEBUG("failed to get handle\n");
        PAUSE();
        return -1;
    }
    h->misc_ptr[0] = (void*)malloc(0x40);
    mem_Read(h->misc_ptr[0], arm11_R(1), 0x40);

    h->misc_ptr[1] = (void*)malloc(textsize * 0x1000);
    h->misc_ptr[2] = (void*)malloc(rosize * 0x1000);
    h->misc_ptr[3] = (void*)malloc(dbsize * 0x1000);



    mem_Read(h->misc_ptr[1], code_ptr, textsize * 0x1000);
    mem_Read(h->misc_ptr[2], ro_ptr, rosize * 0x1000);
    mem_Read(h->misc_ptr[3], data_ptr, datasize * 0x1000);

    arm11_SetR(1, handle);

    return 0;
}
u32 svcCreateProcess()
{
    u32 codeset_handle = arm11_R(1);
    u32 arm11kernelcaps_ptr = arm11_R(2);
    u32 arm11kernelcaps_num = arm11_R(3);

    DEBUG("CreateProcess %08x %08x %08x --stub--\n", codeset_handle, arm11kernelcaps_ptr, arm11kernelcaps_num);

    u32 handle = handle_New(HANDLE_TYPE_PROCESS, 0);
    handleinfo* hi = handle_Get(handle);

    modulenum++;
    if (hi == NULL)
    {
        DEBUG("out of handles\n");
        return -1;
    }
    hi->misc[0] = modulenum;
    curprocesshandlelist = realloc(curprocesshandlelist, sizeof(u32)*(modulenum + 1));
    ModuleSupport_Memadd(modulenum, codeset_handle);




    unsigned int i, j;
    unsigned int systemcallmask[8];
    unsigned int unknowndescriptor[28];
    unsigned int svccount = 0;
    unsigned int svcmask = 0;
    unsigned int interrupt[0x80];
    unsigned int interruptcount = 0;
    memset(systemcallmask, 0, sizeof(systemcallmask));
    memset(interrupt, 0, sizeof(interrupt));

    for (i = 0; i < arm11kernelcaps_num; i++)
    {
        unsigned int descriptor = mem_Read32(arm11kernelcaps_ptr + i * 4);

        unknowndescriptor[i] = 0;

        if ((descriptor & (0x1f << 27)) == (0x1e << 27))
            systemcallmask[(descriptor >> 24) & 7] = descriptor & 0x00FFFFFF;
        else if ((descriptor & (0x7f << 25)) == (0x7e << 25))
            fprintf(stdout, "Kernel release version: %d.%d\n", (descriptor >> 8) & 0xFF, (descriptor >> 0) & 0xFF);
        else if ((descriptor & (0xf << 28)) == (0xe << 28))
        {
            for (j = 0; j < 4; j++)
                interrupt[(descriptor >> (j * 7)) & 0x7F] = 1;
        }
        else if ((descriptor & (0xff << 24)) == (0xfe << 24))
            fprintf(stdout, "Handle table size:      0x%X\n", descriptor & 0x3FF);
        else if ((descriptor & (0xfff << 20)) == (0xffe << 20))
        {
            mem_AddMappingIO((descriptor & 0xFFFFF) << 12, 1 << 12, modulenum);
            fprintf(stdout, "Mapping IO address:     0x%X (%s)\n", (descriptor & 0xFFFFF) << 12, (descriptor&(1 << 20)) ? "RO" : "RW");
        }
        else if ((descriptor & (0x7ff << 21)) == (0x7fc << 21))
            fprintf(stdout, "Mapping static address: 0x%X (%s)\n", (descriptor & 0x1FFFFF) << 12, (descriptor&(1 << 20)) ? "RO" : "RW");
        else if ((descriptor & (0x1ff << 23)) == (0x1fe << 23))
        {
            unsigned int memorytype = (descriptor >> 8) & 15;
            fprintf(stdout, "Kernel flags:           \n");
            fprintf(stdout, " > Allow debug:         %s\n", (descriptor&(1 << 0)) ? "YES" : "NO");
            fprintf(stdout, " > Force debug:         %s\n", (descriptor&(1 << 1)) ? "YES" : "NO");
            fprintf(stdout, " > Allow non-alphanum:  %s\n", (descriptor&(1 << 2)) ? "YES" : "NO");
            fprintf(stdout, " > Shared page writing: %s\n", (descriptor&(1 << 3)) ? "YES" : "NO");
            fprintf(stdout, " > Privilege priority:  %s\n", (descriptor&(1 << 4)) ? "YES" : "NO");
            fprintf(stdout, " > Allow main() args:   %s\n", (descriptor&(1 << 5)) ? "YES" : "NO");
            fprintf(stdout, " > Shared device mem:   %s\n", (descriptor&(1 << 6)) ? "YES" : "NO");
            fprintf(stdout, " > Runnable on sleep:   %s\n", (descriptor&(1 << 7)) ? "YES" : "NO");
            fprintf(stdout, " > Special memory:      %s\n", (descriptor&(1 << 12)) ? "YES" : "NO");


            switch (memorytype)
            {
            case 1: fprintf(stdout, " > Memory type:         APPLICATION\n"); break;
            case 2: fprintf(stdout, " > Memory type:         SYSTEM\n"); break;
            case 3: fprintf(stdout, " > Memory type:         BASE\n"); break;
            default: fprintf(stdout, " > Memory type:         Unknown (%d)\n", memorytype); break;
            }
        }
        else if (descriptor != 0xFFFFFFFF)
            unknowndescriptor[i] = 1;
    }

    fprintf(stdout, "Allowed systemcalls:    ");
    for (i = 0; i<8; i++)
    {
        for (j = 0; j<24; j++)
        {
            svcmask = systemcallmask[i];

            if (svcmask & (1 << j))
            {
                unsigned int svcid = i * 24 + j;
                if (svccount == 0)
                {
                    fprintf(stdout, "0x%02X", svcid);
                }
                else if ((svccount & 7) == 0)
                {
                    fprintf(stdout, "                        ");
                    fprintf(stdout, "0x%02X", svcid);
                }
                else
                {
                    fprintf(stdout, ", 0x%02X", svcid);
                }

                svccount++;
                if ((svccount & 7) == 0)
                {
                    fprintf(stdout, "\n");
                }
            }
        }
    }
    if (svccount & 7)
        fprintf(stdout, "\n");
    if (svccount == 0)
        fprintf(stdout, "none\n");


    fprintf(stdout, "Allowed interrupts:     ");
    for (i = 0; i<0x7F; i++)
    {
        if (interrupt[i])
        {
            if (interruptcount == 0)
            {
                fprintf(stdout, "0x%02X", i);
            }
            else if ((interruptcount & 7) == 0)
            {
                fprintf(stdout, "                        ");
                fprintf(stdout, "0x%02X", i);
            }
            else
            {
                fprintf(stdout, ", 0x%02X", i);
            }

            interruptcount++;
            if ((interruptcount & 7) == 0)
            {
                fprintf(stdout, "\n");
            }
        }
    }
    if (interruptcount & 7)
        fprintf(stdout, "\n");
    if (interruptcount == 0)
        fprintf(stdout, "none\n");

    for (i = 0; i<arm11kernelcaps_num; i++)
    {
        unsigned int descriptor = mem_Read32(arm11kernelcaps_ptr + i * 4);

        if (unknowndescriptor[i])
            fprintf(stdout, "Unknown descriptor:     %08X\n", descriptor);
    }



    arm11_SetR(1, handle);

    return 0;
}
u32 process_WaitSynchronization(handleinfo* h, bool *locked)
{
    DEBUG("process wait --stub-- -should unlock when the process ends-\n");
    *locked = 1;
    return 0;
}
u32 svcRun()
{
    u32 handle = arm11_R(0);
    u32 prio = arm11_R(1);
    u32 stacksize = arm11_R(2); //todo
    u32 argc = arm11_R(3);
    u32 argv = arm11_R(4);
    u32 envp = arm11_R(5);
    handleinfo* hi = handle_Get(handle);
    if (hi == NULL)
    {
        DEBUG("wrong handle\n");
        return -1;
    }
    u8* stack =malloc(stacksize);
    ModuleSupport_mem_AddMappingShared(0x10000000 - stacksize,stacksize, stack, hi->misc[0]);

    u8* tprif = malloc(0x1000 * (MAX_THREADS + 1));
    ModuleSupport_mem_AddMappingShared(0xFFFF0000 - 0x1000 * MAX_THREADS, 0x1000 * (MAX_THREADS + 1), tprif, hi->misc[0]);

    ModuleSupport_threads_New(handle_New(HANDLE_TYPE_THREAD, 0), hi->misc[0], 0x00100000, 0x10000000, argc, prio);
    DEBUG("RUN %08X %08X %08X %08X %08X %08X\n", handle, prio, stacksize, argc, argv, envp);
    return 0;
}