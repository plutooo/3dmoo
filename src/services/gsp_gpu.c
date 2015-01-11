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

#include "screen.h"
#include "color.h"
#include "service_macros.h"


u32 numReqQueue = 1;
u32 trigevent = 0;

//#define DUMP_CMDLIST


void gsp_ExecuteCommandFromSharedMem()
{
    int i;

    // For all threads
    for (i = 0; i < 0x4; i++) {
        u8* baseaddr = (u8*)(GSPsharedbuff + 0x800 + i * 0x200);
        u32 header = *(u32*)baseaddr;
        u32 toprocess = (header >> 8) & 0xFF;

        //mem_Dbugdump();

        *(u32*)baseaddr = 0;
        for (u32 j = 0; j < toprocess; j++) {
            u32 cmd_id = *(u32*)(baseaddr + (j + 1) * 0x20);

            switch (cmd_id & 0xFF) {
            case GSP_ID_REQUEST_DMA: { /* GX::RequestDma */
                u32 src = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                u32 dest = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                u32 size = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);

                GPUDEBUG("GX RequestDma 0x%08x 0x%08x 0x%08x\n", src, dest, size);

                if (dest - 0x1f000000 > 0x600000 || dest + size - 0x1f000000 > 0x600000) {
                    GPUDEBUG("dma copy into non VRAM not suported\n");
                    continue;
                }

                if((src - 0x1f000000 > 0x600000 || src + size - 0x1f000000 > 0x600000))
                {
                    mem_Read(&VRAMbuff[dest - 0x1F000000], src, size);
                }
                else
                {
                    //Can safely assume this is a copy from VRAM to VRAM
                    memcpy(&VRAMbuff[dest - 0x1F000000], &VRAMbuff[src - 0x1F000000], size);
                }

                gpu_SendInterruptToAll(6);
                break;
            }

            case GSP_ID_SET_CMDLIST: { /* GX::SetCmdList Last */
                u32 addr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                u32 size = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                u32 flags = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);

                GPUDEBUG("GX SetCommandList Last 0x%08x 0x%08x 0x%08x\n", addr, size, flags);

#ifdef DUMP_CMDLIST
                char name[0x100];
                static u32 cmdlist_ctr;

                sprintf(name, "Cmdlist%08x.dat", cmdlist_ctr++);
                FILE* out = fopen(name, "wb");
#endif

                u8* buffer = malloc(size);
                mem_Read(buffer, addr, size);
                gpu_ExecuteCommands(buffer, size);

#ifdef DUMP_CMDLIST
                fwrite(buffer, size, 1, out);
                fclose(out);
#endif
                free(buffer);
                break;
            }

            case GSP_ID_SET_MEMFILL: {
                u32 addr1, val1, addrend1, addr2, val2, addrend2, width;
                addr1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                val1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                addrend1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                addr2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x10);
                val2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x14);
                addrend2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x18);
                width = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x1C);

                GPUDEBUG("GX SetMemoryFill 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", addr1, val1, addrend1, addr2, val2, addrend2, width);
                if (addr1 - 0x1f000000 > 0x600000 || addrend1 - 0x1f000000 > 0x600000) {
                    GPUDEBUG("SetMemoryFill into non VRAM not suported\r\n");
                } else {
                    u32 size = getsizeofwight(width & 0xFFFF);
                    u32 k;
                    for(k = addr1; k < addrend1; k+=size) {
                        s32 m;
                        for(m = size - 1; m >= 0; m--)
                        {
                            VRAMbuff[m + (k - 0x1F000000)] = (u8)(val1 >> (m * 8));
                        }
                    }
                }
                if (addr2 - 0x1f000000 > 0x600000 || addrend2 - 0x1f000000 > 0x600000) {
                    if (addr2 && addrend2)
                        GPUDEBUG("SetMemoryFill into non VRAM not suported\r\n");
                } else {
                    u32 size = getsizeofwight((width >> 16) & 0xFFFF);
                    u32 k;
                    for(k = addr2; k < addrend2; k += size) {
                        s32 m;
                        for (m = size - 1; m >= 0; m--)
                            VRAMbuff[m + (k - 0x1F000000)] = (u8)(val2 >> (m * 8));
                    }
                }
                gpu_SendInterruptToAll(0);
                break;
            }
            case GSP_ID_SET_DISPLAY_TRANSFER:
            {
                    gpu_SendInterruptToAll(4); 


                    u32 inpaddr, outputaddr, inputdim, outputdim, flags, unk;
                    inpaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                    outputaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                    inputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                    outputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x10);
                    flags = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x14);
                    unk = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x18);
                    GPUDEBUG("GX SetDisplayTransfer 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", inpaddr, outputaddr, inputdim, outputdim, flags, unk);

                    u8 * inaddr = get_pymembuffer(convertvirtualtopys(inpaddr));
                    u8 * outaddr = get_pymembuffer(convertvirtualtopys(outputaddr));

                    u32 rely = (inputdim & 0xFFFF);
                    u32 relx = ((inputdim >> 0x10) & 0xFFFF);

                    u32 outy = (outputdim & 0xFFFF);
                    u32 outx = ((outputdim >> 0x10) & 0xFFFF);

                    if(inputdim != outputdim) {
                        /*FILE *test = fopen("Conversion.bin", "wb");
                        u32 len = 0;
                        switch(flags & 0x700)
                        {
                            case 0: //RGBA8
                                len = rely * relx * 4;
                                break;
                            case 0x100: //RGB8
                                len = rely * relx * 3;
                                break;
                            case 0x200: //RGB565
                            case 0x300: //RGB5A1
                            case 0x400: //RGBA4
                                len = rely * relx * 2;
                                break;
                        }
                        fwrite(inaddr, 1, len, test);
                        fclose(test);*/

                        //Lets just skip the first 80*240*bytesperpixel or so as its blank then continue as usual
                        switch(flags & 0x700)
                        {
                            case 0: //RGBA8
                                inaddr += outy * abs(relx - outx) * 4;
                                break;
                            case 0x100: //RGB8
                                inaddr += outy * abs(relx - outx) * 3;
                                break;
                            case 0x200: //RGB565
                            case 0x300: //RGB5A1
                            case 0x400: //RGBA4
                                inaddr += outy * abs(relx - outx) * 2;
                                break;
                        }
                        //GPUDEBUG("error converting from %08x to %08x\n", inputdim, outputdim);
                        //break;
                    }

                    if((flags & 0x700) == ((flags & 0x7000) >> 4))
                    {
                        u32 len = 0;
                        switch(flags & 0x700)
                        {
                            case 0: //RGBA8
                                len = rely * relx * 4;
                                break;
                            case 0x100: //RGB8
                                len = rely * relx * 3;
                                break;
                            case 0x200: //RGB565
                            case 0x300: //RGB5A1
                            case 0x400: //RGBA4
                                len = rely * relx * 2;
                                break;
                        }
                        GPUDEBUG("copying %d (width %d/%d, height %d/%d)\n", len, relx, outx, rely, outy);
                        memcpy(outaddr, inaddr, len);
                    }
                    else
                    {
                        GPUDEBUG("converting %d to %d (width %d/%d, height %d/%d)\n", (flags & 0x700) >> 8, (flags & 0x7000) >> 12, relx, outx, rely, outy);
                        Color color;
                        
                        for(u32 y = 0; y < outy; ++y)
                        {
                            for(u32 x = 0; x < outx; ++x) 
                            {
                                switch(flags & 0x700) { //input format

                                    case 0: //RGBA8
                                        color_decode(inaddr, RGBA8, &color);
                                        inaddr += 4;
                                        break;
                                    case 0x100: //RGB8
                                        color_decode(inaddr, RGB8, &color);
                                        inaddr += 3;
                                        break;
                                    case 0x200: //RGB565
                                        color_decode(inaddr, RGB565, &color);
                                        inaddr += 2;
                                        break;
                                    case 0x300: //RGB5A1
                                        color_decode(inaddr, RGBA5551, &color);
                                        inaddr += 2;
                                        break;
                                    case 0x400: //RGBA4
                                        color_decode(inaddr, RGBA4, &color);
                                        inaddr += 2;
                                        break;
                                    default:
                                        GPUDEBUG("error unknown input format %04X\n", flags & 0x700);
                                        break;
                                }
                                //write it back

                                switch(flags & 0x7000) { //output format

                                    case 0: //RGBA8
                                        color_encode(&color, RGBA8, outaddr);
                                        outaddr += 4;
                                        break;
                                    case 0x1000: //RGB8
                                        color_encode(&color, RGB8, outaddr);
                                        outaddr += 3;
                                        break;
                                    case 0x2000: //RGB565
                                        color_encode(&color, RGB565, outaddr);
                                        outaddr += 2;
                                        break;
                                    case 0x3000: //RGB5A1
                                        color_encode(&color, RGBA5551, outaddr);
                                        outaddr += 2;
                                        break;
                                    case 0x4000: //RGBA4
                                        color_encode(&color, RGBA4, outaddr);
                                        outaddr += 2;
                                        break;
                                    default:
                                        GPUDEBUG("error unknown output format %04X\n", flags & 0x7000);
                                        break;
                                }

                            }
                        }
                    }
                    updateFramebuffer();
                    break;
                }
            case GSP_ID_SET_TEXTURE_COPY: {
                gpu_SendInterruptToAll(1);
                u32 inpaddr, outputaddr /*,size*/, inputdim, outputdim, flags;
                inpaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                outputaddr = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                u32 size = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                inputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x10);
                outputdim = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x14);
                flags = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x18);
                GPUDEBUG("GX SetTextureCopy 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X --todo--\r\n", inpaddr, outputaddr, size, inputdim, outputdim, flags);

                updateFramebuffer();
                //goto theother; //untill I know what is the differnece
                break;
            }
            case GSP_ID_FLUSH_CMDLIST: {
                u32 addr1, size1, addr2, size2, addr3, size3;
                addr1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x4);
                size1 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x8);
                addr2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0xC);
                size2 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x10);
                addr3 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x14);
                size3 = *(u32*)(baseaddr + (j + 1) * 0x20 + 0x18);
                GPUDEBUG("GX SetCommandList First 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", addr1, size1, addr2, size2, addr3, size3);
                break;
            }
            default:
                GPUDEBUG("GX cmd 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\r\n", *(u32*)(baseaddr + (j + 1) * 0x20), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x4), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x8), *(u32*)((baseaddr + (j + 1) * 0x20) + 0xC), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x10), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x14), *(u32*)((baseaddr + (j + 1) * 0x20) + 0x18), *(u32*)((baseaddr + (j + 1) * 0x20)) + 0x1C);
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
        GPUDEBUG("failed to get Event\n");
        PAUSE();
        return -1;// -1;
    }
    h->locked = false; //unlock we are fast

    *(u32*)(GSPsharedbuff + *threadID * 0x40) = 0x0; //dump from save GSP v0 flags 0
    *(u32*)(GSPsharedbuff + *threadID * 0x44) = 0x0; //dump from save GSP v0 flags 0
    *(u32*)(GSPsharedbuff + *threadID * 0x48) = 0x0; //dump from save GSP v0 flags 0
    return 0x2A07; //dump from save GSP v0 flags 0
}


SERVICE_START(gsp_gpu);

SERVICE_CMD(0x10082) // WriteHWRegs
{
    u32 inaddr  = CMD(4);
    u32 length  = CMD(2);
    u32 addr    = CMD(1);
    u32 ret     = 0;

    GPUDEBUG("GSPGPU_WriteHWRegs\n");

    if ((addr & 0x3) != 0) {
        GPUDEBUG("Misaligned address\n");
        ret = 0xe0e02a01;
    }
    if (addr > 0x420000) {
        GPUDEBUG("Address out of range\n");
        ret = 0xe0e02a01;
    }
    if (length > 0x80) {
        GPUDEBUG("Too long\n");
        ret = 0xe0e02bec;
    }
    if (length & 0x3) {
        GPUDEBUG("Length misaligned\n");
        ret = 0xe0e02bf2;
    }

    if(ret == 0) {
        u32 i;

        for (i = 0; i < length; i += 4) {
            u32 val = mem_Read32(inaddr + i);
            u32 addr_out = addr + i;

            GPUDEBUG("Writing %08x to register %08x..\n", val, addr_out);
            gpu_WriteReg32(addr_out, val);
        }
    }

    RESP(1, ret);
    return 0;
}

SERVICE_CMD(0x20084) // WriteHWRegsWithMask
{
    u32 inmask  = CMD(6);
    u32 inaddr  = CMD(4);
    u32 length  = CMD(2);
    u32 addr    = CMD(1);
    u32 ret     = 0;

    GPUDEBUG("GSPGPU_WriteHWRegsWithMask\n");

    if ((addr & 0x3) != 0) {
        GPUDEBUG("Misaligned address\n");
        ret = 0xe0e02a01;
    }
    if (addr > 0x420000) {
        GPUDEBUG("Address out of range\n");
        ret = 0xe0e02a01;
    }
    if (length > 0x80) {
        GPUDEBUG("Too long\n");
        ret = 0xe0e02bec;
    }
    if (length & 0x3) {
        GPUDEBUG("Length misaligned\n");
        ret = 0xe0e02bf2;
    }

    if(ret == 0) {
        u32 i;

        for (i = 0; i < length; i += 4) {
            u32 reg = gpu_ReadReg32(addr + i);
            u32 mask = mem_Read32(inaddr + i);
            u32 val = mem_Read32(inaddr + i);

            GPUDEBUG("Writing %08x mask %08x to register %08x..\n", val, mask, addr + i);
            gpu_WriteReg32(addr + i, (reg & ~mask) | (val & mask));
        }
    }

    RESP(1, ret);
    return 0;
}

SERVICE_CMD(0x30082) // WriteHWRegsRepeat
{
    u32 inaddr  = CMD(4);
    u32 length  = CMD(2);
    u32 addr    = CMD(1);
    u32 ret     = 0;

    GPUDEBUG("GSPGPU_WriteHWRegsRepeat\n");

    if ((addr & 0x3) != 0) {
        GPUDEBUG("Misaligned address\n");
        ret = 0xe0e02a01;
    }
    if (addr > 0x420000) {
        GPUDEBUG("Address out of range\n");
        ret = 0xe0e02a01;
    }
    if (length > 0x80) {
        GPUDEBUG("Too long\n");
        ret = 0xe0e02bec;
    }
    if (length & 0x3) {
        GPUDEBUG("Length misaligned\n");
        ret = 0xe0e02bf2;
    }

    if(ret == 0) {
        u32 i;

        for (i = 0; i < length; i += 4) {
            u32 val = mem_Read32(inaddr + i);

            GPUDEBUG("Writing %08x to register %08x..\n", val, addr);
            gpu_WriteReg32(addr, val);
        }
    }

    RESP(1, ret);
    return 0;
}


SERVICE_CMD(0x40080) // ReadHWRegs
{
    u32 outaddr = EXTENDED_CMD(1);
    u32 length  = CMD(2);
    u32 addr    = CMD(1);
    u32 ret     = 0;

    GPUDEBUG("GSPGPU_ReadHWRegs addr=%08x to=%08x length=%08x\n", addr, outaddr, length);

    if ((addr & 0x3) != 0) {
        GPUDEBUG("Misaligned address\n");
        ret = 0xe0e02a01;
    }
    if (addr > 0x420000) {
        GPUDEBUG("Address out of range\n");
        ret = 0xe0e02a01;
    }
    if (length > 0x80) {
        GPUDEBUG("Too long\n");
        ret = 0xe0e02bec;
    }
    if (length & 0x3) {
        GPUDEBUG("Length misaligned\n");
        ret = 0xe0e02bf2;
    }

    if(ret == 0) {
        u32 i;
        for (i = 0; i < length; i += 4)
            mem_Write32((u32)(outaddr + i), gpu_ReadReg32((u32)(addr + i)));
    }

    RESP(1, ret);
    return 0;
}

SERVICE_CMD(0x50200) // SetBufferSwap
{
    GPUDEBUG("SetBufferSwap screen=%08x\n", CMD(1));

    u32 screen = CMD(1);
    u32 reg = screen ? 0x400554 : 0x400454;

    if(gpu_ReadReg32(reg) < 0x52) {
        // init screen
        // TODO: reverse this.
    }

    // TODO: Get rid of this:
    updateFramebufferaddr(arm11_ServiceBufferAddress() + 0x88, //don't use CMD(2) here it is not working!
                          screen & 0x1);

    screen_RenderGPU(); //display new stuff

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x80082) // FlushDataCache
{
    GPUDEBUG("FlushDataCache addr=%08x, size=%08x, h=%08x\n", CMD(1), CMD(2), CMD(4));
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0xB0040) // SetLcdForceBlack
{
    u8 flag = mem_Read8(arm11_ServiceBufferAddress() + 0x84);
    GPUDEBUG("SetLcdForceBlack %02x\n", flag);

    if (flag == 0)
        gpu_WriteReg32(0x202204, 0);
    else
        gpu_WriteReg32(0x202204, 0x1000000);
    if (flag == 0)
        gpu_WriteReg32(0x1ed02a04, 0);
    else
        gpu_WriteReg32(0x1ed02a04, 0x1000000);

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0xC0000) // TriggerCmdReqQueue
{
    GPUDEBUG("TriggerCmdReqQueue\n");
    gsp_ExecuteCommandFromSharedMem();

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00100040) // SetAxiConfigQoSMode
{
    GPUDEBUG("SetAxiConfigQoSMode\n");

    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x130042) // RegisterInterruptRelayQueue
{
    GPUDEBUG("RegisterInterruptRelayQueue %08x %08x\n",
          mem_Read32(arm11_ServiceBufferAddress() + 0x84),
          mem_Read32(arm11_ServiceBufferAddress() + 0x8C));

    u32 threadID = 0;
    u32 outMemHandle = 0;

    mem_Write32(arm11_ServiceBufferAddress() + 0x84,
                GPURegisterInterruptRelayQueue(mem_Read32(arm11_ServiceBufferAddress() + 0x84),
                        mem_Read32(arm11_ServiceBufferAddress() + 0x8C), &threadID, &outMemHandle));

    mem_Write32(arm11_ServiceBufferAddress() + 0x88, threadID);
    mem_Write32(arm11_ServiceBufferAddress() + 0x90, outMemHandle);
    return 0;
}

SERVICE_CMD(0x160042) // AcquireRight
{
    GPUDEBUG("AcquireRight %08x %08x --todo--\n", mem_Read32(arm11_ServiceBufferAddress() + 0x84), mem_Read32(arm11_ServiceBufferAddress() + 0x8C));
    mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
    return 0;
}

SERVICE_CMD(0x1E0080) // SetInternalPriorities
{
    GPUDEBUG("SetInternalPriorities %08x %08x %08x %08x %08x %08x %08x %08x --todo--\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(5), CMD(6), CMD(7), CMD(8));
    RESP(1, 0);
    return 0;
}

SERVICE_END();


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

    if (ID == 4)
    {
        extern int noscreen;
        if (!noscreen) {
            screen_HandleEvent();
            screen_RenderGPU();
        }
    }

}
