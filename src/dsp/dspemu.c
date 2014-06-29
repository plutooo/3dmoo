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
#include "handles.h"
#include "mem.h"
#include "arm11.h"
#include "dsp.h"



#undef DEBUG
#define DEBUG(...) do {                                 \
    int old = color_green();                        \
    fprintf(stdout, "%04x: ", pc);              \
    color_restore(old);                             \
    fprintf(stdout, __VA_ARGS__);                   \
} while (0);

//#define DISASM 1
#define EMULATE 1

u8 ram[0x20000];

//register
u16 stt[3];
u16 mode[4];
u16 pc = 0;
u16 sp = 0;
u16 r[6];
u16 rb = 0;
u16 y = 0;
u16 st[3];
u16 cfgi = 0;
u16 cfgj = 0;
u32 ph = 0;
u32 b[2];
u32 a[2];
u16 ext[4];
u16 sv = 0;
u16 lc = 0; //wtf is that I don't know


static u32 Read32(uint8_t p[4])
{
    u32 temp = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
    return temp;
}

static u16 FetchWord(u16 addr)
{
    u16 temp = ram[addr*2] | (ram[addr*2 + 1] << 8);
    return temp;
}

static void writeWord(u16 addr,u16 data)
{
    ram[addr * 2] = data&0xFF;
    ram[addr * 2 + 1] = (data >> 8) & 0xFF;
}

void DSPwrite16_8(u8 addr, u16 data)
{
    writeWord(addr | (st[1] << 8), data);
}
void DSPwrite16_16(u16 addr, u16 data)
{
    writeWord(addr, data);
}

u16 DSPread16_8(u8 addr)
{
    return FetchWord(addr | (st[1] << 8));
}
u16 DSPread16_16(u16 addr)
{
    return FetchWord(addr);
}


s32 getlastbit(u32 val)
{
    if (val & 0x80000000)return 31;
    if (val & 0x40000000)return 30;
    if (val & 0x20000000)return 29;
    if (val & 0x10000000)return 28;
    if (val & 0x8000000)return 27;
    if (val & 0x4000000)return 26;
    if (val & 0x2000000)return 25;
    if (val & 0x1000000)return 24;
    if (val & 0x800000)return 23;
    if (val & 0x400000)return 22;
    if (val & 0x200000)return 21;
    if (val & 0x100000)return 20;
    if (val & 0x80000)return 19;
    if (val & 0x40000)return 18;
    if (val & 0x20000)return 17;
    if (val & 0x10000)return 16;
    if (val & 0x8000)return 15;
    if (val & 0x4000)return 14;
    if (val & 0x2000)return 13;
    if (val & 0x1000)return 12;
    if (val & 0x800)return 11;
    if (val & 0x400)return 10;
    if (val & 0x200)return 9;
    if (val & 0x100)return 8;
    if (val & 0x80)return 7;
    if (val & 0x40)return 6;
    if (val & 0x20)return 5;
    if (val & 0x10)return 4;
    if (val & 0x8)return 3;
    if (val & 0x4)return 2;
    if (val & 0x2)return 1;
    if (val & 0x1)return 0;
    
    return -1; //that is needed
}
void updatecmpflags(u32 data, u8 MSB, bool v, bool c)
{
    u16 temp = st[0] & 0xF01F;
    if (data == 0 && MSB == 0)temp |= 0x800; //Z
    if (MSB & 0x8) temp |= 0x400; //M
    if (data & 0xC0000000) temp |= 0x200; //N
    if (v) temp |= 0x100; //V
    if (c) temp |= 0x80; //C
    if ((MSB != 0 && (data & 0x80000000)) || (MSB != 0xF && (!(data & 0x80000000)))) temp |= 0x40; //E
    if (v) temp |= 0x20; //L
    st[0] = temp;
}


u32 doops3(u8 op3, u32 in, u32 in2,u8 *MSB)
{
    switch (op3)
    {
    case 0: //or
    {
              u32 ret = in | in2;
              u16 temp = st[0] & 0xF0BF;
              if (ret == 0 && *MSB == 0)temp |= 0x800; //Z
              if (*MSB & 0x8) temp |= 0x400; //M
              if (ret & 0xC0000000) temp |= 0x200; //N
              if ((*MSB != 0 && (ret & 0x80000000)) || (*MSB != 0xF && (!(ret & 0x80000000)))) temp |= 0x40; //E
              st[0] = temp;
              return ret;
    }
    case 3:
    {
              u8 temp2 = *MSB;
              u32 temp1 = in + in2;
              if (in < temp1) temp2++;
              bool v = false;
              if (temp2 & 0x10)v = true;
              bool c = false;
              if (getlastbit(*MSB) != getlastbit(temp2))c = true;
              if (temp2 == 0 && getlastbit(in) > getlastbit(temp1))c = true;
              updatecmpflags(temp1, temp2, v, c);
              *MSB = temp2;
              return temp1;
    }
    case 7:
    {
              u8 temp2 = *MSB;
              u32 temp1 = in - in2;
              if (in < temp1) temp2--;
              bool v = false;
              if (temp2 & 0x10)v = true;
              bool c = false;
              if (getlastbit(*MSB) != getlastbit(temp2))c = true;
              if (temp2 == 0 && getlastbit(in) < getlastbit(temp1))c = true;
              updatecmpflags(temp1, temp2, v, c);
              *MSB = temp2;
              return temp1;
    }
    default:
        DEBUG("unimplemented op3 %02x\n", op3);
        break;
    }

    return -1;
}

u16 getrrrrr(u8 op)
{
    switch (op)
    {
    case 0:
        return r[0];
    case 1:
        return r[1];
    case 2:
        return r[2];
    case 3:
        return r[3];
    case 4:
        return r[4];
    case 5:
        return r[5];
    case 6:
        return rb;
    case 7:
        return y;
    case 8:
        return st[0];
    case 9:
        return st[1];
    case 0xA:
        return st[2];
    case 0xB:
        return ph;
    case 0xC:
        return pc;
    case 0xD:
        return sp;
    case 0xE:
        return cfgi;
    case 0xF:
        return cfgj;
    case 0x10:
        return ((b[0] >> 16) & 0xFFFF);
    case 0x11:
        return ((b[1] >> 16) & 0xFFFF);
    case 0x12:
        return b[0] & 0xFFFF;
    case 0x13:
        return b[1] & 0xFFFF;
    case 0x14:
        return ext[0];
    case 0x15:
        return ext[1];
    case 0x16:
        return ext[2];
    case 0x17:
        return ext[3];
    case 0x18:
        DEBUG("get a0 rrrrr todo");
        return a[0];
    case 0x19:
        DEBUG("get a1 rrrrr todo");
        return a[1];
    case 0x1A:
        return ((a[0] >> 16) & 0xFFFF);
        break;
    case 0x1B:
        return ((a[1] >> 16) & 0xFFFF);
        break;
    case 0x1C:
        return a[0] & 0xFFFF;
        break;
    case 0x1D:
        return a[1] & 0xFFFF;
    case 0x1E:
        return sv;
    case 0x1F:
        return lc;
    }
    return 0;
}
void setrrrrr(u8 op, u16 data)
{
    switch (op)
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
    case 0xA:
        st[2] = data;
        break;
    case 0xB:
        ph = data;
        break;
    case 0xC:
        pc = data - 1; //because there is a pc++ at the end
        break;
    case 0xD:
        sp = data;
        break;
    case 0xE:
        cfgi = data;
        break;
    case 0xF:
        cfgj = data;
        break;
    case 0x10:
        b[0] = (data << 16) | (b[0] & 0xFFFF);
        break;
    case 0x11:
        b[1] = (data << 16) | (b[1] & 0xFFFF);
        break;
    case 0x12:
        b[0] = data | (b[0] & 0xFFFF0000);
        break;
    case 0x13:
        b[1] = data | (b[1] & 0xFFFF0000);
        break;
    case 0x14:
        ext[0] = data;
        break;
    case 0x15:
        ext[1] = data;
        break;
    case 0x16:
        ext[2] = data;
        break;
    case 0x17:
        ext[3] = data;
        break;
    case 0x18:
        a[0] = data;
        st[0] = st[0] & 0x0FFF; //clear MSB
        break;
    case 0x19:
        a[1] = data;
        st[1] = st[1] & 0x0FFF; //clear MSB
        break;
    case 0x1A:
        a[0] = (data << 16) | (a[0] & 0xFFFF);
        break;
    case 0x1B:
        a[1] = (data << 16) | (a[1] & 0xFFFF);
        break;
    case 0x1C:
        a[0] = data | (a[0] & 0xFFFF0000);
        break;
    case 0x1D:
        a[1] = data | (a[1] & 0xFFFF0000);
        break;
    case 0x1E:
        sv = data;
        break;
    case 0x1F:
        lc = data;
        break;
    }
}






/*
inter RESET 0x0
inter TRAP/BI 0x2
inter NMI 0x4
inter INT0 0x6
inter INT1 0xE
inter INT2 0x16
*/
const char* mulXXX[] = {
    "mpy", "mpysu", "mac", "macus",
    "maa", "macuu", "macsu", "maasu"
};
const char* morpone[] = {
    "stt0", "stt1", "stt2", "wrong_ModStt", "mod0", "mod1", "mod2", "mod3"
};
const char* morptwo[] = {
    "ar0", "ar1", "arp0", "arp1", "arp2", "arp3", "wrong_AddrRegs", "wrong_AddrRegs"
};

const char* mulXX[] = {
    "mpy", "mac",
    "maa", "macsu"
};

const char* ops[] = {
    "or", "and", "xor", "add", "tst0_a", "tst1_a",
    "cmp", "sub", "msu", "addh", "addl", "subh", "subl",
    "sqr", "sqra", "cmpu"
};


void doops(u8 ops, u32 data1, u8 *MSB1, u32 data2, u8 MSB2)
{
    switch (ops)
    {
    case 15:
    {
               u8 temp2 = *MSB1 - MSB2;
               u32 temp1 = data1 - data2;
               if (data1 < data2) temp2--;
               bool v = false;
               if (temp2 & 0x10)v = true;
               bool c = false;
               if (getlastbit(*MSB1) > getlastbit(temp2))c = true;
               if (temp2 == 0 && getlastbit(data1) > getlastbit(temp1))c = true;
               updatecmpflags(temp1, temp2, v, c);
               break;
    }
    default:
        DEBUG("unknown ops %02x\n",ops);
        break;
    }
}

const char* ops3[] = {
    "or", "and", "xor", "add",
    "invalid!", "invalid!", "cmp", "sub"
};
const char* restep[] = {
    "stepi0", "stepj0"
};

const char* alb_ops[] = {
    "set", "rst", "chng", "addv", "tst0_mask1", "tst0_mask2", "cmpv", "subv"
};
u16 doalb_ops(u8 ops,u16 data1, u16 data2)
{
    switch (ops)
    {
    case 0:
    {
              u16 ret = data1 | data2;
              u16 temp = st[0] & 0xF3FF;
              if (ret == 0)temp |= 0x800; //Z
              if (ret & 0x8000) temp |= 0x400; //M
              st[0] = temp;
    }
    case 1:
    {
              u16 ret = data1 & ~data2;
              u16 temp = st[0] & 0xF3FF;
              if (ret == 0)temp |= 0x800; //Z
              if (ret & 0x8000) temp |= 0x400; //M
              st[0] = temp;
    }
    case 3: //addv
    {
                u16 ret = data1 + data2;
                u16 temp = st[0] & 0xF37F;
                if (ret == 0)temp |= 0x800; //Z
                if (ret & 0x8000) temp |= 0x400; //M
                if (getlastbit(ret) > getlastbit(data1)) temp |= 0x80; //C
                st[0] = temp;
                return ret;
    }
    default:
        DEBUG("unknown alb_ops\n");
        return data1;
    }
}

const char* mm[] = {
    "nothing", "+1", "-1", "+step"
};
const char* rrrrr[] = {
    "r0",
    "r1",
    "r2",
    "r3",
    "r4",
    "r5",
    "rb",
    "y",
    "st0",
    "st1",
    "st2",
    "p / ph",
    "pc",
    "sp",
    "cfgi",
    "cfgj",
    "b0h",
    "b1h",
    "b0l",
    "b1l",
    "ext0",
    "ext1",
    "ext2",
    "ext3",
    "a0",
    "a1",
    "a0l",
    "a1l",
    "a0h",
    "a1h",
    "1c",
    "sv"
};

const char* AB[] = {
    "b0", "b1", "a0", "a1"
};

const char* cccc[] = {
    "true", "eq", "neg", "gt",
    "ge", "1t", "le", "nn",
    "c", "v", "e", "l",
    "nr", "niu0", "iu0", "iu1"
};

const char* fff[] = {
    "shr", "shr4", "shl", "shl4",
    "ror", "rol", "clr", "reserved"

};
const char* rNstar[] = {
    "r0", "r1", "r2", "r3",
    "r4", "r5", "rb", "y"
};
const char* ABL[] = {
    "B0L", "B0H", "B1L", "B1H", "A0L", "A0H", "A1L", "A1H"
};
u16 getABL(u8 op)
{
    switch (op)
    {
    case 0:
        return b[0] & 0xFFFF;
    case 1:
        return (b[0]>>16) & 0xFFFF;
    case 2:
        return b[1] & 0xFFFF;
    case 3:
        return (b[1] >> 16) & 0xFFFF;
    case 4:
        return a[0] & 0xFFFF;
    case 5:
        return (a[0] >> 16) & 0xFFFF;
    case 6:
        return a[1] & 0xFFFF;
    case 7:
        return (a[1] >> 16) & 0xFFFF;
    default:
        DEBUG("getABL error %02x\n",op);
        return 0;
    }
}

const char* swap[] = {
    "a0 <-> b0",
    "a0 <-> b1",
    "a1 <-> b0",
    "a1 <-> b1",
    "a0 <-> b0 and a1 -> b1",
    "a0 <-> b1 and a1 -> b0",
    "a0 -> b0 and -> a1",
    "a0 -> b1 -> a1",
    "a1 -> b0 -> a0",
    "a1 -> b0 ->a0",
    "b0 -> a1 -> b1",
    "b0 -> a1 -> b1",
    "b1 -> a0 -> b0",
    "b1 -> a1 -> b0",
    "unk.",
    "unk."
}; 

const char* ffff[] = {
    "shr",
    "shr4",
    "shl",
    "shl4",
    "ror",
    "rol",
    "clr",
    "reserved",
    "not",
    "neg",
    "rnd",
    "pacr",
    "clrr",
    "inc",
    "dec",
    "copy"
};
u32 doffff(u32 data,u8 ffff,u8* MSB)
{
    switch (ffff)
    {
    case 6: //clr
    {
                u16 temp = st[0] & 0xF0BF;
                temp |= 0x800; //Z
                //M
                //N
                //E
                st[0] = temp;
                *MSB = 0;
                return 0x0;
    }
    case 0xc: //clrr
    {
                 u16 temp = st[0] & 0xF0BF;
                 //Z
                 //M
                 //N
                 //E
                 st[0] = temp;
                 *MSB = 0;
                 return 0x8000;
    }
    case 0xD: //inc
    {
                  u8 temp2 = *MSB;
                  u32 temp1 = data + 1;
                  if (data == 0) temp2++;
                  bool v = false;
                  if (temp2 & 0x10)v = true;
                  bool c = false;
                  if (getlastbit(*MSB) != getlastbit(temp2))c = true;
                  if (temp2 == 0 && getlastbit(data) > getlastbit(temp1))c = true;
                  updatecmpflags(temp1, temp2, v, c);
                  *MSB = temp2;
                  return temp1;
    }
    case 0xE: //dec
    {
                  u8 temp2 = *MSB;
                  u32 temp1 = data - 1;
                  if (data == 0) temp2--;
                  bool v = false;
                  if (temp2 & 0x10)v = true;
                  bool c = false;
                  if (getlastbit(*MSB) != getlastbit(temp2))c = true;
                  if (temp2 == 0 && getlastbit(data) < getlastbit(temp1))c = true;
                  updatecmpflags(temp1, temp2, v, c);
                  *MSB = temp2;
                  return temp1;
    }
    default:
        DEBUG("unknown ffff %02x\n",ffff);
        return data;
        break;
    }
}

int HasOp3(u16 op)
{
    u16 opc = (op >> 9) & 0x7;

    if(opc != 4 && opc != 5)
        return opc;

    return -1;
}

bool cccccheck(u8 cccc)
{
    switch (cccc)
    {
    case 0:
        return true;
    case 1:
        if (st[0] & 0x800)return true; //Z == 1
        return false;
    case 2:
        if (st[0] & 0x800)return false;
        return true; //Z == 0
    case 3:
        if (st[0] & 0x400 || st[0] & 0x800)return false;
        return true; //M = 0 and Z = 0
    case 4:
        if (st[0] & 0x400)return false;
        return true; //M == 0
    default:
        DEBUG("unk. cccc %d\n",cccc);
        return true;
    }
}
u16 fixending(u16 in, u8 pos)
{
    if (in & (1 << (pos - 1)))
    {
        return (in | (0xFFFF << (pos - 1)));
    }
    return in;
}

u16 getmorpone(u16 morpone)
{
    switch (morpone)
    {
    case 0:
        return stt[0];
    case 1:
        return stt[1];
    case 2:
        return stt[2];
    case 4:
        return mode[0];
    case 5:
        return mode[1];
    case 6:
        return mode[2];
    case 7:
        return mode[3];
    default:
        DEBUG("unknown morpone %02x\n", morpone);
        return 0;
    }
}
void setmorpone(u16 morpone, u16 data)
{
    switch (morpone)
    {
    case 0:
        stt[0] = data;
        break;
    case 1:
        stt[1] = data;
        break;
    case 2:
        stt[2] = data;
        break;
    case 4:
        mode[0] = data;
        break;
    case 5:
        mode[1] = data;
        break;
    case 6:
        mode[2] = data;
        break;
    case 7:
        mode[3] = data;
        break;
    default:
        DEBUG("unknown morpone %02x\n", morpone);
        break;
    }
}
void DSP_Step()
{
    // Currently a disassembler.
    // Implemented up to MUL y, (rN).

    u16 op = FetchWord(pc);
    u8 ax = op & (0x100) ? 1 : 0; // use a0 or a1

    switch(op >> 12) {
    case 0:
        if ((op&~0x7) == 0x8)
        {
            u16 extra = FetchWord(pc + 1);
            DEBUG("mov %04x, %s\n", extra, morptwo[op & 0x7]);
            pc++;
            break;
        }
        if ((op&~0x7) == 0x30)//correct this may be wrong
        {
            u16 extra1 = FetchWord(pc + 1);
#ifdef DISASM
            DEBUG("mov #0x%04x,%s\n", extra1, morpone[op&0x7]);
#endif
#ifdef EMULATE
            setmorpone(op & 0x7,extra1);
#endif
            pc++;
            break;
        }
        if (op == 0x20)
        {
            DEBUG("trap\n");
            break;
        }
        if (op == 0)
        {
#ifdef DISASM
            DEBUG("nop\n");
#endif
            break;
        }
        if ((op & 0xE00) == 0x200)
        {
            DEBUG("load modi #%04x\n", op & 0x1FF);
            break;
        }
        if ((op & 0xE00) == 0xA00)
        {
            DEBUG("load modj #%04x\n", op & 0x1FF);
            break;
        }
        if ((op & 0xF00) == 0x400)
        {
#ifdef DISASM
            DEBUG("load page #%02x\n", op & 0xFF);
#endif 
#ifdef EMULATE
            st[1] = (st[1] & 0xFF00) | (op & 0xFF);
#endif
            break;
        }
        if ((op & 0xF80) == 0x80)
        {
            DEBUG("rets (r%d) (modifier=%s) (disable=%d)\n", op & 0x7, mm[(op >> 3) & 3],(op>>5)&0x1);
            break;
        }
        if ((op & 0xF00) == 0x900)
        {
            DEBUG("rets #%02x\n", op&0xFF);
            break;
        }
        if ((op & 0xF80) == 0x100)
        {
            DEBUG("movs %s, %s\n",rrrrr[op&0x1F],AB[(op >> 5)&0x3]);
            break;
        }
        if ((op & 0xF80) == 0x180)
        {
            DEBUG("movs (r%d) (modifier=%s), %s\n", op&0x7,mm[(op>>3)&3], AB[(op >> 5) & 0x3]);
            break;
        }
        if ((op & 0xE00) == 0x600)
        {
            DEBUG("movp (r%d) (modifier=%s), (r%d) (modifier=%s)\n", op & 0x7, mm[(op >> 3) & 3],(op>>5)&0x3,mm[(op>>7)&0x3]);
            break;
        }
        if ((op & 0xFC0) == 0x040)
        {
            DEBUG("movp a%d, %s\n", (op >> 5) & 0x1, rrrrr[op & 0x1F]);
            break;
        }
        if ((op & 0xFC0) == 0x040)
        {
            DEBUG("movp a%d, %s\n",(op >> 5) &0x1, rrrrr[op&0x1F]);
            break;
        }
        if((op&0xF00) == 0x800)
        {
            DEBUG("mpyi %02X\n", op & 0xFF);
            break;
        }
        if((op & 0xE00) == 0xE00) {
            // TODO: divs
            DEBUG("divs??\n");
            break;
        }
        if ((op & 0xF00) == 0x500)
        {
            DEBUG("mov %02x, sv\n",op&0xFF);
            break;
        }
        if ((op & 0xF00) == 0xD00) //00001101...rrrrr
        {
            DEBUG("rep %s\n", rrrrr[op & 0x1F]);
            break;
        }
        if ((op & 0xF00) == 0xC00)
        {
            DEBUG("rep %02x\n", op&0xFF);
            break;
        }
        DEBUG("? %04X\n", op);
        break;
    case 1:
        if (!(op & 0x800))
        {
            DEBUG("callr %s %02x\n", cccc[op & 0xF], (op >> 4) & 0x7F);
            break;
        }
        if ((op & 0xC00) == 0x800)
        {
            DEBUG("mov %s, (r%d) (modifier=%s)\n",rrrrr[(op >> 5)&0x1F], op & 0x7, mm[(op >> 3) & 3]);
            break;
        }
        if ((op & 0xC00) == 0xC00)
        {
            DEBUG("mov (r%d) (modifier=%s), %s\n", op & 0x7, mm[(op >> 3) & 3], rrrrr[(op >> 5) & 0x1F]);
            break;
        }
        DEBUG("? %04X\n", op);
        break;
    case 2:
        if (!(op & 0x100))
        {
            DEBUG("mov %s, #%02x\n", rNstar[(op >> 9) & 0x7],op&0xFF);
            break;
        }
    case 3:
        if((op&0xF100) ==0x3000)
        {
#ifdef DISASM
            DEBUG("mov %s, #%02x\n",ABL[(op >>9)&0x7],op&0xFF);
#endif
#ifdef EMULATE
            DSPwrite16_8(op & 0xFF, getABL((op >> 9) & 0x7));
#endif
            break;
        }
        if ((op & 0xF00) == 0x100)
        {
            DEBUG("mov #%02x, a%dl\n", (op>>12)&0x1, op & 0xFF);
            break;
        }
        if ((op & 0xF00) == 0x500)
        {
            DEBUG("mov #%02x, a%dh\n", (op >> 12) & 0x1, op & 0xFF);
            break;
        }
        if ((op & 0x300) == 0x300)
        {
            DEBUG("mov #%02x, %s\n", op & 0xFF,rNstar[(op>>10)&0x7]);
            break;
        }
        if ((op & 0xB00) == 0x900)
        {
            DEBUG("mov #%02x, ext%d\n", op & 0xFF, ((op>>11)&0x2) | ((op >>10) &0x1));
            break;
        }

        DEBUG("? %04X\n", op);
        break;
    case 4:

        if(!(op & 0x80)) {
            int op3 = HasOp3(op);

            if(op3 != -1) {
                // ALU (rb + #offset7), ax
                DEBUG("%s (rb + %02x), a%d\n", ops3[op3], op & 0x7F, ax);
                break;
            }
        }
        if (op == 0x43C0)
        {
#ifdef DISASM
            DEBUG("dint\n");
#endif
#ifdef EMULATE
            st[0] &= ~0x2;
#endif
            break;
        }
        if (op == 0x4380)
        {
#ifdef DISASM
            DEBUG("eint\n");
#endif
#ifdef EMULATE
            st[0] |= 0x2;
#endif
            break;
        }
        if ((op & ~0x3) == 0x4D80)
        {
            DEBUG("load ps %d\n", op&0x3);
            break;
        }
        if ((op & 0xFE0) == 0x5C0)
        {
            DEBUG("reti %s (switch=%d)\n", cccc[op & 0xF],(op>>4)&1);
            break;
        }
        if ((op & 0xFF0) == 0x580)
        {
#ifdef DISASM
            DEBUG("ret %s \n", cccc[op&0xF]);
#endif
#ifdef EMULATE
            if (cccccheck(op & 0xF))
            {
                pc = DSPread16_16(sp) - 1;//pc++; at the end
                sp++;
            }
#endif
            break;
        }
        if ((op & ~0xF00F) == 0x180)
        {
            u16 extra = FetchWord(pc + 1);
#ifdef DISASM
            DEBUG("br %s %04x\n", cccc[op&0xF],extra);
#endif
            pc++;
#ifdef EMULATE
            if (cccccheck(op&0xF))pc = extra - 1;
#endif
            break;
        }
        if ((op&~0x7F) == 0x4B80)
        {
            DEBUG("banke #%02x\n", op & 0x7F);
            break;
        }
        if ((op&~0x3F) == 0x4980)
        {
            DEBUG("swap %s\n", swap[op&0xF]);
            break;
        }
        if ((op & 0xC0) == 0x40) //0100111111-rrrrr
        {
            DEBUG("movsi %s, %s (#%02x)\n", rNstar[(op >> 9)&0x7], AB[(op >>5)&0x3],op&0x1F);
            break;
        }
        if ((op & 0xFC0) == 0xFC0) //0100111111-rrrrr
        {
            DEBUG("mov %s, icr\n", rrrrr[op&0x1F]);
            break;
        }
        if ((op & 0xFC0) == 0xF80) //0100111111-vvvvv
        {
            DEBUG("mov %d, icr\n", op & 0x1F);
            break;
        }
        if((op & 0xFFFC) == 0x421C) {
            // lim
            switch(op & 3) {
            case 0:
                DEBUG("lim a0\n");
                break;
            case 1:
                DEBUG("lim a0, a1\n");
                break;
            case 2:
                DEBUG("lim a1, a0\n");
                break;
            case 3:
                DEBUG("lim a1\n");
                break;
            }

            break;
        }
        if((op & 0xFE0) == 0x7C0)
        {
            DEBUG("mov mixp , %s\n", rrrrr[op & 0x1F]);
            break;
        }
        if ((op & 0xFE0) == 0x7E0)
        {
            DEBUG("mov sp, %s\n", rrrrr[op & 0x1F]);
            break;
        }
        if ((op&0xFF0) == 0x1C0)
        {
            u16 extra = FetchWord(pc + 1);
            pc++;
#ifdef DISASM
            DEBUG("call %s %04x\n", cccc[op & 0xF], extra);
#endif
#ifdef EMULATE
            if (cccccheck(op & 0xF))
            {
                sp--;
                writeWord(sp, pc + 1);
                pc = extra - 1;
            }
#endif
            break;
        }
        if ((op&~0x7) == 0x4388) //this is strange
        {
            u16 extra = FetchWord(pc + 1);
            pc++;
#ifdef DISASM
            DEBUG("%s #%04x, %s\n", alb_ops[1], extra, morpone[op & 0x7]);
#endif
#ifdef EMULATE
            setmorpone(op & 0x7, doalb_ops(1, getmorpone(op & 0x7), extra));
#endif
            break;
        }
        if ((op&~0x7) == 0x43C8) //this is strange
        {
            u16 extra = FetchWord(pc + 1);
            pc++;
#ifdef DISASM
            DEBUG("%s #%04x, %s\n", alb_ops[0], extra, morpone[op & 0x7]);
#endif
#ifdef EMULATE
            setmorpone(op & 0x7, doalb_ops(0, getmorpone(op & 0x7), extra));
#endif
            break;
        }
        DEBUG("? %04X\n", op);
        break;
    case 0x5:
        if (!(op & 0x800))
        {
#ifdef DISASM
            DEBUG("brr %s %02x\n", cccc[op & 0xF], (op >> 4)&0x7F);
#endif
#ifdef EMULATE
            u32 brroffset = (op >> 4) & 0x7F;
            if (brroffset & 0x40)
                brroffset += 0xFF80;
            if (cccccheck(op & 0xF))pc += brroffset; //pc++; is at the end
#endif
            break;
        }
        if ((op & 0xF00) == 0xC00)
        {
            DEBUG("bkrep %02x\n", op & 0xFF);
            break;
        }
        if ((op & 0xf80) == 0xD00)
        {
            u16 extra = FetchWord(pc+1);
            DEBUG("bkrep %s %d %04x\n", rrrrr[op & 0x1F], (op >> 5) & 0x3, extra);
            break;
        }
        if ((op &0xF00) == 0xF00)
        {
            DEBUG("movd r%d (modifier=%s),r%d (modifier=%s)\n",3 + (op >> 2) & 0x1, mm[(op >> 3) & 0x3] ,op & 0x3, mm[(op >> 5) & 0x3]);
            break;
        }
        if ((op &~0x1F) == 0x5E60)
        {
#ifdef DISASM
            DEBUG("pop %s\n", rrrrr[op & 0x1F]);
#endif
#ifdef EMULATE
            setrrrrr(op & 0x1F, DSPread16_16(sp));
            sp++;
#endif
            break;
        }
        if (op == 0x5F40)
        {
            u16 extra = FetchWord(pc + 1);
            DEBUG("push #%04x\n", extra);
            pc++;
            break;
        }
        if ((op & 0xFE0) == 0xE40)
        {
#ifdef DISASM
            DEBUG("push %s\n", rrrrr[op&0x1F]);
#endif
#ifdef EMULATE
            sp--;
            DSPwrite16_16(sp, getrrrrr(op & 0x1F));
#endif
            break;
        }
        if ((op & 0xEE0) == 0xE00) //0101111-000rrrrr
        {
            u16 extra = FetchWord(pc + 1);
#ifdef DISASM
            DEBUG("mov #%04x, %s\n", extra, rrrrr[op&0x1F]);
#endif
#ifdef EMULATE
            setrrrrr(op & 0x1F, extra);
#endif
            pc++;
            break;
        }
        if ((op & 0xEE0) == 0xEE0) //0101111b001-----
        {
            u16 extra = FetchWord(pc + 1);
            DEBUG("mov #%04x, b%d\n", extra, ax);
            pc++;
            break;
        }
        if ((op & 0xC00) == 0x800)
        {
#ifdef DISASM
            DEBUG("mov %s, %s\n",rrrrr[op & 0x1F] ,rrrrr[(op >> 5) & 0x1F] );
#endif
#ifdef EMULATE
            u16 restt = getrrrrr(op & 0x1F);


            u16 temp = st[0] & 0xF1BF;
            if (restt == 0)temp |= 0x800; //Z
            //M
            //N
            //E
            st[0] = temp;

            setrrrrr((op >> 5) & 0x1F, restt);
#endif
            break;
        }
        if ((op & 0xFFC0) == 0x5EC0)
        {
            DEBUG("mov %s, b%d\n", rrrrr[op& 0x1F],(op>>5)&0x1);
            break;
        }
        if ((op & 0xFFE0) == 0x5F40)
        {
            DEBUG("mov %s, mixp\n", rrrrr[op & 0x1F]);
            break;
        }
        DEBUG("? %04X\n", op);
        break;
    case 0x6:
    case 0x7:
        if ((op & 0xF00) == 0x700)
        {
#ifdef DISASM
            DEBUG("%s %s a%d\n", ffff[(op>> 4)&0xF],cccc[op&0xF],(op>>12)&0x1);
#endif
#ifdef EMULATE
            if (cccccheck(op & 0xF))
            {
                u8 MSB = (st[(op >> 12) & 0x1]) & 0xF;
                a[(op >> 12) & 0x1] = doffff(a[(op >> 12) & 0x1], (op >> 4) & 0xF,&MSB);
                st[(op >> 12) & 0x1] = st[(op >> 12) & 0x1] & 0xFFF | (MSB << 12);
            }
#endif
            break;
        }
        if ((op&0x700) == 0x300)
        {
            DEBUG("movs %02x, %s\n",op&0xFF,AB[(op>> 11)&0x3]);
            break;
        }
        if ((op & 0xEF80) == 0x6F00)
        {
            DEBUG("%s b%d Bit %d\n",fff[(op >> 4)&0x7],(op >> 12)&0x1,op&0xF);
            break;
        }
        if ((op & 0xE300) == 0x6000)
        {
            DEBUG("mov #%02x ,%s\n",op&0xFF ,rNstar[(op >> 10)&0x7]);
            break;
        }
        if ((op & 0xE700) == 0x6100)
        {
            DEBUG("mov #%02x ,%s\n", op & 0xFF, AB[(op >> 11) & 0x3]);
            break;
        }
        if ((op & 0xE300) == 0x6200)
        {
            DEBUG("mov #%02x ,%s\n", op & 0xFF, ABL[(op >> 10) & 0x7]);
            break;
        }
        if ((op & 0xEF00) == 0x6500)
        {
            DEBUG("mov #%02x ,a%dHeu\n", op & 0xFF, (op >> 12)&0x1);
            break;
        }
        if ((op & 0xFF00) == 0x6D00)
        {
            DEBUG("mov #%02x ,sv\n", op & 0xFF);
            break;
        }
        if ((op & 0xFF00) == 0x7D00)
        {
            DEBUG("mov sv ,#%02x\n", op & 0xFF);
            break;
        }
        DEBUG("? %04X\n", op);
        break;
    case 0x8:
        if ((op & 0xE0) == 0xA0) {
            //MUL y, (rN)
            DEBUG("%s y, (a%d),(r%d) (modifier=%s)\n", mulXXX[(op >> 8) & 0x7], (op >> 11) & 0x1, op&0x7, mm[(op >> 3) & 3]);
            break;
        }
        if ((op & 0xE0) == 0x80) {
            //MUL y, register
            DEBUG("%s y, (a%d),%s\n", mulXXX[(op >> 8) & 0x7], (op >> 11) & 0x1, rrrrr[op & 0x1F]);
            break;
        }
        if ((op & 0xE0) == 0x00) {
            //MUL (rN), ##long immediate
            u16 longim = FetchWord(pc + 1);
            DEBUG("%s %s, (a%d),%04x\n", mulXXX[(op >> 8) & 0x7], rrrrr[op & 0x1F], (op >> 11) & 0x1, longim);
            pc++;
            break;
        }
        if ((op&~0x8) == 0x8971)
        {
            u16 extra = FetchWord(pc + 1);
            DEBUG("mov %04x, %s\n", extra, restep[(op >> 3)&0x1]);
            pc++;
            break;
        }
    case 0x9:
        if ((op & 0xE0) == 0xA0)
        {
            int op3 = HasOp3(op);

            if (op3 != -1) {
#ifdef DISASM
                DEBUG("%s a%dl, %s\n", ops3[op3], ax, rrrrr[op & 0x1F]);
#endif
#ifdef EMULATE
                u8 MSB = 0;
                a[ax] = (doops3(op3, a[ax] & 0xFFFF, getrrrrr(op & 0x1F), &MSB) & 0xFFFF) | a[ax] & ~0xFFFF;
#endif
                break;
            }
        }
        if ((op & 0xF240) == 0x9240)
        {
            DEBUG("shfi %s, %s %02x\n", AB[(op >> 10) & 0x3], AB[(op >> 7) & 0x3], fixending(op & 0x3F,6));
            break;
        }

        if ((op & 0xFEE0) == 0x9CC0)
        {
            DEBUG("movr %s ,a%d\n", rrrrr[op&0x1F], ax);
            break;
        }
        if ((op & 0xFEE0) == 0x9CC0)
        {
            DEBUG("mov (r%d) (modifier=%s) ,a%d\n", op & 0x7, mm[(op >> 3) & 3], ax);
            break;
        }
        if ((op & 0xFEE0) == 0x98C0)
        {
            DEBUG("mov (r%d) (modifier=%s) ,b%d\n", op & 0x7, mm[(op >> 3) & 3], ax);
            break;
        }
        if ((op & 0xFEE0) == 0x9840)
        {
            DEBUG("exp r%d (modifier=%s), a%d\n", op & 0x7, mm[(op >> 3) & 3], ax);
            break;
        }
        if ((op & 0xFEE0) == 0x9040)
        {
            DEBUG("exp %s, a%d\n", rrrrr[op & 0x1F], ax);
            break;
        }
        if ((op & 0xFEFE) == 0x9060)
        {
            DEBUG("exp b%d, a%d\n", op & 0x1, ax);
            break;
        }
        if ((op & 0xFEFE) == 0x9C40)
        {
            DEBUG("exp r%d (modifier=%s), sv\n", op & 0x7, mm[(op >> 3) & 3]);
            break;
        }
        if ((op & 0xFEFE) == 0x9440)
        {
            DEBUG("exp %s, sv\n", rrrrr[op & 0x1F]);
            break;
        }
        if ((op & 0xFFFE) == 0x9460)
        {
            DEBUG("exp b%d, sv\n", op & 0x1);
            break;
        }

        if((op & 0xFEE0) == 0x90C0)
        {
            u16 extra = FetchWord(pc + 1);
            DEBUG("msu (a%d) (r%d), %04x (modifier=%s)\n",ax, op & 0x7,extra, mm[(op >> 3) & 3]);
            pc++;
            break;
        }
        if((op&0xF0E0) == 0x9020)
        {
            DEBUG("tstb (r%d) (modifier=%s) (bit=%d)\n", op & 0x7, mm[(op >> 3) & 3],(op >> 8)&0xF);
            break;
        }
        if ((op & 0xF0E0) == 0x9000)
        {
            DEBUG("tstb %s (bit=%d)\n", rrrrr[op&0x1F], (op >> 8) & 0xF);
            break;
        }
        if ((op & 0xE0) == 0x80) {
            // ALM (rN)
            DEBUG("%s (r%d), a%d (modifier=%s)\n", ops[(op >> 9) & 0xF], op & 0x7, ax, mm[(op >> 3) & 3]);
            break;
        }
        if ((op & 0xE0) == 0xA0) {
            // ALM register
            u16 r = op & 0x1F;

            if(r < 22) {
                DEBUG("%s %s, a%d\n", ops[(op >> 9) & 0xF], rrrrr[r], ax);
                break;
            }

            DEBUG("? %04X\n", op);
        } else if(((op >> 6) & 0x7) == 7) {
            u16 extra = FetchWord(pc+1);
            pc++;

            if(!(op & 0x100)) {
                // ALB (rN)
                DEBUG("%s (r%d), %04x (modifier=%s)\n", alb_ops[(op >> 9) & 0x7], op & 0x7,
                      extra & 0xFFFF, mm[(op >> 3) & 3]);
                break;
            } else {
                // ALB register
                u16 r = op & 0x1F;

                if(r < 22) {
#ifdef DISASM
                    DEBUG("%s %s, %04x\n", alb_ops[(op >> 9) & 0x7], rrrrr[r], extra & 0xFFFF);
#endif
#ifdef EMULATE
                    setrrrrr(op & 0x1F,doalb_ops((op >> 9) & 0x7, getrrrrr(op & 0x1F), extra));
#endif
                    break;
                }

                DEBUG("? %04X\n", op);
            }
            break;
        } else if((op & 0xF0FF) == 0x80C0) {
            int op3 = HasOp3(op);

            if (op3 != -1) {
                // ALU ##long immediate
                u16 extra = FetchWord(pc + 1);
                pc++;
#ifdef DISASM
                DEBUG("%s %04x, a%d\n", ops3[op3], extra & 0xFFFF, ax);
#endif
#ifdef EMULATE
                u8 MSB = (st[(op >> 12) & 0x1] >> 12) & 0xF;
                a[ax] = doops3(op3,a[ax],extra,&MSB);
                st[(op >> 12) & 0x1] = (st[(op >> 12) & 0x1] & 0xFFF) | MSB << 12;
#endif

                break;
            }

            DEBUG("? %04X\n", op);
            break;
        } else if((op & 0xFF70) == 0x8A60) {
            // TODO: norm
            u16 extra = FetchWord(pc+1);
            pc++;

            DEBUG("norm??\n");
            break;
        } else if((op & 0xF8E7) == 0x8060) {
            // maxd
            int d = op & (1 << 10);
            int f = op & (1 << 9);
            DEBUG("max%s %s, a%d\n", d ? "d" : "", f ? "gt" : "ge", ax);
            break;
        }

        DEBUG("? %04X\n", op);
        break;

    case 0xA:
    case 0xB:
        // ALM direct
#ifdef DISASM
        DEBUG("%s %02x, a%d\n", ops[(op >> 9) & 0xF], op & 0xFF, ax);
#endif
#ifdef EMULATE
        {
            u8 MSB = (st[ax] >> 12);
            doops((op >> 9) & 0xF, a[ax], &MSB, DSPread16_8(op & 0xFF), 0);
            st[ax] = (MSB << 12) | st[ax]&0xFFF;
        }
#endif
        break;

    case 0xC: {
        int op3 = HasOp3(op);

        if(op3 != -1) {
            // ALU #short immediate
            DEBUG("%s %02x, a%d\n", ops3[op3], op & 0xFF, ax);
            break;
        }
        DEBUG("? %04X\n", op);
        break;
    }


    case 0xD:
        if (op == 0xD390)
        {
            DEBUG("cntx r\n");
            break;
        }
        if (op == 0xD380)
        {
            DEBUG("cntx s\n");
            break;
        }
        if (op == 0xD7C0)
        {
            DEBUG("retid\n");
            break;
        }
        if (op == 0xD780)
        {
            DEBUG("retd\n");
            break;
        }
        if ((op & ~0xF010) == 0x381)
        {
#ifdef DISASM
            DEBUG("call a%d\n",(op>>4)&0x1);
#endif
#ifdef EMULATE
            sp--;
            writeWord(sp, pc + 1);
            pc = a[(op >> 4) & 0x1] - 1;//pc++;
#endif
            break;
        }
        if (op == 0xD3C0)
        {
            DEBUG("break\n");
            break;
        }
        if ((op & 0xF80) == 0xF80)
        {
            DEBUG("load stepj #%02x\n", op&0x7F);
            break;
        }
        if ((op & 0xF80) == 0xB80)
        {
            DEBUG("load stepi #%02x\n", op & 0x7F);
            break;
        }
        if ((op & 0xFEFC) == 0xD498)//1101010A100110--
        {
            u16 extra = FetchWord(pc + 1);
            pc++;
            DEBUG("mov (rb + #%04x), a%d\n", extra, ax);
            break;
        }
        if ((op & 0xFE80) == 0xDC80)//1101110a1ooooooo
        {
            DEBUG("mov (rb + #%02x), a%d\n", op&0x7F,ax);
            break;
        }
        if ((op & 0xFE80) == 0xD880)//1101100a1ooooooo
        {
            DEBUG("move a%dl, (rb + #%02x)\n", op & 0x7F, ax);
            break;
        }
        if ((op & 0xF39F) == 0xD290)//1101ab101AB10000
        {
            DEBUG("mov %s, %s\n", AB[(op >> 5) & 0x3], AB[(op >> 10) & 0x3]);
            break;
        }
        if ((op & 0xFF9F) == 0xD490)
        {
            DEBUG("mov repc, %s\n", AB[(op >> 5) & 0x3]);
            break;
        }
        if ((op & 0xFF9F) == 0xD491)
        {
            DEBUG("mov dvm, %s\n", AB[(op >> 5) & 0x3]);
            break;
        }
        if ((op & 0xFF9F) == 0xD492)
        {
            DEBUG("mov icr, %s\n", AB[(op >> 5) & 0x3]);
            break;
        }
        if ((op & 0xFF9F) == 0xD493)
        {
            DEBUG("mov x, %s\n", AB[(op >> 5) & 0x3]);
            break;
        }
        if ((op & 0xE80) == 0x080) {
            //msu (rJ), (rI) 1101000A1jjiiwqq
            DEBUG("msu r%d (modifier=%s),r%d (modifier=%s) a%d\n", op & 0x3, mm[(op >> 5) & 0x3], 3 + (op >> 2) & 0x1, mm[(op >> 3) & 0x3], ax);
            break;
        }
        if ((op & 0xFEFC) == 0xD4B8) //1101010a101110--
        {
            u16 extra = FetchWord(pc + 1);
            DEBUG("mov [%04x], a%d\n",extra,ax);
            pc++;
            break;
        }
        if ((op & 0xFEFC) == 0xD4BC) //1101010a101111--
        {
            u16 extra = FetchWord(pc + 1);
            DEBUG("mov a%dl, [%04x]\n", ax,extra);
            pc++;
            break;
        }

        if ((op & 0xF3FF) == 0xD2D8)
        {
            DEBUG("mov %s,x\n",AB[(op>>10) & 0x3]);
            break;
        }
        if ((op & 0xF3FF) == 0xD298)
        {
            DEBUG("mov %s,dvm\n", AB[(op >> 10) & 0x3]);
            break;
        }
        if (!(op & 0x80)) {
            //MUL (rJ), (rI) 1101AXXX0jjiiwqq
            DEBUG("%s r%d (modifier=%s),r%d (modifier=%s) a%d\n", mulXXX[(op >> 8) & 0x7], op & 0x3, mm[(op >> 5) & 0x3], 3 + (op >> 2) & 0x1, mm[(op >> 3) & 0x3], (op >> 11) & 0x1);
            break;
        }
        if ((op & 0xFE80) == 0xD080){
            //msu (rJ), (rI)
            DEBUG("msu r%d (modifier=%s),r%d (modifier=%s) a%d\n", op & 0x3, mm[(op >> 5) & 0x3], 3 + (op >> 2) & 0x1, mm[(op >> 3) & 0x3], ax);
            break;
        }
        if ((op & 0xF390) == 0xD280){
            //shfc
            DEBUG("shfc %s %s %s\n", AB[(op >> 10) & 0x3], AB[(op >> 5) & 0x3], cccc[op&0xF]);
            break;
        }
        if((op & 0xFED8) == 0xD4D8) {
            int op3 = HasOp3(op);

            if(op3 != -1) {
                u16 extra = FetchWord(pc+1);
                pc+=2;

                if(op & (1 << 5)) {  // ALU [##direct add.],ax
                    DEBUG("%s [%04x], a%d\n",
                          ops3[op3],
                          extra & 0xFFFF,
                          op & (0x1000) ? 1 : 0);
                    break;
                } else { // ALU (rb + ##offset),ax
                    DEBUG("%s (rb + %04x), a%d\n",
                          ops3[op3],
                          extra & 0xFFFF,
                          op & (0x1000) ? 1 : 0);
                    break;
                }
            }
        }
        DEBUG("? %04X\n", op);
        break;

    case 0xE:
        // ALB direct
        if(op & (1 << 8)) {
            u16 extra = FetchWord(pc+2);

            DEBUG("%s %02x, %04x\n", alb_ops[(op >> 9) & 0x7], op & 0xFF, extra & 0xFFFF);
            pc++;
            break;
        }
        else
        {
            DEBUG("%s a%d #%02x\n", mulXX[(op >> 9) & 0x3], (op >> 11) & 0x1,op&0xFF);
            break;
        }
        DEBUG("? %04X\n", op);
        break;
    case 0xF:
        DEBUG("tstb #%02x (bit=%d)\n", op&0xFF, (op >> 8) & 0xF);
        break;
    default:
        DEBUG("? %04X\n",op);
    }

    pc++;
}

void DSP_Run()
{
    pc = 0x0; //reset
    while (1)
    {
        //DEBUG("op:%04x (%04x) %04x\n", FetchWord(pc),pc,sp);
        DSP_Step();
    }
}

void DSP_LoadFirm(u8* bin)
{
    dsp_header head;
    memcpy(&head, bin, sizeof(head));

    u32 magic = Read32(head.magic);
    u32 contsize = Read32(head.content_sz);
    u32 unk1 = Read32(head.unk1);
    u32 unk6 = Read32(head.unk6);
    u32 unk7 = Read32(head.unk7);

    DEBUG("head %08X %08X %08X %08X %08X %02X %02X %02X %02X\n",
          magic, contsize, unk1, unk6, unk7, head.unk2, head.unk3, head.num_sec, head.unk5);

    for (int i = 0; i < head.num_sec; i++) {
        u32 dataoffset = Read32(head.segment[i].data_offset);
        u32 destoffset = Read32(head.segment[i].dest_offset);
        u32 size = Read32(head.segment[i].size);
        u32 select = Read32(head.segment[i].select);

        DEBUG("segment %08X %08X %08X %08X\n", dataoffset, destoffset, size, select);
        memcpy(ram + (destoffset *2), bin + dataoffset, size);
    }

    //DSP_Run();
}
