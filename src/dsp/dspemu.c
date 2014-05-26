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
#include "dsp.h"

#define DISASM 1

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
    switch (type) {
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
const char* ops[] = {
    "or", "and", "xor", "add", "tst0_a", "tst1_a",
    "cmp", "sub", "msu", "addh", "addl", "subh", "subl",
    "sqr", "sqra", "cmpu"
};

const char* ops3[] = {
    "or", "and", "xor", "add",
    "invalid!", "invalid!", "cmp", "sub"
};

const char* alb_ops[] = {
    "set", "rst", "chng", "addv", "tst0_mask1", "tst0_mask2", "cmpv", "subv"
};

const char* mm[] = {
    "nothing", "+1", "-1", "+step"
};

const char* rrrrr[] = {
    "r0", "r1", "r2", "r3",
    "r4", "r5", "rb", "y",
    "st0", "st1", "st2", "p/ph",
    "pc", "sp", "cfgi", "cfgj",
    "b0h", "b1h", "b0l", "b1l",
    "ext0", "ext1"
};

int HasOp3(u16 op) {
    u16 opc = (op >> 9) & 0x7;

    if(opc != 4 && opc != 5)
        return opc;

    return -1;
}

void DSP_Step()
{
    u16 op = fetch(pc);
    u8 ax = op & (0x100) ? 1 : 0; // use a0 or a1

    switch(op >> 12) {
    case 0:
        if((op & 0xFE00) == 0x0E00) {
            // TODO: divs
            DEBUG("divs??\n");
        }
        break;

    case 4:
        if(!(op & 0x8000)) {
            u16 op3 = HasOp3(op);

            if(op3 != -1) {
                // ALU (rb + #offset7), ax
                DEBUG("%s (rb + %02), a%d\n", ops3[op3], op & 0x7F, ax);
                break;
            }
            DEBUG("?\n");
        }
        DEBUG("?\n");
        break;

    case 0x8:
    case 0x9:
        if((op >> 6) & 0x7 == 4) {
            // ALM (rN)
            DEBUG("%s (r%d), a%d (modifier=%s)\n", ops[(op >> 9) & 0xF], op & 0x7, ax, mm[(op >> 3) & 3]);
            break;
        }
        else if((op >> 6) & 0x7 == 5) {
            // ALM register
            u16 r = op & 0x1F;
            
            if(r < 22) {
                DEBUG("%s %s, a%d\n", ops[(op >> 9) & 0xF], rrrrr[r], ax);
                break;
            }

            DEBUG("?\n");
        }
        else if((op >> 6) & 0x7 == 7) {
            u16 extra = fetch(pc+1);

            if(!(op & 0x100)) {
                // ALB (rN)
                DEBUG("%s (r%d), %04x (modifier=%s)\n", alb_ops[(op >> 9) & 0x7], op & 0x7,
                      extra & 0xFFFF, mm[(op >> 3) & 3]);
            }
            else {
                // ALB register
                u16 r = op & 0x1F;
           
                if(r < 22) {
                    DEBUG("%s %s, %04x\n", alb_ops[(op >> 9) & 0x7], rrrrr[r], extra & 0xFFFF);
                    break;
                }

                DEBUG("?\n");
            }
            break;
        }
        else if((op & 0xF0FF) == 0x80C0) {
            u16 op3 = HasOp3(op);

            if(op3 != -1) {
                // ALU ##long immediate
                u16 extra = fetch(pc+1);

                DEBUG("%s %04x, a%d\n", ops3[op3], extra & 0xFFFF, ax);
                break;
            }

            DEBUG("?\n");
            break;
        }
        else if((op & 0xFF70) == 0x8A60) {
            // TODO: norm
            DEBUG("norm??\n");
            break;
        }
        break;

    case 0xA:
    case 0xB:
        // ALM direct
        DEBUG("%s %02x, a%d\n", ops[(op >> 9) & 0xF], op & 0xFF, ax);
        break;

    case 0xC: {
        u16 op3 = HasOp3(op);

        if(op3 != -1) {
            // ALU #short immediate
            DEBUG("%s %02x, a%d\n", ops3[op3], op & 0xFF, ax);
            break;
        }
        DEBUG("?\n");
        break;
    }


    case 0xD:
        if(op & 0xFED8 == 0xD4D8) {
            u16 op3 = HasOp3(op);

            if(op3 != -1) {
                u16 extra = fetch(pc+1);
                if(op & (1 << 5)) {  // ALU [##direct add.],ax
                    DEBUG("%s [%04x], a%d\n",
                          ops3[op3],
                          extra & 0xFFFF,
                          op & (0x1000) ? 1 : 0);
                    break;
                }
                else { // ALU (rb + ##offset),ax
                    DEBUG("%s (rb + %04x), a%d\n",
                          ops3[op3],
                          extra & 0xFFFF, 
                          op & (0x1000) ? 1 : 0);
                    break;
                }
            }
        }

    case 0xE:
        // ALB direct
        if(op & (1 << 8)) {
            u16 extra = fetch(pc+1);

            DEBUG("%s %02x, %04x\n", alb_ops[(op >> 9) & 0x7], op & 0xFF, extra & 0xFFFF);
            break;
        }

        DEBUG("?\n");
        break;
    default:
        DEBUG("?\n");
    }
    pc++;
}

void DSP_Run()
{
    pc = 0x0; //reset
    while (1)
        DSP_Step();
    return;
}

void DSP_LoadFirm(u8* bin) //todo check sha256
{
    dsp_header head;
    memcpy(&head, bin, sizeof(head));

    u32 magic = Read32fromaddr(head.magic);
    u32 contsize = Read32fromaddr(head.content_sz);
    u32 unk1 = Read32fromaddr(head.unk1);
    u32 unk6 = Read32fromaddr(head.unk6);
    u32 unk7 = Read32fromaddr(head.unk7);

    DEBUG("head %08X %08X %08X %08X %08X %02X %02X %02X %02X\n",
          magic, contsize, unk1, unk6, unk7, head.unk2, head.unk3, head.num_sec, head.unk5);

    for (int i = 0; i < head.num_sec; i++) {
        u32 dataoffset = Read32fromaddr(head.segment[i].data_offset);
        u32 destoffset = Read32fromaddr(head.segment[i].dest_offset);
        u32 size = Read32fromaddr(head.segment[i].size);
        u32 select = Read32fromaddr(head.segment[i].select);

        DEBUG("segment %08X %08X %08X %08X\n", dataoffset, destoffset, size, select);
        memcpy(ram + destoffset * 2, bin + dataoffset, size * 2);
    }

    DSP_Run();
}
