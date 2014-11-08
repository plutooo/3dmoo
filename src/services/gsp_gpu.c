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
#include "mem.h"
#include "gpu.h"


u32 numReqQueue = 1;

u32 trigevent = 0;


u32 GPUnum = 0;

void GPUTriggerCmdReqQueue()
{
    for (int i = 0; i < 0x4; i++) { //for all threads
        u8 *baseaddr = (u8*)(GSPsharedbuff + 0x800 + i * 0x200);
        u32 header = *(u32*)baseaddr;
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
                DEBUG("GX RequestDma 0x%08X 0x%08X 0x%08X\r\n", src, dest, size);
                if (dest - 0x1f000000 > 0x600000 || dest + size - 0x1f000000 > 0x600000) {
                    DEBUG("dma copy into non VRAM not suported\r\n");
                    continue;
                }


                //for (u32 k = 0; k < size; k++)
                //    VRAMbuff[k + dest - 0x1F000000] = mem_Read8((src + k)); //todo speed up

                mem_Read(&VRAMbuff[dest - 0x1F000000], src, size);
                gpu_SendInterruptToAll(6);
                break;
            case 1:
                addr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                size = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                flags = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                DEBUG("GX SetCommandList Last 0x%08X 0x%08X 0x%08X\r\n", addr, size, flags);

                char name[0x100];
                sprintf(name, "Cmdlist%08x.dat", GPUnum);
                GPUnum++;
                FILE* out = fopen(name, "wb");

                u8* buffer = malloc(size);
                mem_Read(buffer, addr, size);

                runGPU_Commands(buffer, size);

                //u8* buffer = get_pymembuffer(addr);

                fwrite(buffer, size, 1, out);
                fclose(out);


                break;
            case 2: {
                        u32 addr1, val1, addrend1, addr2, val2, addrend2, width;
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
                        }
                        else {
                            u32 size = getsizeofwight(width & 0xFFFF);
                            u32 k;
                            for (k = 0; k*size + addr1 < addrend1; k++) {
                                s32 m;
                                for (m = size - 1; m >= 0; m--)
                                    VRAMbuff[m + k*size + addr1 - 0x1F000000] = (u8)(val1 >> (m * 8));
                            }
                        }
                        if (addr2 - 0x1f000000 > 0x600000 || addrend2 - 0x1f000000 > 0x600000) {
                            DEBUG("SetMemoryFill into non VRAM not suported\r\n");
                        }
                        else {
                            u32 size = getsizeofwight((width >> 16) & 0xFFFF);
                            u32 k;
                            for (k = 0; k*size + addr2 < addrend2; k++) {
                                s32 m;
                                for (m = size - 1; m >= 0; m--)
                                    VRAMbuff[m + k*size + addr2 - 0x1F000000] = (u8)(val2 >> (m * 8));
                            }
                        }
                        gpu_SendInterruptToAll(0);
                        break;
            }
            case 3: 
            
theother:
            {


                        gpu_SendInterruptToAll(1); //this should be at the start
                        gpu_SendInterruptToAll(4); //this is wrong


                        u32 inpaddr, outputaddr, inputdim, outputdim, flags;
                        inpaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                        outputaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                        inputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                        outputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x10);
                        flags = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x14);
                        DEBUG("GX SetDisplayTransfer 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", inpaddr, outputaddr, inputdim, outputdim, flags);

                        if (inputdim != outputdim) {
                            DEBUG("error converting from %08x to %08x\n", inputdim, outputdim);
                            break;
                        }
                        
                        u8 * inaddr = get_pymembuffer(convertvirtualtopys(inpaddr));
                        u8 * outaddr = get_pymembuffer(convertvirtualtopys(outputaddr));

                        u32 rely = (inputdim & 0xFFFF);
                        u32 relx = (inputdim >> 0x10) & 0xFFFF;

                        for (u32 y = 0; y < rely; ++y) {
                            for (u32 x = 0; x < relx; ++x) {
                                u8 a = 0xFF;
                                u8 r = 0xFF;
                                u8 g = 0xFF;
                                u8 b = 0xFF;
                                switch (flags & 0x700)//input format
                                {

                                case 0: //RGBA8

                                    r = *inaddr;
                                    inaddr++;
                                    g = *inaddr;
                                    inaddr++;
                                    b = *inaddr;
                                    inaddr++;
                                    a = *inaddr;
                                    inaddr++;
                                    break;
                                case 0x100: //RGB8
                                    r = *inaddr;
                                    inaddr++;
                                    g = *inaddr;
                                    inaddr++;
                                    b = *inaddr;
                                    inaddr++;
                                    a = 0xFF;
                                    break;
                                case 0x200: //RGB565
                                {
                                            u8 reg1 = *inaddr;
                                            inaddr++;
                                            u8 reg2 = *inaddr;
                                            inaddr++;
                                            r = (reg1&0x1F)<<3;
                                            g = (((reg1 & 0xE0) >> 5) + (reg2 & 0x7) << 3) << 2;
                                            b = ((reg2 & 0xF8) >> 3) << 3;
                                            a = 0xFF;
                                }
                                    break;
                                case 0x300: //RGB5A1
                                {
                                    u8 reg1 = *inaddr;
                                    inaddr++;
                                    u8 reg2 = *inaddr;
                                    inaddr++;
                                    r = (reg1 & 0x1F) << 3;
                                    g = (((reg1 & 0xE0) >> 5) + (reg2 & 0x3) << 3) << 3;
                                    b = ((reg2 & 0x7C) >> 3) << 3;
                                    if (reg2)a = 0xFF;
                                }
                                    break;
                                case 0x400: //RGBA4
                                {
                                    u8 reg1 = *inaddr;
                                    inaddr++;
                                    u8 reg2 = *inaddr;
                                    inaddr++;
                                    r = (reg1 & 0xF) << 4;
                                    g = reg1 & 0xF0;
                                    b = (reg2 & 0xF) << 4;
                                    a = reg2 & 0xF0;
                                    break;
                                }
                                default:
                                    DEBUG("error unknow input format\n");
                                    break;
                                }
                                //write it back

                                switch (flags & 0x7000)//input format
                                {

                                case 0: //RGBA8

                                    *outaddr = r;
                                    outaddr++;
                                    *outaddr = g;
                                    outaddr++;
                                    *outaddr = b;
                                    outaddr++;
                                    *outaddr = a;
                                    outaddr++;
                                    break;
                                case 0x1000: //RGB8
                                    if (a)
                                    {
                                        *outaddr = r;
                                        outaddr++;
                                        *outaddr = g;
                                        outaddr++;
                                        *outaddr = b;
                                        outaddr++;
                                    }
                                    else
                                    {
                                        *outaddr = 0;
                                        outaddr++;
                                        *outaddr = 0;
                                        outaddr++;
                                        *outaddr = 0;
                                        outaddr++;
                                    }
                                    break;
                                case 0x2000: //RGB565
                                {
                                    u16 result = (r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11);
                                    *outaddr = result & 0xFF;
                                    outaddr++;
                                    *outaddr = (result >> 8) & 0xFF;
                                    outaddr++;
                                }
                                    break;
                                case 0x3000: //RGB5A1
                                    *outaddr = (r >> 3) | (((g >> 3) << 5) & 0xE0);
                                    outaddr++;
                                    *outaddr = (u8)((b >> 3) << 3) | ((g >> 3) & 0x7);
                                    if (a) *outaddr |= 0x80;
                                    outaddr++;
                                    break;
                                case 0x4000: //RGBA4
                                    *outaddr = (r >> 4) | (g & 0xF0);
                                    outaddr++;
                                    *outaddr = (b >> 4) | (a & 0xF0);
                                    outaddr++;
                                    break;
                                default:
                                    DEBUG("error unknow output format\n");
                                    break;
                                }

                           } 
                       }






                        //memcpy(get_pymembuffer(convertvirtualtopys(outputaddr)), get_pymembuffer(convertvirtualtopys(inpaddr)), sizeoutp);
                        updateFramebuffer();


                        //mem_Dbugdump();

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
                        DEBUG("GX SetTextureCopy 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X --todo--\r\n", inpaddr, outputaddr, size, inputdim, outputdim, flags);

                        updateFramebuffer();
                        //goto theother; //untill I know what is the differnece
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
                        DEBUG("GX SetCommandList First 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", addr1, size1, addr2, size2, addr3, size3);
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

u32 gsp_gpu_SyncRequest()
{
    u32 cid = mem_Read32(arm11_ServiceBufferAddress() + 0x80);



    u32 outaddr;
    u32 lange;
    u32 addr;
    u32 ret;
    switch (cid) {
    case 0x10082: { //GSPGPU_WriteHWRegs(u32 regAddr, u32* data, u8 size)
        outaddr = mem_Read32(arm11_ServiceBufferAddress() + 0x90);
        lange = mem_Read32(arm11_ServiceBufferAddress() + 0x88);
        addr = mem_Read32(arm11_ServiceBufferAddress() + 0x84);
        ret = 0;
        DEBUG("GSPGPU_WriteHWRegs %08x %08x %08x\n", addr, outaddr, lange);
        if ((lange & 0x3) != 0)
        {
            DEBUG("misaligned address\n");
            ret = 0xe0e02a01;
        }
        if (addr > 0x420000)
        {
            DEBUG("address out of range\n");
            ret = 0xe0e02a01;
        }
        if (lange > 0x80)
        {
            DEBUG("to long\n");
            ret = 0xe0e02bec;
        }
        if (lange & 0x3)
        {
            DEBUG("length misaligned\n");
            ret = 0xe0e02bf2;
        }
        for (u32 i = 0; i < lange; i += 4) gpu_WriteReg32((u32)(addr + i), mem_Read32((u32)(outaddr + i)));
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, ret); //no error
        return 0;
    }
    case 0x40080: { //GSPGPU_ReadHWRegs(u32 regAddr, u32* data, u8 size)
        outaddr = mem_Read32(arm11_ServiceBufferAddress() + 0x184);
        lange = mem_Read32(arm11_ServiceBufferAddress() + 0x88);
        addr = mem_Read32(arm11_ServiceBufferAddress() + 0x84);
        DEBUG("GSPGPU_ReadHWRegs %08x %08x %08x\n", addr, outaddr, lange);
        if ((lange & 0x3) != 0) DEBUG("Misaligned address\n");
        for (u32 i = 0; i < lange; i += 4) mem_Write32((u32)(outaddr + i), gpu_ReadReg32((u32)(addr + i)));
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    case 0x50200:
        updateFramebufferaddr(arm11_ServiceBufferAddress() + 0x84, mem_Read8(arm11_ServiceBufferAddress() + 0x84) & 0x1);
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    case 0xB0040: { //SetLcdForceBlack
        DEBUG("SetLcdForceBlack %02x --todo--\n", mem_Read8(arm11_ServiceBufferAddress() + 0x84));
        unsigned char* buffer = get_pymembuffer(0x18000000);
        //memset(buffer, 0, 0x46500 * 6); //no this is todo
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    case 0xC0000: { //TriggerCmdReqQueue
        DEBUG("TriggerCmdReqQueue\n");
        GPUTriggerCmdReqQueue();
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    case 0x130042: { //RegisterInterruptRelayQueue
        DEBUG("RegisterInterruptRelayQueue %08x %08x\n", mem_Read32(arm11_ServiceBufferAddress() + 0x84), mem_Read32(arm11_ServiceBufferAddress() + 0x8C));
        u32 threadID = 0;
        u32 outMemHandle = 0;
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, GPURegisterInterruptRelayQueue(mem_Read32(arm11_ServiceBufferAddress() + 0x84), mem_Read32(arm11_ServiceBufferAddress() + 0x8C), &threadID, &outMemHandle)); //no error
        mem_Write32(arm11_ServiceBufferAddress() + 0x88, threadID);
        mem_Write32(arm11_ServiceBufferAddress() + 0x90, outMemHandle);
        return 0;
    }
    case 0x160042: { //AcquireRight
        DEBUG("AcquireRight %08x %08x --todo--\n", mem_Read32(arm11_ServiceBufferAddress() + 0x84), mem_Read32(arm11_ServiceBufferAddress() + 0x8C));
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    default:
        DEBUG("STUBBED GPUGSP %x %x %x %x\n", mem_Read32(arm11_ServiceBufferAddress() + 0x80), mem_Read32(arm11_ServiceBufferAddress() + 0x84), mem_Read32(arm11_ServiceBufferAddress() + 0x88), mem_Read32(arm11_ServiceBufferAddress() + 0x8C));
    }
    PAUSE();

    return 0;
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
void gpu_SendInterruptToAll(u32 ID)
{
    int i;
    handleinfo* h = handle_Get(trigevent);
    if (h == NULL) {
        return;
    }
    h->locked = false; //unlock we are fast
    for (i = 0; i < 4; i++) {
        u8 next = *(u8*)(GSPsharedbuff + i * 0x40);        //0x33 next is 00
        u8 inuse = *(u8*)(GSPsharedbuff + i * 0x40 + 1);
        next += inuse;
        if (inuse > 0x20 && ((ID == 2) || (ID == 3)))
            continue; //todo

        *(u8*)(GSPsharedbuff + i * 0x40 + 1) = inuse + 1;
        *(u8*)(GSPsharedbuff + i * 0x40 + 2) = 0x0; //no error
        next = next % 0x34;
        *(u8*)(GSPsharedbuff + i * 0x40 + 0xC + next) = ID;
    }

}
