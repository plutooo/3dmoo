#include "util.h"

#define MAX_IO_REGS 0x420000
static u32 io_regs[MAX_IO_REGS/4];


void gpu_WriteReg32(u32 addr, u32 data)
{
    //GPUDEBUG("w32 %08x to %08x\n",data, addr);

    if(addr >= 0x1EB00000 && addr < (0x1EB00000 + 0x420000))
    {
        DEBUG("Write to %08X from physical address %08X\n", addr - 0x1EB00000, addr);
        addr -= 0x1EB00000;
    }

    if(addr >= MAX_IO_REGS) {
        GPUDEBUG("Write to %08x: out of range\n", addr);
        return;
    }

    io_regs[addr/4] = data;
}


u32 gpu_ReadReg32(u32 addr)
{
    //GPUDEBUG("r32 %08x\n", addr);

    if(addr >= MAX_IO_REGS) {
        GPUDEBUG("Read to %08x: out of range write\n", addr);
        return 0;
    }

    return io_regs[addr/4];
}
