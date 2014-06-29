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

#define CPUsvcbuffer 0xFFFF0000

u32 gsp_gpu_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);



    u32 outaddr;
    u32 lange;
    u32 addr;
    switch (cid) {
    case 0x10082: //GSPGPU_WriteHWRegs(u32 regAddr, u32* data, u8 size)
    {
        outaddr = mem_Read32(CPUsvcbuffer + 0x90);
        lange = mem_Read32(CPUsvcbuffer + 0x88);
        addr = mem_Read32(CPUsvcbuffer + 0x84);
        DEBUG("GSPGPU_WriteHWRegs %08x %08x %08x", addr, outaddr, lange);
        if ((lange & 0x3) != 0) DEBUG("nicht surportete Länge");
        for (u32 i = 0; i < lange; i += 4) GPUwritereg32((u32)(addr + i), mem_Read32((u32)(outaddr + i)));
        return 0;
    }
    case 0x40080: //GSPGPU_ReadHWRegs(u32 regAddr, u32* data, u8 size)
    {
        outaddr = mem_Read32(CPUsvcbuffer + 0x184);
        lange = mem_Read32(CPUsvcbuffer + 0x88);
        addr = mem_Read32(CPUsvcbuffer + 0x84);
        DEBUG("GSPGPU_ReadHWRegs %08x %08x %08x", addr, outaddr, lange);
        if ((lange & 0x3) != 0) DEBUG("nicht surportete Länge");
        for (u32 i = 0; i < lange; i += 4) mem_Write32((u32)(outaddr + i), GPUreadreg32((u32)(addr + i)));
        return 0;
    }
    case 0xB0040: //SetLcdForceBlack
    {
        DEBUG("SetLcdForceBlack");
        unsigned char* buffer = get_pymembuffer(0x18000000);
        memset(buffer, 0, 0x46500 * 6);
        return 0;
    }
    case 0xC0000: //TriggerCmdReqQueue
    {
        DEBUG("TriggerCmdReqQueue\n");
        GPUTriggerCmdReqQueue();
        mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
        return 0;
    }
    case 0x130042: //RegisterInterruptRelayQueue
    {
                       DEBUG("RegisterInterruptRelayQueue %08x %08x\n", mem_Read32(CPUsvcbuffer + 0x84), mem_Read32(CPUsvcbuffer + 0x8C));
                       u32 threadID = 0;
                       u32 outMemHandle = 0;
                       GPURegisterInterruptRelayQueue(mem_Read32(CPUsvcbuffer + 0x84), mem_Read32(CPUsvcbuffer + 0x8C), &threadID, &outMemHandle);
                       mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
                       mem_Write32(CPUsvcbuffer + 0x88, threadID);
                       mem_Write32(CPUsvcbuffer + 0x90, outMemHandle);
                       return 0;
    }
    case 0x160042: //AcquireRight
    {
                       DEBUG("AcquireRight %08x %08x", mem_Read32(CPUsvcbuffer + 0x84), mem_Read32(CPUsvcbuffer + 0x8C));
                       mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
                       return 0;
    }
    default:
        DEBUG("STUBBED GPUGSP %x %x %x %x\n", mem_Read32(CPUsvcbuffer + 0x80), mem_Read32(CPUsvcbuffer + 0x84), mem_Read32(CPUsvcbuffer + 0x88), mem_Read32(CPUsvcbuffer + 0x8C));
    }
    PAUSE();

    return 0;
}
