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

void initGPU()
{
    IObuffer = malloc(0x420000);
	LINEmembuffer = malloc(0x8000000);
	VRAMbuff = malloc(0x600000);
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

}
void GPURegisterInterruptRelayQueue(u32 Flags, u32 Kevent, u32*threadID, u32*outMemHandle)
{

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