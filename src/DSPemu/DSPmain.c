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


#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "handles.h"
#include "mem.h"
#include "arm11.h"
#include "DSP.h"

#define disarm 1

u8 ram[0x10000];

u16 pc = 0;
static u32 Read32fromaddr(uint8_t p[4])
{
    u32 temp = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
    return temp;
}
static u16 fetch(u16 addr)
{
    u16 temp = ram[addr] | ram[addr + 1] << 8;
    return temp;
}
bool condmet(u8 con) //todo
{
    return true;
}
void stepdsp()
{
    u16 op = fetch(pc);
    if ((op & 0xFFF0) == 0x4180) //br
    {
        if (condmet(op & 0xF))
        {
            pc = fetch(pc + 2);
#ifdef disarm
            DEBUG("b%01X %04X\n",op&0xF,pc);
#endif
        }
        return;
    }
    if ((op & 0xFF70) == 0x8A60) //norm 10001010A110mmnn
    {
        return;
    }
    DEBUG("unknown op %04X\n",op);
}
void runDSP()
{
    while (1)stepdsp();
    return;
}

void loadDSP(u8* bin) //todo check sha256
{
    DEBUG("genocider:\"hello\"");
    DSPhead head;
    memcpy(&head, bin, sizeof(head));
    u32 magic = Read32fromaddr(head.magic);
    u32 contsize = Read32fromaddr(head.contentsize);
    u32 unk1 = Read32fromaddr(head.unk1);
    u32 unk6 = Read32fromaddr(head.unk6);
    u32 unk7 = Read32fromaddr(head.unk7);
    DEBUG("head %08X %08X %08X %08X %08X %02X %02X %02X %02X\n", magic, contsize, unk1, unk6, unk7, head.unk2, head.unk3, head.numsec, head.unk5);

    for (int i = 0; i < head.numsec;i++)
    {
        u32 dataoffset = Read32fromaddr(head.sement[i].dataoffset);
        u32 destoffset = Read32fromaddr(head.sement[i].destoffset);
        u32 size = Read32fromaddr(head.sement[i].size);
        u32 select = Read32fromaddr(head.sement[i].select);
        DEBUG("segment %08X %08X %08X %08X\n", dataoffset, destoffset, size, select);
        if (select == 0)//this is internal mem
        {
            memcpy(ram + destoffset, bin + dataoffset, size);
        }
    }
    runDSP();
}