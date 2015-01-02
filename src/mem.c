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
#include "handles.h"

#include "armdefs.h"
#include "armemu.h"
#include "threads.h"
#include "gpu.h"

#include "mem.h"

#ifdef GDB_STUB
#include "gdb/gdbstub.h"
#include "gdb/gdbstub_internal.h"
#include "IO/IO_reg.h"

extern struct armcpu_memory_iface *gdb_memio;
#endif

extern ARMul_State s;
extern u8* loader_Shared_page;
void ModuleSupport_Threadsadd(u32 modulenum);


memmap_t mappings[MAX_MAPPINGS];
size_t   num_mappings;

extern u32 main_current_module;

#define MEM_TRACE_COND main_current_module == 3

//#define MEM_TRACE 1
#define PRINT_ILLEGAL 1
#define EXIT_ON_ILLEGAL 1


#ifdef MODULE_SUPPORT


typedef struct {
    u8 name[8];
    u8 unk[8];
    u8 textaddr[4];
    u8 textsize[4];
    u8 roaddr[4];
    u8 rosize[4];
    u8 dataaddr[4];
    u8 datasize[4];
    u8 rooffset[4];
    u8 dataoffset[4];
    u8 bdsize[4];
    u8 pid[8];
    u8 unk2[8];
} ctr_CodeSetInfo;

memmap_t **mappingsproc;
size_t*   num_mappingsproc;
u32 currentmap = 0;

void ModuleSupport_Memadd(u32 modulenum, u32 codeset_handle)
{
    mappingsproc = (memmap_t **)realloc(mappingsproc, sizeof(memmap_t *)*(modulenum + 1));

    *(mappingsproc + modulenum) = (memmap_t *)malloc(sizeof(memmap_t)*(MAX_MAPPINGS));

    num_mappingsproc = (size_t*)realloc(num_mappingsproc, sizeof(size_t*)*(modulenum + 1));
    memset((num_mappingsproc + modulenum), 0, sizeof(size_t*));

    ModuleSupport_Threadsadd(modulenum);

    handleinfo* hi = handle_Get(codeset_handle);
    if (hi == NULL)
    {
        ERROR("can't read codeset");
        return;
    }
    ctr_CodeSetInfo* co = hi->misc_ptr[0];
    ModuleSupport_mem_AddMappingShared(Read32(co->textaddr), Read32(co->textsize) * 0x1000, hi->misc_ptr[1], modulenum, PERM_RX, STAT_PRIVAT);
    ModuleSupport_mem_AddMappingShared(Read32(co->roaddr), Read32(co->rosize) * 0x1000, hi->misc_ptr[2], modulenum, PERM_R, STAT_PRIVAT);
    ModuleSupport_mem_AddMappingShared(Read32(co->dataaddr), Read32(co->bdsize) * 0x1000, hi->misc_ptr[3], modulenum, PERM_RW, STAT_PRIVAT);
    ModuleSupport_mem_AddMappingShared(0x1FF80000, 0x2000, loader_Shared_page, modulenum, PERM_RW, STAT_SHARED);
}

void ModuleSupport_MemInit(u32 modulenum)
{
    mappingsproc = (memmap_t **)malloc(sizeof(memmap_t *)*(modulenum + 1));

    u32 i;
    for (i = 0; i < (modulenum + 1); i++) {
        *(mappingsproc + i) = (memmap_t *)malloc(sizeof(memmap_t)*(MAX_MAPPINGS));
    }

    num_mappingsproc = (size_t*)malloc(sizeof(size_t*)*(modulenum + 1));
    memset(num_mappingsproc, 0, sizeof(size_t*)*(modulenum + 1));

    ModuleSupport_ThreadsInit(modulenum);
}

void ModuleSupport_SwapProcessMem(u32 newproc)
{
    memcpy(*(mappingsproc + currentmap), mappings, sizeof(memmap_t)*(MAX_MAPPINGS)); //save maps
    *(num_mappingsproc + currentmap) = num_mappings;

    memcpy(mappings, *(mappingsproc + newproc), sizeof(memmap_t)*(MAX_MAPPINGS)); //load maps
    num_mappings = *(num_mappingsproc + newproc);

    ModuleSupport_SwapProcessThreads(newproc);
    currentmap = newproc;
}
int ModuleSupport_mem_AddMappingShared(uint32_t base, uint32_t size, u8* data, u32 process, u32 perm, u32 status)
{
    if (size == 0)
        return 0;
    memmap_t mappings[MAX_MAPPINGS];
    memcpy(mappings, *(mappingsproc + process), sizeof(memmap_t)*(MAX_MAPPINGS)); //load maps
    u32 num_mappings = *(num_mappingsproc + process);

    if (num_mappings == MAX_MAPPINGS) {
        ERROR("too many mappings.\n");
        return 1;
    }

    size_t i = num_mappings, j;
    mappings[i].base = base;
    mappings[i].size = size;
    mappings[i].accesses = 0;
    mappings[i].permition = perm;
    mappings[i].State = status;

    for (j = 0; j<num_mappings; j++) {
        if (Overlaps(&mappings[j], &mappings[i])) {
            ERROR("trying to add overlapping mapping %08x, size=%08x.\n",
                base, size);
            return 2;
        }
    }

    mappings[i].phys = data;

#ifdef MEM_TRACE_EXTERNAL
    if (
        (base & 0xFFFF0000) == 0x1FF80000
        || base == 0x10000000
        || base == 0x14000000
        //|| base == 0x10002000
        ) {
        mappings[i].enable_log = true;
    }
    else {
        mappings[i].enable_log = false;
    }
#endif

    num_mappings++;

    memcpy(*(mappingsproc + process), mappings, sizeof(memmap_t)*(MAX_MAPPINGS)); //save maps
    *(num_mappingsproc + process) = num_mappings;

    return 0;
}
int mem_AddMappingIO(uint32_t base, uint32_t size, u32 process, u32 perm)

{

    memmap_t mappings[MAX_MAPPINGS];
    memcpy(mappings, *(mappingsproc + process), sizeof(memmap_t)*(MAX_MAPPINGS)); //load maps
    u32 num_mappings = *(num_mappingsproc + process);

    if (size == 0)
        return 0;

    if (num_mappings == MAX_MAPPINGS) {
        ERROR("too many mappings.\n");
        return 1;
    }

    size_t i = num_mappings, j;
    mappings[i].base = base;
    mappings[i].size = size;
    mappings[i].accesses = 0;
    mappings[i].permition = perm;
    mappings[i].State = 2; //IO

    for (j = 0; j<num_mappings; j++) {
        if (Overlaps(&mappings[j], &mappings[i])) {
            ERROR("trying to add overlapping --replace-- mapping %08x, size=%08x.\n",
                base, size);
            return 2;
        }
    }


#ifdef MEM_TRACE_EXTERNAL
    if (
        (base & 0xFFFF0000) == 0x1FF80000
        || base == 0x10000000
        || base == 0x14000000
        //|| base == 0x10002000
        ) {
        mappings[i].enable_log = true;
    }
    else {
        mappings[i].enable_log = false;
    }
#endif

    num_mappings++;

    memcpy(*(mappingsproc + process), mappings, sizeof(memmap_t)*(MAX_MAPPINGS)); //save maps
    *(num_mappingsproc + process) = num_mappings;

    return 0;
}

#endif


void mem_Dbugdump()
{
    size_t i;
    char name[0x200];
    for (i = 0; i<num_mappings; i++) {
        u32 schei = mappings[i].size;
        sprintf(name, "dump%08X %08X.bin", mappings[i].base, schei);
        FILE* data = fopen(name, "wb");
        fwrite(mappings[i].phys, 1, schei, data);
        fclose(data);
    }
    FILE* data = fopen("VRAMdump.bin", "wb");
    fwrite(VRAMbuff, 1, 0x600000, data);
    fclose(data);

    FILE* membuf = fopen("LINEmembuffer.bin", "wb");
    fwrite(LINEmembuffer, 1, 0x8000000, membuf);
    fclose(membuf);
}

static int Overlaps(memmap_t* a, memmap_t* b)
{
    if(a->base <= b->base && b->base < (a->base+a->size))
        return 1;
    if(a->base < (b->base+b->size) && (b->base+b->size) < (a->base+a->size))
        return 1;
    return 0;
}

static int Contains(memmap_t* m, uint32_t addr, uint32_t sz)
{
    return (m->base <= addr && (addr+sz) <= (m->base+m->size));
}

static int AddMapping(uint32_t base, uint32_t size, u32 perm, u32 status)
{
    if(size == 0)
        return 0;

    if(num_mappings == MAX_MAPPINGS) {
        ERROR("too many mappings.\n");
        return 1;
    }

    size_t i = num_mappings, j;
    mappings[i].base = base;
    mappings[i].size = size;
    for(j=0; j<num_mappings; j++) {
        if(Overlaps(&mappings[j], &mappings[i])) {

            if (mappings[i].base == mappings[j].base) //this is a realloc
            {
                mappings[j].size = size;
                mappings[j].permition = perm;
                mappings[j].State = status;
                mappings[j].phys = realloc(mappings[j].phys, size);
                return 0;
            }

            ERROR("trying to add overlapping mapping %08x, size=%08x.\n",
                  base, size);
            return 2;
        }
    }

    mappings[i].accesses = 0; 
    mappings[i].permition = perm;
    mappings[i].State = status;



    mappings[i].phys = calloc(sizeof(uint8_t), size);
    if(mappings[i].phys == NULL) {
        ERROR("calloc failed for %08x, size=%08x\n", base, size);
        return 3;
    }

#ifdef MEM_TRACE_EXTERNAL
    if (
        (base & 0xFFFF0000) == 0x1FF80000
        || base == 0x10000000
        || base == 0x14000000
        //|| base == 0x10002000
    ) {
        mappings[i].enable_log = true;
    } else {
        mappings[i].enable_log = false;
    }
#endif

    num_mappings++;
    return 0;
}

int mem_AddMappingShared(uint32_t base, uint32_t size, u8* data,u32 perm,u32 status)
{
    if (size == 0)
        return 0;

    if (num_mappings == MAX_MAPPINGS) {
        ERROR("too many mappings.\n");
        return 1;
    }

    size_t i = num_mappings, j;

    mappings[i].base = base;
    mappings[i].size = ((size +3)/4)*4; //aline

    bool ovelap = false;

    for (j = 0; j<num_mappings; j++) {
        if (Overlaps(&mappings[j], &mappings[i])) {
            ERROR("trying to add overlapping mapping overwriting %08x, size=%08x.\n",base, size);
            ovelap = true;
            i = j;
            break;
        }
    }

    mappings[i].base = base;
    mappings[i].size = ((size + 3) / 4) * 4; //aline
    mappings[i].accesses = 0;
    mappings[i].phys = data;
    mappings[i].permition = perm;
    mappings[i].State = status;

#ifdef MEM_TRACE_EXTERNAL
    if (
        (base & 0xFFFF0000) == 0x1FF80000
        || base == 0x10000000
        || base == 0x14000000
        //|| base == 0x10002000
    ) {
        mappings[i].enable_log = true;
    } else {
        mappings[i].enable_log = false;
    }
#endif
    if (!ovelap)
        num_mappings++;
    return 0;
}

int mem_AddSegment(uint32_t base, uint32_t size, uint8_t* data,u32 perm, u32 status)
{
    DEBUG("adding %08x %08x\n", base, size);
    int rc;

    rc = AddMapping(base, size, perm, status);
    if(rc != 0)
        return rc;

    if(data != NULL)
        memcpy(mappings[num_mappings - 1].phys, data, size);
    return 0;
}

int mem_Write8(uint32_t addr, uint8_t w)
{

    if ((addr & 0xFFFF0000) == 0x1FF80000) { //wrong
        ERROR("ARM11 kernel write %08x %02X\n", addr,w);
    }

#ifdef MEM_TRACE
    if (MEM_TRACE_COND)
    {
        fprintf(stderr, "w8 %08x <- w=%02x\n", addr, w & 0xff);
    }
#endif

    size_t i;
    for(i=0; i<num_mappings; i++) {
        if(Contains(&mappings[i], addr, 1)) {
#ifdef MEM_REORDER
            mappings[i].accesses++;
#endif
#ifdef MEM_TRACE_EXTERNAL
            if (mappings[i].enable_log)fprintf(stderr, "w8 %08x <- w=%02x pc=%08x\n", addr, w & 0xff, s.Reg[15]);
#endif
            if (mappings[i].State == 2){
                IO_Write8(addr, w);
            return;
        }
            else
                mappings[i].phys[addr - mappings[i].base] = w;
            return 0;
        }
    }

#ifdef PRINT_ILLEGAL
    ERROR("trying to write8 unmapped addr %08x, w=%02x\n", addr, w & 0xff);
    arm11_Dump();
#endif
#ifdef EXIT_ON_ILLEGAL
    exit(1);
#endif
    return 1;
}

uint8_t mem_Read8(uint32_t addr)
{
    if ((addr & 0xFFFF0000) == 0x1FF80000) { //wrong
        ERROR("ARM11 kernel read 8 %08x\n", addr);
    }
#ifdef MEM_TRACE
    if (MEM_TRACE_COND)
    {
        fprintf(stderr, "r8 %08x\n", addr);
    }
#endif

    size_t i;

    for(i=0; i<num_mappings; i++) {
        if(Contains(&mappings[i], addr, 1)) {
#ifdef MEM_REORDER
            mappings[i].accesses++;
#endif
#ifdef MEM_TRACE_EXTERNAL
            if (mappings[i].enable_log)fprintf(stderr, "r8 %08x pc=%08x\n", addr, s.Reg[15]);
#endif
            if (mappings[i].State == 2)
                return IO_Read8(addr);
            else
                return mappings[i].phys[addr - mappings[i].base];
        }
    }
#ifdef PRINT_ILLEGAL
    ERROR("trying to read8 unmapped addr %08x\n", addr);
    arm11_Dump();
#endif
#ifdef EXIT_ON_ILLEGAL
    exit(1);
#endif
    return 0;
}

int mem_Write16(uint32_t addr, uint16_t w)
{
    if ((addr & 0xFFFF0000) == 0x1FF80000) { //wrong
        ERROR("ARM11 kernel write %08x %04X\n", addr, w);
    }

#ifdef MEM_TRACE
    if (MEM_TRACE_COND)
    {
        fprintf(stderr, "w16 %08x <- w=%04x\n", addr, w & 0xffff);
    }
#endif

    size_t i;
    for(i=0; i<num_mappings; i++) {
        if(Contains(&mappings[i], addr, 2)) {
#ifdef MEM_REORDER
            mappings[i].accesses += 2;
#endif
#ifdef MEM_TRACE_EXTERNAL
            if (mappings[i].enable_log)fprintf(stderr, "w16 %08x <- w=%04x pc=%08x\n", addr, w & 0xffff, s.Reg[15]);
#endif
            // Unaligned.
            if (mappings[i].State == 2) {
                IO_Write16(addr, w);
            return;
        }
            else
            {
                if (addr & 1) {
                    mappings[i].phys[addr - mappings[i].base] = (u8)w;
                    mappings[i].phys[addr - mappings[i].base + 1] = (u8)(w >> 8);
                }
                else
                    *(uint16_t*)(&mappings[i].phys[addr - mappings[i].base]) = w;
            }
            return 0;
        }
    }

#ifdef PRINT_ILLEGAL
    ERROR("trying to write16 unmapped addr %08x, w=%04x\n", addr, w & 0xffff);
    arm11_Dump();
#endif
#ifdef EXIT_ON_ILLEGAL
    exit(1);
#endif
    return 1;
}

uint16_t mem_Read16(uint32_t addr)
{

    if ((addr & 0xFFFF0000) == 0x1FF80000) { //wrong
        ERROR("ARM11 kernel read 16 %08x\n", addr);
    }
#ifdef MEM_TRACE
    if (MEM_TRACE_COND)
    {
        fprintf(stderr, "r16 %08x\n", addr);
    }
#endif

    size_t i;
    for(i=0; i<num_mappings; i++) {
        if(Contains(&mappings[i], addr, 2)) {

#ifdef MEM_REORDER
            mappings[i].accesses += 2;
#endif

#ifdef MEM_TRACE_EXTERNAL
            if (mappings[i].enable_log)fprintf(stderr, "r16 %08x pc=%08x\n", addr, s.Reg[15]);
#endif

            if (mappings[i].State == 2)
                return IO_Read16(addr);

            else
            // Unaligned.
            if (addr & 1) {
                uint16_t ret = mappings[i].phys[addr - mappings[i].base + 1] << 8;
                ret |= mappings[i].phys[addr - mappings[i].base];
                return ret;
            }

                return *(uint16_t*) (&mappings[i].phys[addr - mappings[i].base]);
        }
    }

#ifdef PRINT_ILLEGAL
    ERROR("trying to read16 unmapped addr %08x\n", addr);
    arm11_Dump();
#endif
#ifdef EXIT_ON_ILLEGAL
    exit(1);
#endif
    return 0;
}

int mem_Write32(uint32_t addr, uint32_t w)
{

    if ((addr & 0xFFFF0000) == 0x1FF80000) { //wrong
        ERROR("ARM11 kernel write %08x %08X\n", addr, w);
    }

    if (addr > 0xFFFFFFF0) { //wrong
        ERROR("trying to write32 unmapped addr %08x, w=%08x\n", addr, w);
        arm11_Dump();
        return 0;
    }
#ifdef MEM_TRACE
    if (MEM_TRACE_COND)
    {
        fprintf(stderr, "w32 %08x <- w=%08x\n", addr, w);
    }
#endif
    size_t i;
    for(i=0; i<num_mappings; i++) {
        if (Contains(&mappings[i], addr, 4)) {

#ifdef MEM_REORDER
            mappings[i].accesses += 4;
#endif

#ifdef MEM_TRACE_EXTERNAL
            if (mappings[i].enable_log)
                fprintf(stderr, "w32 %08x <- w=%08x pc=%08x\n", addr, w, s.Reg[15]);
#endif

            if (mappings[i].State == 2){
                IO_Write32(addr, w);
                return;
        }
            else
            {
                // Unaligned.
                if (addr & 3) {
                    mappings[i].phys[addr - mappings[i].base] = w;
                    mappings[i].phys[addr - mappings[i].base + 1] = w >> 8;
                    mappings[i].phys[addr - mappings[i].base + 2] = w >> 16;
                    mappings[i].phys[addr - mappings[i].base + 3] = w >> 24;
                }
                else
                    *(uint32_t*)(&mappings[i].phys[addr - mappings[i].base]) = w;
                return 0;
            }
        }
    }
#ifdef PRINT_ILLEGAL
    ERROR("trying to write32 unmapped addr %08x, w=%08x\n", addr, w);
    arm11_Dump();
#endif
#ifdef EXIT_ON_ILLEGAL
    exit(1);
#endif
    return 0;
}

bool mem_test(uint32_t addr)
{
    size_t i;

    for (i = 0; i<num_mappings; i++) {
        if (Contains(&mappings[i], addr, 4)) {
            return true;
        }
    }
    return false;
}

u32 mem_Read32(uint32_t addr)
{
    if ((addr & 0xFFFF0000) == 0x1FF80000) { //wrong
        ERROR("ARM11 kernel read 32 %08x\n", addr);
    }
    size_t i;
    for(i=0; i<num_mappings; i++) {
        if(Contains(&mappings[i], addr, 4)) {

#ifdef MEM_REORDER
            mappings[i].accesses += 4;
#endif

#ifdef MEM_TRACE_EXTERNAL
            if (mappings[i].enable_log) {
                fprintf(stderr, "r32 %08x pc=%08x\n", addr, s.Reg[15]);
                //arm11_Dump();
            }
#endif

            if (mappings[i].State == 2)
                return IO_Read32(addr);
            else
            {
            // Unaligned.
            u32 temp = *(uint32_t*)(&mappings[i].phys[addr - mappings[i].base]);
#ifdef MEM_TRACE
            if (MEM_TRACE_COND)
            {
                if (addr == 0x110688)
                {
                    int i = 0;
                }
                fprintf(stderr, "r32 %08x --> %08x (%08X)\n", addr, temp, s.Reg[15]);
            }
#endif

                switch (addr & 3) {
                case 0:
                    return temp;
                case 1:
                    return (temp & 0xFF) << 8 | ((temp & 0xFF00) >> 8) << 16 | ((temp & 0xFF0000) >> 16) << 24 | ((temp & 0xFF000000) >> 24) << 0;
                case 2:
                    return (temp & 0xFFFF) << 16 | (temp & 0xFFFF0000) >> 16;
                case 3:
                    return (temp & 0xFF) << 24 | ((temp & 0xFF00) >> 8) << 0 | ((temp & 0xFF0000) >> 16) << 8 | ((temp & 0xFF000000) >> 24) << 16;
                }
            }
        }
    }

#ifdef PRINT_ILLEGAL
    ERROR("trying to read32 unmapped addr %08x\n", addr);
    arm11_Dump();
    //mem_Dbugdump();
#endif
#ifdef EXIT_ON_ILLEGAL
    exit(1);
#endif
    return 0;
}

int mem_Write(const uint8_t* in_buff, uint32_t addr, uint32_t size)
{
#ifdef MEM_TRACE
    fprintf(stderr, "w (sz=%08x) %08x\n", size, addr);
#endif

    size_t i;
    uint32_t map = 0xdeadc0de;
    for (i = 0; i<num_mappings; i++) {
        if (Contains(&mappings[i], addr, size)) {
#ifdef MEM_REORDER
            mappings[i].accesses += size;
#endif
            memcpy(&mappings[i].phys[addr - mappings[i].base],in_buff, size);
            return 0;
        } else if (Contains(&mappings[i], addr, 1)) {
            map = i;
        }
    }

#ifdef MEM_REORDER
    mappings[i].accesses += size;
#endif

    //If spread across multiple mappings
    if (map != 0xdeadc0de) {
        uint32_t base = mappings[map].base;
        uint32_t base_size = mappings[map].size;
        uint32_t part2_size = (addr + size) - (base + base_size);
        uint32_t part1_size = size - part2_size;
        mem_Write(in_buff, addr, part1_size);
        mem_Write(in_buff + part1_size, addr + part1_size, part2_size);
        return 0;
    }

#ifdef PRINT_ILLEGAL
    ERROR("trying to write 0x%x bytes unmapped addr %08x\n", size, addr);
    arm11_Dump();
#endif
#ifdef EXIT_ON_ILLEGAL
    exit(1);
#endif
    return -1;
}

int mem_Read(uint8_t* buf_out, uint32_t addr, uint32_t size)
{
#ifdef MEM_TRACE
    fprintf(stderr, "r (sz=%08x) %08x\n", size, addr);
#endif

    size_t i;
    uint32_t map = 0xdeadc0de;
    for(i=0; i<num_mappings; i++) {
        if(Contains(&mappings[i], addr, size)) {
#ifdef MEM_REORDER
            mappings[i].accesses += size;
#endif
            memcpy(buf_out, &mappings[i].phys[addr - mappings[i].base], size);
            return 0;
        } else if (Contains(&mappings[i], addr, 1)) {
            map = i;
        }
    }

#ifdef MEM_REORDER
    mappings[map].accesses += size;
#endif

    //If spread across multiple mappings
    if (map != 0xdeadc0de) {
        uint32_t base = mappings[map].base;
        uint32_t base_size = mappings[map].size;
        uint32_t part2_size = (addr + size) - (base + base_size);
        uint32_t part1_size = size - part2_size;
        mem_Read(buf_out, addr, part1_size);
        mem_Read(buf_out + part1_size, addr + part1_size, part2_size);
        return 0;
    }

#ifdef PRINT_ILLEGAL
    ERROR("trying to read 0x%x bytes unmapped addr %08x\n", size, addr);
    arm11_Dump();
#endif
#ifdef EXIT_ON_ILLEGAL
    exit(1);
#endif
    return -1;
}

u8* mem_rawaddr(uint32_t addr, uint32_t size)
{
#ifdef MEM_TRACE
    fprintf(stderr, "r (sz=%08x) %08x\n", size, addr);
#endif

    size_t i;
    for (i = 0; i<num_mappings; i++) {
        if (Contains(&mappings[i], addr, size)) {
#ifdef MEM_REORDER
            mappings[i].accesses++;
#endif
            return (u8*)&mappings[i].phys[addr - mappings[i].base];
        }
    }
#ifdef PRINT_ILLEGAL
    ERROR("trying to remap 0x%x bytes unmapped addr %08x\n", size, addr);
    arm11_Dump();
#endif
#ifdef EXIT_ON_ILLEGAL
    exit(1);
#endif
    return NULL;
}

#ifdef MEM_REORDER
//Reorder the mappings according to number of accesses using selection sort
//Should be called once per frame?
void mem_Reorder()
{
    size_t i;
    for(i = 0; i < num_mappings - 1; ++i)
    {
        size_t j, max;
        memmap_t temp;
        max = i;
        for(j = i + 1; j < num_mappings; ++j)
        {
            if(mappings[j].accesses > mappings[max].accesses)
                max = j;
        }

        memcpy(&temp, &mappings[i], sizeof(memmap_t));
        memcpy(&mappings[i], &mappings[max], sizeof(memmap_t));
        memcpy(&mappings[max], &temp, sizeof(memmap_t));
    }

    //Reset accesses because the most used could be different
    for(i = 0; i < num_mappings; i++)
        mappings[i].accesses = 0;
}
#endif
