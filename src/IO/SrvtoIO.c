#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "SrvtoIO.h"

u8* IObuffer;
u8* LINEmembuffer;
u8* VRAMbuff;
u8* GSPsharedbuff;

u32 numReqQueue = 1;
void initGPU()
{
    IObuffer = malloc(0x420000);
    LINEmembuffer = malloc(0x8000000);
    VRAMbuff = malloc(0x600000);
    GSPsharedbuff = malloc(GSPsharebuffsize);
    GPUwritereg32(frameselectoben, 0);
    GPUwritereg32(RGBuponeleft, 0x18000000);
    GPUwritereg32(RGBuptwoleft, 0x18046500);
}

void GPUwritereg32(u32 addr, u32 data)
{
    DEBUG("GPU write %08x to %08x\n",data,addr);
    if (addr > 0x420000) {
        DEBUG("write out of range write");
        return;
    }
    *(uint32_t*)(&IObuffer[addr]) = data;
    switch (addr) {
    default:
        break;
    }
}
u32 GPUreadreg32(u32 addr)
{
    DEBUG("GPU read %08x\n", addr);
    if (addr > 0x420000) {
        DEBUG("read out of range write");
        return 0;
    }
    return *(uint32_t*)(&IObuffer[addr]);
}
void GPUTriggerCmdReqQueue() //todo
{
    for (int i = 0; i < 0xFF; i++) { //for all threads
        u32 baseaddr = (u32)(GSPsharedbuff + 0x800 + i * 0x200);
        u32 header = mem_Read32((u32)(baseaddr));
        //Console.WriteLine(Convert.ToString(header,0x10));
        u32 toprocess = (header >> 8) & 0xFF;
        for (u32 j = 0; j < toprocess; j++) {
            mem_Write32(baseaddr, 0);
            u32 CMDID = mem_Read32((u32)(baseaddr + (j + 1) * 0x20));
            u32 src;
            u32 dest;
            u32 size;
            switch (CMDID & 0xFF) {
            case 0:
                src = mem_Read32((u32)(baseaddr + (j + 1) * 0x20 + 0x4));
                dest = mem_Read32((u32)(baseaddr + (j + 1) * 0x20 + 0x8));
                size = mem_Read32((u32)(baseaddr + (j + 1) * 0x20 + 0xC));
                if (dest - 0x1f000000 > 0x600000)DEBUG("dma copy into non VRAM not suported");


                for (u32 k = 0; k < size; k++)
                    VRAMbuff[k + dest - 0x1F000000] = mem_Read8((src + k)); //todo speed up

                break;
            default:
                DEBUG("GX cmd 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X", mem_Read32((baseaddr + (j + 1) * 0x20)), mem_Read32((baseaddr + (j + 1) * 0x20) + 0x4), mem_Read32((baseaddr + (j + 1) * 0x20) + 0x8), mem_Read32((baseaddr + (j + 1) * 0x20) + 0xC), mem_Read32((baseaddr + (j + 1) * 0x20) + 0x10), mem_Read32((baseaddr + (j + 1) * 0x20) + 0x14), mem_Read32((baseaddr + (j + 1) * 0x20) + 0x18), mem_Read32((baseaddr + (j + 1) * 0x20)) + 0x1C);
                break;
            }
        }
    }
}
void GPURegisterInterruptRelayQueue(u32 Flags, u32 Kevent, u32*threadID, u32*outMemHandle)
{
    *threadID = ++numReqQueue;
    *outMemHandle = handle_New(HANDLE_TYPE_SHAREDMEM, 0);
}
u8* get_pymembuffer(u32 addr)
{
    if (addr >= 0x18000000 && addr <= 0x18600000)return VRAMbuff + (addr - 0x18000000);
    if (addr >= 0x20000000 && addr <= 0x28000000)return LINEmembuffer + (addr - 0x20000000);
    return NULL;
}
u32 get_py_memrestsize(u32 addr)
{
    if (addr > 0x18000000 && addr < 0x18600000)return addr - 0x18000000;
    if (addr > 0x20000000 && addr < 0x28000000)return addr - 0x20000000;
    return 0;
}