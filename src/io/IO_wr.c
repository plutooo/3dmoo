/*
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

#include "armdefs.h"
#include "armemu.h"
#include "threads.h"
#include "gpu.h"

#include "IO/IO_reg.h"

u32 SPI2CNT = 0;
u32 SPI2DATA = 0;
u32 SPI2selected = 0;
u32 SPI2counter = 0;

u32 GPIOr24 = 0; //this is most likely the current status

s32 test = 0;

u8 IO_Read8(u32 addr)
{
    switch (addr)
    {
    case 0x1ec44001:
    {
        u32 temp = SPI2CNT;
        SPI2CNT &= ~0x80;
        return 0x15;
        break;
    }
    default:
       DEBUG("r8 %08x\n", addr);
       return 0x0;
    }
}
u16 IO_Read16(u32 addr)
{
    DEBUG("r16 %08x\n", addr);
    switch (addr)
    {
    case 0x1ec47014: //current stat?
        return 0x0020;
    default:
        return 0xFFFFFFFF;
    }
    return 0;
}
u32 IO_Read32(u32 addr)
{
    DEBUG("r32 %08x\n", addr);
    switch (addr)
    {
    case 0x1ec47014:
    
    case 0x1ec47024:
        return GPIOr24;
        break;
    default:
        return 0xFFFFFFFF;
    }
}
void IO_Write8(u32 addr,u8 v)
{
    switch (addr)
    {
    case 0x1ec44001:
    {
        SPI2CNT = v;
        if (SPI2CNT & 0x2) //Start
        {
            DEBUG("I2C device %02x\n", SPI2DATA);
            SPI2selected = SPI2DATA;
        }
        break;
    }
    case 0x1ec44000:
    {
        DEBUG("w8 %08x %02x\n", addr, v);
        SPI2DATA = v;
        break;
    }
    default:
        DEBUG("w8 %08x %02x\n", addr,v);
        break;
    }
}
void IO_Write16(u32 addr, u16 v)
{
    DEBUG("w16 %08x %04x\n", addr, v);
}
void IO_Write32(u32 addr, u32 v)
{
    DEBUG("w32 %08x %08x\n", addr, v);
    switch (addr)
    {
    case 0x1ec47024:
        GPIOr24 = v;
        break;
    default:
        break;
    }
}