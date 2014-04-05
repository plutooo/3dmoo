#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"

u8* IObuffer;
void initGPU()
{
	IObuffer = malloc(0x420000);
}

void GPUwritereg32(u32 addr, u32 data)
{
	DEBUG("GPU write %08x to %08x",data,addr);
	if (addr > 0x420000)
	{
		DEBUG("write out of range write");
		return;
	}
	*(uint32_t*)(&IObuffer[addr]) = data;
	switch (addr)
	{
	default:
		break;
	}
}
void GPUreadreg32(u32 addr)
{
	DEBUG("GPU read %08x", addr);
	if (addr > 0x420000)
	{
		DEBUG("read out of range write");
		return;
	}
	return *(uint32_t*)(&IObuffer[addr]);
}
void GPUTriggerCmdReqQueue() //todo
{

}
void GPURegisterInterruptRelayQueue(u32 Flags, u32 Kevent, u32*threadID, u32*outMemHandle)
{
		
}