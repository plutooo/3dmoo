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
    VRAMbuff = malloc(0x800000);//malloc(0x600000);
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
/*
0 = PSC0 private?
1 = PSC1 private?
2 = PDC0 public?
3 = PDC1 public?
4 = PPF  private?
5 = P3D  private?
6 = DMA  private?

PDC0 called every line?
PDC1 called every VBlank?

*/

u32 convertvirtualtopys(u32 addr) //topo
{
    if (addr >= 0x14000000 && addr < 0x1C000000)return addr + 0xC000000; //FCRAM
    if (addr >= 0x1F000000 && addr < 0x1F600000)return addr - 0x7000000; //VRAM
    DEBUG("can't convert vitual to py %08x",addr);
    return 0;
}
void sendGPUinterall(u32 ID)
{
    int i;
    handleinfo* h = handle_Get(trigevent);
    if (h == NULL) {
        return;
    }
    h->locked = false; //unlock we are fast
    for (i = 0; i < 4; i++) {
        u8 next = *(u8*)(GSPsharedbuff + i * 0x40);        //0x33 next is 00
        next  += *(u8*)(GSPsharedbuff + i * 0x40 + 1) = *(u8*)(GSPsharedbuff + i * 0x40 + 1) + 1;
        *(u8*)(GSPsharedbuff + i * 0x40 + 2) = 0x0; //no error
        next = next % 0x34;
        *(u8*)(GSPsharedbuff + i * 0x40 + 0xC + next) = ID;
    }

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
u32 getsizeofwight(u16 val) //this is the size of pixel
{
    switch (val) {
    case 0x0201:
        return 3;
    default:
        DEBUG("unknown len %04x",val);
        return 3;
    }
}
u32 getsizeofwight32(u32 val)
{
    switch (val) {
    case 0x00800080: //no idea why
        return 0x8000;
    default:
        return (val & 0xFFFF) * ((val>>16) & 0xFFFF) * 3;
    }
}

u32 getsizeofframebuffer(u32 val)
{
    switch (val) {
    case 0x0201:
        return 3;
    default:
        DEBUG("unknown len %08x", val);
        return 3;
    }
}


void updateFramebuffer()
{
    //we use the last in buffer with flag set
    int i;
    for (i = 0; i < 4; i++) {
        u8 *baseaddrtop = (u8*)(GSPsharedbuff + 0x200 + i * 0x80); //top
        if (*(u8*)(baseaddrtop + 1)) {
            *(u8*)(baseaddrtop + 1) = 0;
            if (*(u8*)(baseaddrtop))
                baseaddrtop += 0x20; //get the other
            else
                baseaddrtop += 0x4;
            GPUwritereg32(frameselectoben, *(u32*)(baseaddrtop));
            if ((*(u32*)(baseaddrtop)& 0x1) == 0)
                GPUwritereg32(RGBuponeleft, convertvirtualtopys(*(u32*)(baseaddrtop + 4)));
            else
                GPUwritereg32(RGBuptwoleft, convertvirtualtopys(*(u32*)(baseaddrtop + 4)));
            //the rest is todo
        }
        u8 *baseaddrbot = (u8*)(GSPsharedbuff + 0x240 + i * 0x80); //bot
        if (*(u8*)(baseaddrbot + 1)) {
            *(u8*)(baseaddrbot + 1) = 0;
            if (*(u8*)(baseaddrbot))
                baseaddrbot += 0x20; //get the other
            else
                baseaddrbot += 0x4;
            //GPUwritereg32(frameselectoben, *(u32*)(baseaddrbot)); //todo
            if ((*(u32*)(baseaddrbot) &0x1) == 0)
                GPUwritereg32(RGBdownoneleft, convertvirtualtopys(*(u32*)(baseaddrbot + 4)));
            else
                GPUwritereg32(RGBdowntwoleft, convertvirtualtopys(*(u32*)(baseaddrbot + 4)));
            //the rest is todo


        }
    }
    return;
}
u32 GPUnum = 0;

void GPUTriggerCmdReqQueue() //todo
{
    for (int i = 0; i < 0x4; i++) { //for all threads
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
            u32 addr;
            u32 flags;
            switch (CMDID & 0xFF) {
            case 0:
                src = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                dest = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                size = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                DEBUG("GX RequestDma 0x%08X 0x%08X 0x%08X\r\n", addr, size, flags);
                if (dest - 0x1f000000 > 0x600000 || dest + size - 0x1f000000 > 0x600000) {
                    DEBUG("dma copy into non VRAM not suported\r\n");
                    continue;
                }


                //for (u32 k = 0; k < size; k++)
                //    VRAMbuff[k + dest - 0x1F000000] = mem_Read8((src + k)); //todo speed up

                mem_Read(&VRAMbuff[dest - 0x1F000000], src, size);

                break;
            case 1:
                addr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                size = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                flags = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                DEBUG("GX SetCommandList Last 0x%08X 0x%08X 0x%08X --todo--\r\n",addr,size,flags);

                char name[0x100];
                sprintf(name, "Cmdlist%08x.dat", GPUnum);
                GPUnum++;
                FILE* out = fopen(name,"wb");

                u8* buffer = malloc(size);
                mem_Read(buffer, addr, size);

                //u8* buffer = get_pymembuffer(addr);

                fwrite(buffer, size, 1, out);
                fclose(out);

                sendGPUinterall(5);//P3D
                break;
            case 2: {
                u32 addr1, val1, addrend1, addr2, val2, addrend2,width;
                addr1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                val1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                addrend1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                addr2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x10);
                val2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x14);
                addrend2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x18);
                width = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x1C);
                DEBUG("GX SetMemoryFill 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", addr1, val1, addrend1, addr2, val2, addrend2, width);
                if (addr1 - 0x1f000000 > 0x600000 || addrend1 - 0x1f000000 > 0x600000) {
                    DEBUG("SetMemoryFill into non VRAM not suported\r\n");
                } else {
                    u32 size = getsizeofwight(width&0xFFFF);
                    u32 k;
                    for (k = 0; k*size + addr1 < addrend1; k++) {
                        u32 m;
                        for (m = 0; m<size; m++)
                            VRAMbuff[m + k*size + addr1 - 0x1F000000] = (u8)(val1 >> (m * 8));
                    }
                }
                if (addr2 - 0x1f000000 > 0x600000 || addrend2 - 0x1f000000 > 0x600000) {
                    DEBUG("SetMemoryFill into non VRAM not suported\r\n");
                } else {
                    u32 size = getsizeofwight((width >> 16) & 0xFFFF);
                    u32 k;
                    for (k = 0; k*size + addr2 < addrend2; k++) {
                        u32 m;
                        for (m = 0; m<size; m++)
                            VRAMbuff[m + k*size + addr2 - 0x1F000000] = (u8)(val1 >> (m * 8));
                    }
                }
                break;
            }
            case 3: {
                u32 inpaddr, outputaddr, inputdim, outputdim, flags;
                inpaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                outputaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                inputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                outputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x10);
                flags = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x14);
                DEBUG("GX SetDisplayTransfer 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X --todo--\r\n", inpaddr, outputaddr, inputdim, outputdim, flags);

                if (inputdim != outputdim) {
                    DEBUG("error converting from %08x to %08x", inputdim, outputdim);
                }
                u32 sizeoutp = getsizeofwight32(inputdim);

                memcpy(get_pymembuffer(convertvirtualtopys(outputaddr)),get_pymembuffer(convertvirtualtopys(inpaddr)) , sizeoutp);
                updateFramebuffer();

                sendGPUinterall(0);
                sendGPUinterall(1);
                sendGPUinterall(4);
                sendGPUinterall(5);
                sendGPUinterall(6);

                mem_Dbugdump();

                break;
            }
            case 4: {
                u32 inpaddr, outputaddr /*,size*/, inputdim, outputdim, flags;
                inpaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                outputaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                size = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                inputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x10);
                outputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x14);
                flags = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x18);
                DEBUG("GX SetTextureCopy 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X --todo--\r\n", inpaddr, outputaddr,size, inputdim, outputdim, flags);

                updateFramebuffer();
                break;
            }
            case 5: {
                u32 addr1, size1, addr2, size2, addr3, size3;
                addr1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                size1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                addr2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                size2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x10);
                addr3 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x14);
                size3 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x18);
                DEBUG("GX SetCommandList First 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X --todo--\r\n",addr1, size1, addr2, size2, addr3, size3);
                break;
            }
            default:
                DEBUG("GX cmd 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", *(u32*)(baseaddr + (j + 1) * 0x20), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x4), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x8), *(u32*)((baseaddr + (j + 1) * 0x20) + 0xC), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x10), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x14), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x18), *(u32*)((baseaddr + (j + 1) * 0x20)) + 0x1C);
                break;
            }
        }
    }
}

u32 GPURegisterInterruptRelayQueue(u32 flags, u32 Kevent, u32*threadID, u32*outMemHandle)
{
    *threadID = numReqQueue++;
    *outMemHandle = handle_New(HANDLE_TYPE_SHAREDMEM, MEM_TYPE_GSP_0);
    trigevent = Kevent;
    handleinfo* h = handle_Get(Kevent);
    if (h == NULL) {
        DEBUG("failed to get Event\n");
        PAUSE();
        return -1;// -1;
    }
    h->locked = false; //unlock we are fast

    *(u32*)(GSPsharedbuff + *threadID * 0x40) = 0x0; //dump from save GSP v0 flags 0
    *(u32*)(GSPsharedbuff + *threadID * 0x44) = 0x0; //dump from save GSP v0 flags 0
    *(u32*)(GSPsharedbuff + *threadID * 0x48) = 0x0; //dump from save GSP v0 flags 0
    return 0x2A07; //dump from save GSP v0 flags 0
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
