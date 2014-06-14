/*
* Copyright (C) 2014 - plutoo
* Copyright (C) 2014 - ichfly
* Copyright (C) 2014 - Normmatt
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
#include "mem.h"
#include "gpu.h"

/*u8* IObuffer;
u8* LINEmembuffer;
u8* VRAMbuff;
u8* GSPsharedbuff;*/

extern int noscreen;

u32 numReqQueue = 1;

u32 trigevent = 0;
void initGPU()
{
    IObuffer = malloc(0x420000);
    LINEmembuffer = malloc(0x8000000);
    VRAMbuff = malloc(0x600000);
    GSPsharedbuff = malloc(GSPsharebuffsize);

    memset(IObuffer, 0, 0x420000);
    memset(LINEmembuffer, 0, 0x8000000);
    memset(VRAMbuff, 0, 0x600000);
    memset(GSPsharedbuff, 0, GSPsharebuffsize);

    GPUwritereg32(frameselectoben, 0);
    GPUwritereg32(RGBuponeleft, 0x18000000);
    GPUwritereg32(RGBuptwoleft, 0x18000000 + 0x46500 * 1);
    GPUwritereg32(RGBdownoneleft, 0x18000000 + 0x46500 * 4);
    GPUwritereg32(RGBdowntwoleft, 0x18000000 + 0x46500 * 5);
}

void GPUwritereg32(u32 addr, u32 data)
{
    DEBUG("GPU write %08x to %08x\n",data,addr);
    if (addr >= 0x420000) {
        DEBUG("write out of range write\r\n");
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
    //DEBUG("GPU read %08x\n", addr);
    if (addr >= 0x420000) {
        DEBUG("read out of range write\r\n");
        return 0;
    }
    return *(uint32_t*)(&IObuffer[addr]);
}
void GPUTriggerCmdReqQueue() //todo
{
    for (int i = 0; i < 0xFF; i++) { //for all threads
        u8 *baseaddr = (u8*)(GSPsharedbuff + 0x800 + i * 0x200);
        u32 header = *(u32*)baseaddr;
        //Console.WriteLine(Convert.ToString(header,0x10));
        u32 toprocess = (header >> 8) & 0xFF;
        for (u32 j = 0; j < toprocess; j++) {
            *(u32*)baseaddr = 0;
            u32 CMDID = *(u32*)(baseaddr + (j + 1) * 0x20);
            u32 src;
            u32 dest;
            u32 size;
            switch (CMDID & 0xFF) {
            case 0:
                src = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                dest = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                size = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                if (dest - 0x1f000000 > 0x600000)DEBUG("dma copy into non VRAM not suported\r\n");


                //for (u32 k = 0; k < size; k++)
                //    VRAMbuff[k + dest - 0x1F000000] = mem_Read8((src + k)); //todo speed up

                mem_Read(&VRAMbuff[dest - 0x1F000000], src, size);

                break;
            default:
                DEBUG("GX cmd 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", *(u32*)(baseaddr + (j + 1) * 0x20), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x4), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x8), *(u32*)((baseaddr + (j + 1) * 0x20) + 0xC), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x10), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x14), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x18), *(u32*)((baseaddr + (j + 1) * 0x20)) + 0x1C);
                break;
            }
        }
    }
}

void GPURegisterInterruptRelayQueue(u32 flags, u32 Kevent, u32*threadID, u32*outMemHandle)
{
    *threadID = ++numReqQueue;
    *outMemHandle = handle_New(HANDLE_TYPE_SHAREDMEM, MEM_TYPE_GSP_0);
    trigevent = Kevent;
    handleinfo* h = handle_Get(Kevent);
    if (h == NULL) {
        DEBUG("failed to get Event\n");
        PAUSE();
        return;// -1;
    }
    h->locked = false; //unlock we are fast
}
u8* get_pymembuffer(u32 addr)
{
    if (addr >= 0x18000000 && addr < 0x18600000)return VRAMbuff + (addr - 0x18000000);
    if (addr >= 0x20000000 && addr < 0x28000000)return LINEmembuffer + (addr - 0x20000000);
    return NULL;
}
u32 get_py_memrestsize(u32 addr)
{
    if (addr >= 0x18000000 && addr < 0x18600000)return addr - 0x18000000;
    if (addr >= 0x20000000 && addr < 0x28000000)return addr - 0x20000000;
    return 0;
}
