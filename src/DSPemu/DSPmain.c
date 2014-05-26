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

u8 ram[0x20000];

//register
u16 pc = 0;
u16 sp = 0;
u16 r[6];
u16 rb = 0;
u16 y = 0;
u16 st[3];
u16 cfgi = 0;
u16 cfgj = 0;
u16 ph = 0;
u16 b[2];
u16 a[2];
u16 ext[4];
u16 sv = 0;
u32 onec = 0; //wtf is that I don't know


static u32 Read32fromaddr(uint8_t p[4])
{
    u32 temp = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
    return temp;
}
static u16 fetch(u16 addr)
{
    u16 temp = ram[addr*2] | ram[addr*2 + 1] << 8;
    return temp;
}
setregtyperrrrr(u8 type,u16 data)
{
    switch (type)
    {
    case 0:
        r[0] = data;
        break;
    case 1:
        r[1] = data;
        break;
    case 2:
        r[2] = data;
        break;
    case 3:
        r[3] = data;
        break;
    case 4:
        r[4] = data;
        break;
    case 5:
        r[5] = data;
        break;
    case 6:
        rb = data;
        break;
    case 7:
        y = data;
        break;
    case 8:
        st[0] = data;
        break;
    case 9:
        st[1] = data;
        break;
    case 10:
        st[2] = data;
        break;
    case 11:
        ph = data;
        break;
    case 12:
        pc = data;
        break;
    case 13:
        cfgi = data;
        break;
    case 14:
        cfgj = data;
        break;
    case 15:
        b[0] = data;
        break;
    case 16:
        b[1] = data;
        break;
    case 17:
        b[2] = data;
        break;
    case 18:
        b[3] = data;
        break;
    case 19:
        ext[0] = data;
        break;
    case 20:
        ext[1] = data;
        break;
    case 21:
        ext[2] = data;
        break;
    case 22:
        ext[3] = data;
        break;
    default:
        DEBUG("set unknown reg %02X",type);
        break;

    }
}
bool condmet(u8 con) //todo
{
    return true;
}
/*Modification of rN:
mm = 00 No modification
01 +1
10 -1
11 + step*/

/*
inter RESET 0x0
inter TRAP/BI 0x2
inter NMI 0x4
inter INT0 0x6
inter INT1 0xE
inter INT2 0x16
*/
void stepdsp()
{
    u16 op = fetch(pc);
    if ((op & 0xFFF0) == 0x4180) //br
    {
        if (condmet(op & 0xF))
        {
            pc = fetch(pc + 1);
#ifdef disarm
            DEBUG("b%01X %04X\n",op&0xF,pc);
#endif
        }
        return;
    }
    if (op == 0)
    {
#ifdef disarm
        DEBUG("nop\n");
#endif
        pc++;
        return;
    }
    if ((op & 0xFF00) == 0x400)//load page 00000100vvvvvvvv
    {
        st[1] = st[1] & 0XFF00 | op & 0xFF;
#ifdef disarm
        DEBUG("ldr page 0x%02X\n",op&0xFF);
#endif
        pc++;
        return;
    }
    if ((op & 0xF000) == 0x6000) //ALU #short immediate 1100XXXAvvvvvvv
    {
        u16 temp = a[0];
        if (op & 0x0100)temp = a[1];
        u8 var = op & 0xFF;
        switch (op & 0x0E00)
        {
        case 0x0: //or
            temp = temp | var;
            break;
        case 0x200: //and
            temp = temp & var;
            break;
        case 0x400: //xor
            temp = temp ^ var;
            break;
        case 0x600: //add
            temp = temp + var;
            break;
        case 0x800:
            DEBUG("unknown op %04X\n", op);
            break;
        case 0xA00:
            DEBUG("unknown op %04X\n", op);
            break;
        case 0xC00: //cmp
            DEBUG("unsupported cmp %04X\n", op);
            break;
        case 0xE00: //sub
            temp = temp - var;
            break;
        }
        if (op & 0x0100)
        {
            a[1] = temp;
        }
        else
        {
            a[0] = temp;
        }
        pc++;
        return;
    }
    /*if ((op & 0xFF80) == 0x100)//movs register, AB 000000010abrrrrr
    {

    }*/
    pc++;
    DEBUG("unknown op %04X\n",op);
}
void runDSP()
{
    pc = 0x0; //rest
    while (1)stepdsp();
    return;
}

void loadDSP(u8* bin) //todo check sha256
{
    DEBUG("genocider:\"hello\"\n");
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
        memcpy(ram + destoffset * 2, bin + dataoffset, size * 2);
    }
    runDSP();
}