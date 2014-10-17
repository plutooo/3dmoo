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
#include "handles.h"
#include "mem.h"
#include "arm11.h"

u32 mutex_handle;

u32 myeventhandel = 0;

#define DSPramaddr 0x1FF00000

void initDSP()
{
    mutex_handle = handle_New(HANDLE_TYPE_EVENT, 0);
}

u32 dsp_dsp_SyncRequest()
{
    u32 cid = mem_Read32(arm11_ServiceBufferAddress() + 0x80);

    // Read command-id.
    switch(cid) {
    case 0x001100c2: { //LoadComponent
        u32 size = mem_Read32(arm11_ServiceBufferAddress() + 0x84);
        u32 unk1 = mem_Read16(arm11_ServiceBufferAddress() + 0x88);
        u32 unk2 = mem_Read16(arm11_ServiceBufferAddress() + 0x8C);
        u32 buffer = mem_Read32(arm11_ServiceBufferAddress() + 0x94);
        DEBUG("LoadComponent %08X %08X %04X %04X\n", size, buffer, unk1, unk2);
        FILE* out = fopen("dspfirm.bin", "wb");
        u8* outbuff = (u8*)malloc(size);
        mem_Read(outbuff, buffer, size);
        fwrite(outbuff, size, 1, out);
        fclose(out);

        //DSP_LoadFirm(outbuff);

        free(outbuff);
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0x0); //no error
        mem_Write32(arm11_ServiceBufferAddress() + 0x88, 0x1); //loaded
        return 0;
    }
    case 0x00130082: { //FlushDataCache
        DEBUG("FlushDataCache\n");
        return 0;
    }
    case 0x00140082: { //InvalidateDCache
        DEBUG("InvalidateDCache\n");
        return 0;
    }
    case 0x00150082: { //RegisterInterruptEvents
        u32 param0 = mem_Read32(arm11_ServiceBufferAddress() + 0x84);
        u32 param1 = mem_Read32(arm11_ServiceBufferAddress() + 0x88);
        myeventhandel = mem_Read32(arm11_ServiceBufferAddress() + 0x90);
        DEBUG("RegisterInterruptEvents %08X %08X %08X\n", param0, param1, myeventhandel);
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    case 0x00160000: { //GetSemaphoreEventHandle
        mem_Write32(arm11_ServiceBufferAddress() + 0x8C, mutex_handle);
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    case 0x00170040: { //SetSemaphoreMask
        u32 mask = mem_Read16(arm11_ServiceBufferAddress() + 0x84);
        DEBUG("SetSemaphoreMask %04X\n", mask);
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    case 0x000D0082: { //WriteProcessPipe
        u32 numb = mem_Read32(arm11_ServiceBufferAddress() + 0x84);
        u32 size = mem_Read32(arm11_ServiceBufferAddress() + 0x88);
        u32 buffer = mem_Read32(arm11_ServiceBufferAddress() + 0x90);
        DEBUG("WriteProcessPipe %08X %08X %08X\n", numb, size, buffer);
        for (unsigned int i = 0; i < size; i++) {
            if (i % 16 == 0) printf("\n");
            printf("%02X ", mem_Read8(buffer + i));
        }
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    case 0x00070040: { //WriteReg0x10 todo sound_marker
        u32 numb = mem_Read16(arm11_ServiceBufferAddress() + 0x84);
        DEBUG("WriteReg0x10 %04X\n", numb);
        handleinfo* h = handle_Get(myeventhandel);
        h->locked = false;
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    case 0x001000c0: { //ReadPipeIfPossible
        u32 unk1 = mem_Read32(arm11_ServiceBufferAddress() + 0x84);
        u32 unk2 = mem_Read32(arm11_ServiceBufferAddress() + 0x88);
        u32 size = mem_Read16(arm11_ServiceBufferAddress() + 0x8C);
        u32 buffer = mem_Read32(arm11_ServiceBufferAddress() + 0x184);
        DEBUG("ReadPipeIfPossible %08X %08X %04X %08X\n", unk1, unk2, size, buffer);
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    case 0x000c0040: { //ConvertProcessAddressFromDspDram
        u32 addrin = mem_Read32(arm11_ServiceBufferAddress() + 0x84);
        DEBUG("ConvertProcessAddressFromDspDram %08X\n", addrin);
        mem_Write32(arm11_ServiceBufferAddress() + 0x88, DSPramaddr + addrin + 0x40000);
        mem_Write32(arm11_ServiceBufferAddress() + 0x84, 0); //no error
        return 0;
    }
    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}
