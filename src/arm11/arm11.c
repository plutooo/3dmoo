/*
 * Copyright (C) 2014 - ichfly
 * Copyright (C) 2014 - plutoo
 * Copyright (C) 2011 - Miguel Boton (Waninkoko)
 * Copyright (C) 2010 - crediar, megazig
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
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "../util.h"

static void arm11_Disasm32(u32 a);

#define LSL(x,y)(x << y)
#define LSR(x,y)(x >> y)
#define ASR(x,y)((x & (1 << 31)) ? ((x >> y) | ((~0 >> (32 - y)) << (32 - y))) : (x >> y))
#define ROR(x,y)((x >> y) | (x << (32 - y)))

static u32 r[16];
static u32 *pc, *sp, *lr;

enum {
    EQ = 0,
    NE = 1,
    CS = 2,
    CC = 3,
    MI = 4,
    PL = 5,
    VS = 6,
    VC = 7,
    HI = 8,
    LS = 9,
    GE = 10,
    LT = 11,
    GT = 12,
    LE = 13,
    AL = 14,
};

union {
    struct {
	bool n:1;
	bool z:1;
	bool c:1;
	bool v:1;

	unsigned pad:20;

	bool I:1;
	bool F:1;
	bool t:1;
	u16  mode:5;
    };
    u32 value;
} cpsr;
u32 spsr;


void arm11_Init()
{
    sp = (u32 *)(r + 13);
    lr = (u32 *)(r + 14);
    pc = (u32 *)(r + 15);

    memset(r, 0, sizeof(r));
    cpsr.value = spsr = 0;

}

void arm11_SetPCSP(u32 ipc, u32 isp) {
    *pc = ipc;
    *sp = isp;
}

u32 arm11_R(u32 n) {
    if(n > 15) {
	ERROR("%s: invalid r%n.\n", __func__, n);
	return 0;
    }

    return r[n];
}

void arm11_SetR(u32 n, u32 val) {
    if(n > 15) {
	ERROR("%s: invalid r%n.\n", __func__, n);
	return;
    }
    r[n] = val;
}

static bool CondCheck32(u32 opcode)
{
    switch(opcode >> 28) {
    case EQ:
	return cpsr.z;
    case NE:
	return !cpsr.z;
    case CS:
	return cpsr.c;
    case CC:
	return !cpsr.c;
    case MI:
	return cpsr.n;
    case PL:
	return !cpsr.n;
    case VS:
	return cpsr.v;
    case VC:
	return !cpsr.v;
    case HI:
	return (cpsr.c && !cpsr.z);
    case LS:
	return (!cpsr.c || cpsr.z);
    case GE:
	return (cpsr.n == cpsr.v);
    case LT:
	return (cpsr.n != cpsr.v);
    case GT:
	return (cpsr.n == cpsr.v && !cpsr.z);
    case LE:
	return (cpsr.n != cpsr.v || cpsr.z);
    case AL:
	return true;
    }

    return false;
}

static bool CondCheck16(u16 opcode)
{
    switch((opcode >> 8) & 0xF) {
    case EQ:
	return cpsr.z;
    case NE:
	return !cpsr.z;
    case CS:
	return cpsr.c;
    case CC:
	return !cpsr.c;
    case MI:
	return cpsr.n;
    case PL:
	return !cpsr.n;
    case VS:
	return cpsr.v;
    case VC:
	return !cpsr.v;
    case HI:
	return (cpsr.c && !cpsr.z);
    case LS:
	return (!cpsr.c || cpsr.z);
    case GE:
	return (cpsr.n == cpsr.v);
    case LT:
	return (cpsr.n != cpsr.v);
    case GT:
	return (cpsr.n == cpsr.v && !cpsr.z);
    case LE:
	return (cpsr.n != cpsr.v || cpsr.z);
    case AL:
	return true;
    }

    return false;
}

static void CondPrint32(u32 opcode)
{
    switch (opcode >> 28) {
    case EQ:
	printf("eq"); break;
    case NE:
	printf("ne"); break;
    case CS:
	printf("cs"); break;
    case CC:
	printf("cc"); break;
    case MI:
	printf("mi"); break;
    case PL:
	printf("pl"); break;
    case VS:
	printf("vs"); break;
    case VC:
	printf("vc"); break;
    case HI:
	printf("hi"); break;
    case LS:
	printf("ls"); break;
    case GE:
	printf("ge"); break;
    case LT:
	printf("lt"); break;
    case GT:
	printf("gt"); break;
    case LE:
	printf("le"); break;
    }
}

static void CondPrint16(u16 opcode)
{
    switch ((opcode >> 8) & 0xF) {
    case EQ:
	printf("eq"); break;
    case NE:
	printf("ne"); break;
    case CS:
	printf("cs"); break;
    case CC:
	printf("cc"); break;
    case MI:
	printf("mi"); break;
    case PL:
	printf("pl"); break;
    case VS:
	printf("vs"); break;
    case VC:
	printf("vc"); break;
    case HI:
	printf("hi"); break;
    case LS:
	printf("ls"); break;
    case GE:
	printf("ge"); break;
    case LT:
	printf("lt"); break;
    case GT:
	printf("gt"); break;
    case LE:
	printf("le"); break;
    }
}

static void SuffixPrint(u32 opcode)
{
    if ((opcode >> 20) & 1)
	printf("s");
}

static void ShiftPrint(u32 opcode)
{
    u32 amt = (opcode >> 7)  & 0x1F;

    if (!amt)
	return;

    switch ((opcode >> 5) & 3) {
    case 0:
	printf(",LSL#%d", amt);
	break;
    case 1:
	printf(",LSR#%d", amt);
	break;
    case 2:
	printf(",ASR#%d", amt);
	break;
    case 3:
	printf(",ROR#%d", amt);
	break;
    }
}

static bool CarryFrom(u32 a, u32 b)
{
    return ((a + b) < a) ? true : false;
}

static bool BorrowFrom(u32 a, u32 b)
{
    return (a < b) ? true : false;
}

static bool OverflowFrom(u32 a, u32 b)
{
    u32 s = a + b;

    if((a & (1 << 31)) == (b & (1 << 31)) &&
       (s & (1 << 31)) != (a & (1 << 31)))
	return true;

    return false;
}

static u32 Addition(u32 a, u32 b)
{
    u32 result;

    result = a + b;

    cpsr.c = CarryFrom(a, b);
    cpsr.v = OverflowFrom(a, b);
    cpsr.z = result == 0;
    cpsr.n = result >> 31;

    return result;
}

static u32 Substract(u32 a, u32 b)
{
    u32 result;

    result = a - b;

    cpsr.c = !BorrowFrom(a, b);
    cpsr.v = OverflowFrom(a, -b);
    cpsr.z = result == 0;
    cpsr.n = result >> 31;

    return result;
}

static u32 Shift(u32 opcode, u32 value)
{
    bool S = (opcode >> 20) & 1;

    u32 amt = (opcode >> 7)  & 0x1F;
    u32 result;

    if (!amt)
	return value;

    switch ((opcode >> 5) & 3) {
    case 0:
	if (S) cpsr.c = value & (1 << (32 - amt));

	result = LSL(value, amt);
	break;
    case 1:
	if (S) cpsr.c = value & (1 << (amt - 1));

	result = LSR(value, amt);
	break;
    case 2:
	if (S) cpsr.c = value & (1 << (amt - 1));

	result = ASR(value, amt);
	break;
    case 3:
	result = ROR(value, amt);
	break;

    default:
	result = value;
    }

    return result;
}

static void Push(u32 value)
{
    *sp -= sizeof(u32);
    mem_Write32(*sp, value);
}

static u32 Pop()
{
    u32 addr = *sp;
    *sp += sizeof(u32);

    return mem_Read32(addr);
}

static void Step32()
{
    u32 opcode;

    u32 temp;
    u8 b1;
    u8 b2;
    u8 b3;
    u8 b4;
	
    arm11_Disasm32(*pc);
    opcode = mem_Read32(*pc);

    printf("[%08x] ", *pc);
    *pc += sizeof(opcode);

    u32 Rn    = ((opcode >> 16) & 0xF);
    u32 Rd    = ((opcode >> 12) & 0xF);
    u32 Rm    = ((opcode >> 0) & 0xF);
    u32 Rs    = ((opcode >> 8) & 0xF);
    u32 Imm   = ((opcode >> 0) & 0xFF);
    u32 amt   = Rs << 1;

    bool I = (opcode >> 25) & 1;
    bool P = (opcode >> 24) & 1;
    bool U = (opcode >> 23) & 1;
    bool B = (opcode >> 22) & 1;
    bool W = (opcode >> 21) & 1;
    bool S = (opcode >> 20) & 1;
    bool L = (opcode >> 20) & 1;

    if (((opcode >> 8) & 0xFFFFF) == 0x12FFF) {
	bool link = (opcode >> 5) & 1;

	if (!CondCheck32(opcode))
	    return;

	if (link) *lr = *pc;

	cpsr.t = r[Rm] & 1;
	*pc    = r[Rm] & ~1;

	return;
    }

    if ((opcode & 0x0FF00FF0) == 0x06600FF0)//UQSUB8
    {
        b1 = (u8)(r[Rm]) - (u8)(r[Rn]);
        b2 = (u8)(r[Rm] >> 8) - (u8)(r[Rn] >> 8);
        b3 = (u8)(r[Rm] >> 16) - (u8)(r[Rn] >> 16);
        b4 = (u8)(r[Rm] >> 24) - (u8)(r[Rn] >> 24);
        r[Rd] = (b1 | b2 << 8 | b3 << 16 | b4 << 24);
    }
    if ((opcode & 0x0FFF0FF0) == 0x06bf0070)//sxth
    {
	temp = r[Rm] &0xFFFF;
	if ((temp & 0x8000) != 0)
	{
	    temp |= 0xFFFF0000;
	}
	r[Rd] = temp;
    }
    if ((opcode & 0x0FFF0FF0) == 0x06af0070)//sxtb
    {
	temp = r[Rm] &0xFF;
	if ((temp & 0x80) != 0)
	{
	    temp |= 0xFFFFFF00;
	}
	r[Rd] = temp;
    }
    if ((opcode & 0x0FFF0FF0) == 0x06ef0070)//uxtb
    {
        r[Rd] = (r[Rm] & 0xFF);
    }
    if ((opcode & 0x0FFF0FF0) == 0x06ff0070)//uxth
    {
        r[Rd] = (r[Rm] & 0xFFFF);
    }

    if ((opcode & 0x0FFF0FF0) == 0x06bf0fb0) //rev16
    {
	u32 temp = r[Rm];
        r[Rd] = (((temp & 0xFF) >> 0x0) << 0x8) +
	    (((temp & 0xFF00) >> 0x8) << 0x0) +
	    (((temp & 0xFF0000) >> 0x10) << 0x18) +
	    (((temp & 0xFF000000) >> 0x18) << 0x10);
    }

    if ((opcode & 0x0FFF0FF0) == 0x06bf0f30) //rev
    {
	u32 temp = r[Rm];
        r[Rd] = (((temp & 0xFF) >> 0x0) << 0x18) +
	    (((temp & 0xFF00) >> 0x8) << 0x10) +
	    (((temp & 0xFF0000) >> 0x10) << 0x8) +
	    (((temp & 0xFF000000) >> 0x18) << 0x0);
    }
	
    if ((opcode >> 24) == 0xEF) { // SVC
	u32 Imm = opcode & 0xFFFFFF;
	svc_Execute(Imm & 0xFF);
	return;
    }

    if (((opcode >> 22) & 0x3F) == 0 && // MUL/MLA
	((opcode >>  4) & 0x0F) == 9) {

	if (!CondCheck32(opcode))
	    return;

	if (W)
	    r[Rn] = (r[Rm] * r[Rs] + r[Rd]) & 0xFFFFFFFF;
	else
	    r[Rn] = (r[Rm] * r[Rs]) & 0xFFFFFFFF;

	if (S) {
	    cpsr.z = r[Rn] == 0;
	    cpsr.n = r[Rn] >> 31;
	}

	return;
    }

    //LDREXB/STREXB/LDREXH/STREXH/LDREX/STREX (ARM11)
    if (((opcode >> 23) & 0x1F) == 3 && 
	((opcode >>  4) & 0xFF) == 0xF9) {
	
	if((opcode >> 20) & 1) {
	    if((opcode & 0xF) == 0xF) {
		switch((opcode >> 21) & 3) {
		case 0: //W
		    if (!CondCheck32(opcode))
			return;

		    r[Rd] = mem_Read32(r[Rn], r[Rm]);
		    return;
		case 1: //D
		    if (!CondCheck32(opcode))
			return;

		    if(Rn % 2) {
			ERROR("invalid insn, Rn must be even.\n");
			exit(1);
		    }

		    r[Rd]   = mem_Read32(r[Rn]);
		    r[Rd+1] = mem_Read32(r[Rn]+4);
		    return;
		case 2: //B
		    if (!CondCheck32(opcode))
			return;

		    r[Rd] = mem_Read8(r[Rn]);
		    return;
		case 3: //H
		    if (!CondCheck32(opcode))
			return;

		    r[Rd] = mem_Read16(r[Rn]);
		    return;
		}
	    }
	}
	else {
	    switch((opcode >> 21) & 3) {
	    case 0: //W
		if (!CondCheck32(opcode))
		    return;

		r[Rd] = 0;
		mem_Write32(r[Rn], r[Rm]);
		return;
	    case 1: //D
		if (!CondCheck32(opcode))
		    return;

		if(Rn % 2) {
		    ERROR("invalid insn, Rn must be even.\n");
		    exit(1);
		}

		r[Rd] = 0;
		mem_Write32(r[Rn], r[Rm]);
		mem_Write32(r[Rn]+4, r[Rm+1]);
		return;
	    case 2: //B
		if (!CondCheck32(opcode))
		    return;

		r[Rd] = 0;
		mem_Write8(r[Rn], r[Rm]);
		return;
	    case 3: //H
		if (!CondCheck32(opcode))
		    return;

		r[Rd] = 0;
		mem_Write16(r[Rn], r[Rm]);
		return;
	    }
	}
    }
    //CLREX (ARM11)
    if((opcode >> 20) == 0xF57 && (opcode >> 4) == 1) {
	return;
    }
    //NOP (ARM11)
    if(((opcode << 4) >> 12) == 0x320F0) {
	return;
    }


    switch ((opcode >> 26) & 0x3) {
    case 0: {
	switch ((opcode >> 21) & 0xF) {
	case 0: {// AND
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = r[Rn] & ROR(Imm, amt);
	    else
		r[Rd] = r[Rn] & Shift(opcode, r[Rm]);

	    if (S) {
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 1: {// EOR
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = r[Rn] ^ ROR(Imm, amt);
	    else
		r[Rd] = r[Rn] ^ Shift(opcode, r[Rm]);

	    if (S) {
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 2: {// SUB
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = r[Rn] - ROR(Imm, amt);
	    else
		r[Rd] = r[Rn] - Shift(opcode, r[Rm]);

	    if (S) {
		cpsr.c = (I) ? (r[Rn] >= ROR(Imm, amt)) : (r[Rn] < r[Rd]);
		cpsr.v = (I) ? ((r[Rn] >> 31) & ~(r[Rd] >> 31)) : ((r[Rn] >> 31) & ~(r[Rd] >> 31));
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 3: {// RSB
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = ROR(Imm, amt) - r[Rn];
	    else
		r[Rd] = Shift(opcode, r[Rm]) - r[Rn];

	    if (S) {
		cpsr.c = (I) ? (r[Rn] > Imm) : (r[Rn] > r[Rm]);
		cpsr.v = (I) ? ((Imm >> 31) & ~((Imm - r[Rn]) >> 31)) : ((Imm >> 31) & ~((r[Rm] - r[Rn]) >> 31));
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 4: {// ADD
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd]  = r[Rn] + ROR(Imm, amt);
	    else
		r[Rd] = r[Rn] + Shift(opcode, r[Rm]);

	    if (Rn == 15)
		r[Rd] += 4;

	    if (S) {
		cpsr.c = r[Rd] < r[Rn];
		cpsr.v = (r[Rn] >> 31) & ~(r[Rd] >> 31);
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 5: {// ADC
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = r[Rn] + ROR(Imm, amt) + cpsr.c;
	    else
		r[Rd] = r[Rn] + Shift(opcode, r[Rm]) + cpsr.c;

	    if (S) {
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 6: {// SBC
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = r[Rn] - ROR(Imm, amt) - !cpsr.c;
	    else
		r[Rd] = r[Rn] - Shift(opcode, r[Rm]) - !cpsr.c;

	    if (S) {
		cpsr.c = r[Rd] > r[Rn];
		cpsr.v = (r[Rn] >> 31) & ~(r[Rd] >> 31);
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 7: {// RSC
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = ROR(Imm, amt) - r[Rn] - !cpsr.c;
	    else
		r[Rd] = Shift(opcode, r[Rm]) - r[Rn] - !cpsr.c;

	    if (S) {
		cpsr.c = (I) ? (r[Rd] > Imm) : (r[Rd] > r[Rm]);
		cpsr.v = (I) ? ((r[Rm] >> 31) & ~(r[Rd] >> 31)) : ((r[Rn] >> 31) & ~(r[Rd] >> 31));
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 8: {// TST/MRS
	    if (S) {
		// XXX: Cond??!?!

		u32 result;

		if (!I) {
		    result = r[Rn] & Shift(opcode, r[Rm]);
		} else {
		    result = r[Rn] & ROR(Imm, amt);
		}

		cpsr.z = result == 0;
		cpsr.n = result >> 31;
	    } else {
		r[Rd] = cpsr.value;
	    }

	    return;
	}

	case 9: {// TEQ/MSR
	    if (S) {
		// XXX: Cond?!?!?

		u32 result;

		if (!I) {
		    result = r[Rn] ^ Shift(opcode, r[Rm]);
		} else {
		    result = r[Rn] ^ ROR(Imm, amt);
		}

		cpsr.z = result == 0;
		cpsr.n = result >> 31;
	    } else {
		if (I) {
		    cpsr.value = r[Rm];
		} else {
		    cpsr.value = Imm;
		}
	    }

	    return;
	}

	case 10: {// CMP/MRS2
	    if (S) {
		// XXX: Cond?!?!?

		u32 value;

		if (I) {
		    value = ROR(Imm, amt);
		} else {
		    value = r[Rm];
		}

		if (CondCheck32(opcode))
		    Substract(r[Rn], value);
	    } else {
		// XXX: MRS2 not implemented??
	    }

	    return;
	}

	case 11: {// CMN/MSR2
	    if (S) {
		u32 value;

		if (I) {
		    value = ROR(Imm, amt);
		} else {
		    value = r[Rm];
		}

		if (CondCheck32(opcode))
		    Addition(r[Rn], value);
	    } else {
		// XXX: MRS2 not implemented??
	    }

	    return;
	}

	case 12: {// ORR
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = r[Rn] | ROR(Imm, amt);
	    else 
		r[Rd] = r[Rn] | Shift(opcode, r[Rm]);

	    if (S) {
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 13: {// MOV
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = ROR(Imm, amt);
	    else
		r[Rd] = (Rm == 15) ? (*pc + sizeof(opcode)) : Shift(opcode, r[Rm]);

	    if (S) {
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 14: {// BIC
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = r[Rn] & ~(ROR(Imm, amt));
	    else
		r[Rd] = r[Rd] & ~Shift(opcode, r[Rm]);

	    if (S) {
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}

	case 15: {// MVN
	    if (!CondCheck32(opcode))
		return;

	    if (I)
		r[Rd] = ~ROR(Imm, amt);
	    else
		r[Rd] = ~Shift(opcode, r[Rm]);

	    if (S) {
		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    }

	    return;
	}
	}
    }

    case 1: {// LDR/STR
	u32  addr, value, wb;

	Imm = opcode & 0xFFF;

	if (L && Rn == 15) {
	    addr  = *pc + Imm + sizeof(opcode);
	    value = mem_Read32(addr);

	    if (CondCheck32(opcode))
		r[Rd] = value;

	    return;
	}

	if (I) {
	    value = Shift(opcode, r[Rm]);
	} else {
	    value = Imm;
	}

	if (!CondCheck32(opcode))
	    return;

	if (U) wb = r[Rn] + value;
	else   wb = r[Rn] - value;

	addr = (P) ? wb : r[Rn];

	if (L) {
	    if (B)
		r[Rd] = mem_Read8 (addr);
	    else
		r[Rd] = mem_Read32(addr);
	} else {
	    value = r[Rd];
	    if (Rd == 15)
		value += 8;

	    if (B)
		mem_Write8 (addr, value);
	    else
		mem_Write32(addr, value);
	}

	if (W || !P) r[Rn] = wb;
	return;
    }

    default:
	break;
    }

    switch ((opcode >> 25) & 7) {
    case 4: {// LDM/STM
	// XXX: Conditions?!?!?!
	u32  start = r[Rn];
	bool pf    = false;

	if (B) {
	    if (opcode & (1 << 15))
		cpsr.value = spsr;
	}

	if (L) {
	    s32 i;
	    for (i = 0; i < 16; i++) {
		if ((opcode >> i) & 1) {
		    if (P)  start += (U) ? sizeof(u32) : -sizeof(u32);
		    r[i] = mem_Read32(start);
		    if (!P) start += (U) ? sizeof(u32) : -sizeof(u32);
		}
	    }
	} else {
	    s32 i;
	    for (i = 15; i >= 0; i--) {
		if ((opcode >> i) & 1) {
		    if (P)  start += (U) ? sizeof(u32) : -sizeof(u32);
		    mem_Write32(start, r[i]);
		    if (!P) start += (U) ? sizeof(u32) : -sizeof(u32);
		}
	    }
	}

	if (W) r[Rn] = start;
	return;
    }

    case 5: {// B/BL
	bool link = opcode & (1 << 24);

	Imm = (opcode & 0xFFFFFF) << 2;
	if (Imm & (1 << 25)) Imm = ~(~Imm & 0xFFFFFF);
	Imm += sizeof(opcode);

	if (!CondCheck32(opcode))
	    return;

	if (link) *lr = *pc;
	*pc += Imm;

	return;
    }

    case 7: {// MRC/MCR
	if(((opcode >> 4) & 1) && !((opcode >> 24) & 1)) {
	    bool L = (opcode >> 20) & 1;
	    u32  CRm = opcode & 0xF;
	    u32  CP = (opcode >> 5) & 0x7;
	    u32  CP_num = (opcode >> 8) & 0xF;
	    u32  Rd = (opcode >> 12) & 0xF;
	    u32  CRn = (opcode >> 16) & 0xF;
	    u32  CPOpc = (opcode >> 21) & 0x7;

	    // TODO: Some floating point instruction
	    if(CPOpc == 7) {
		ERROR("Not implemented.\n");
		PAUSE();
		return;
	    }

	    if(L) {
		if(CRm == 0 && CP == 3 && CP_num == 15 && CRn == 13 && CPOpc == 0) {
		    // GetThreadCommandBuffer
		    r[Rd] = 0xFFFF0000;
		    return;
		}
	    }
	    else {
	    }

	    ERROR("MRC/MCR not implemented.\n");
	    exit(1);
	    return;
	}
    }
    }

    // XXX: Error here
    ERROR("unknown opcode (0x%08x)\n", opcode);
    exit(1);
}

static void arm11_Disasm32(u32 a)
{
    u32 opcode;

    opcode = mem_Read32(a);

    u32 Rn    = ((opcode >> 16) & 0xF);
    u32 Rd    = ((opcode >> 12) & 0xF);
    u32 Rm    = ((opcode >> 0) & 0xF);
    u32 Rs    = ((opcode >> 8) & 0xF);
    u32 Imm   = ((opcode >> 0) & 0xFF);
    u32 amt   = Rs << 1;

    bool I = (opcode >> 25) & 1;
    bool P = (opcode >> 24) & 1;
    bool U = (opcode >> 23) & 1;
    bool B = (opcode >> 22) & 1;
    bool W = (opcode >> 21) & 1;
    bool S = (opcode >> 20) & 1;
    bool L = (opcode >> 20) & 1;

    if (((opcode >> 8) & 0xFFFFF) == 0x12FFF) {
	bool link = (opcode >> 5) & 1;

	printf("b%sx", (link) ? "l" : "");
	CondPrint32(opcode);
	printf(" r%d\n", Rm);
	return;
    }

    if ((opcode >> 24) == 0xEF) {
	u32 Imm = opcode & 0xFFFFFF;
	printf("swi 0x%X\n", Imm);
	return;
    }

    if (((opcode >> 22) & 0x3F) == 0 &&
	((opcode >>  4) & 0x0F) == 9) {
	printf("%s", (W) ? "mla" : "mul");
	CondPrint32(opcode);
	SuffixPrint(opcode);

	printf(" r%d, r%d, r%d", Rn, Rm, Rs);
	if (W)
	    printf(", r%d", Rd);
	printf("\n");
	return;
    }

    //LDREXB/STREXB/LDREXH/STREXH/LDREX/STREX (ARM11)
    if (((opcode >> 23) & 0x1F) == 3 && 
	((opcode >>  4) & 0xFF) == 0xF9) {

	if((opcode >> 20) & 1) {
	    if((opcode & 0xF) == 0xF) {
		printf("ldrex%c", "\0dbh"[((opcode >> 21) & 3)]);
		CondPrint32(opcode);
		printf(" r%d, [r%d]\n", Rd, Rn);
		return;
	    }
	}
	else {
	    printf("strex%c", "\0dbh"[((opcode >> 21) & 3)]);
	    CondPrint32(opcode);
	    printf(" r%d, r%d, r%d\n", Rd, Rm, Rn);
	    return;
	}
    }
    //CLREX (ARM11)
    if((opcode >> 20) == 0xF57 && (opcode >> 4) == 1) {
	printf("clrex");
	return;
    }
    //NOP (ARM11)
    if(((opcode << 4) >> 12) == 0x320F0) {
	switch(opcode & 0xFF) {
	case 0:
	    printf("nop");
	    return;
	case 1:
	    printf("yield");
	    return;
	default:
	    printf("nop (reserved)");
	    return;
	}
    }


    switch ((opcode >> 26) & 0x3) {
    case 0: {
	switch ((opcode >> 21) & 0xF) {
	case 0: {// AND
	    printf("and");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 1: {// EOR
	    printf("eor");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 2: {// SUB
	    printf("sub");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 3: {// RSB
	    printf("rsb");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 4: {// ADD
	    printf("add");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 5: {// ADC
	    printf("adc");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 6: {// SBC
	    printf("sbc");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 7: {// RSC
	    printf("rsc");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 8: {// TST/MRS
	    if (S) {
		u32 result;

		printf("tst");
		CondPrint32(opcode);

		if (!I) {
		    printf(" r%d, r%d\n", Rn, Rm);
		    ShiftPrint(opcode);
		} else {
		    printf(" r%d, #0x%X\n", Rn, ROR(Imm, amt));
		}
	    } else {
		printf("mrs r%d, cpsr\n", Rd);
	    }

	    return;
	}

	case 9: {// TEQ/MSR
	    if (S) {
		u32 result;

		printf("teq");
		CondPrint32(opcode);

		if (!I) {
		    printf(" r%d, r%d\n", Rn, Rm);
		} else {
		    printf(" r%d, #0x%X\n", Rn, ROR(Imm, amt));
		}
	    } else {
		if (I) {
		    printf("msr cpsr, r%d\n", Rm);
		} else {
		    printf("msr cpsr, 0x%08X\n", Imm);
		}
	    }

	    return;
	}

	case 10: {// CMP/MRS2
	    if (S) {
		u32 value;

		printf("cmp");
		CondPrint32(opcode);

		if (I) {
		    printf(" r%d, 0x%08X\n", Rn, value);
		} else {
		    printf(" r%d, r%d\n", Rn, Rm);
		}
	    } else
		printf("mrs2...\n");

	    return;
	}

	case 11: {// CMN/MSR2
	    if (S) {
		u32 value;

		printf("cmn");
		CondPrint32(opcode);

		if (I) {
		    printf(" r%d, 0x%08X\n", Rn, value);
		} else {
		    printf(" r%d, r%d\n", Rn, Rm);
		}
	    } else
		printf("msr2...\n");

	    return;
	}

	case 12: {// ORR
	    printf("orr");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 13: {// MOV
	    printf("mov");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d", Rd, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, #0x%X", Rd, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 14: {// BIC
	    printf("bic");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d, r%d", Rd, Rn, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, r%d, #0x%X", Rd, Rn, ROR(Imm, amt));

	    printf("\n");
	    return;
	}

	case 15: {// MVN
	    printf("mvn");
	    CondPrint32(opcode);
	    SuffixPrint(opcode);

	    if (!I) {
		printf(" r%d, r%d", Rd, Rm);
		ShiftPrint(opcode);
	    } else
		printf(" r%d, #0x%X", Rd, ROR(Imm, amt));

	    printf("\n");
	    return;
	}
	}
    }

    case 1: {// LDR/STR
	u32  addr, value, wb;

	printf("%s%s", (L) ? "ldr" : "str", (B) ? "b" : "");
	CondPrint32(opcode);
	printf(" r%d,", Rd);

	Imm = opcode & 0xFFF;

	if (L && Rn == 15) {
	    addr  = a + Imm + sizeof(opcode);
	    value = mem_Read32(addr);

	    printf(" =0x%x\n", value);
	    return;
	}

	printf(" [r%d", Rn);

	if (I) {
	    printf(", %sr%d", (U) ? "" : "-", Rm);
	    ShiftPrint(opcode);
	} else {
	    value = Imm;
	    printf(", #%s0x%X", (U) ? "" : "-", value);
	}
	printf("]%s\n", (W) ? "!" : "");
	return;
    }

    default:
	break;
    }

    switch ((opcode >> 25) & 7) {
    case 4: {// LDM/STM
	u32  start = r[Rn];
	bool pf    = false;

	if (L) {
	    printf("ldm");
	    if (Rn == 13)
		printf("%c%c", (P) ? 'e' : 'f', (U) ? 'd' : 'a');
	    else
		printf("%c%c", (U) ? 'i' : 'd', (P) ? 'b' : 'a');
	} else {
	    printf("stm");
	    if (Rn == 13)
		printf("%c%c", (P) ? 'f' : 'e', (U) ? 'a' : 'd');
	    else
		printf("%c%c", (U) ? 'i' : 'd', (P) ? 'b' : 'a');
	}

	if (Rn == 13)
	    printf(" sp");
	else
	    printf(" r%d", Rn);

	if (W) printf("!");
	printf(", {");

	s32 i;
	for (i = 0; i < 16; i++) {
	    if ((opcode >> i) & 1) {
		if (pf) printf(", ");
		printf("r%d", i);

		pf = true;
	    }
	}

	printf("}");
	if (B) {
	    printf("^");
	}
	printf("\n");
	return;
    }

    case 5: {// B/BL
	bool link = opcode & (1 << 24);

	printf("b%s", (link) ? "l" : "");
	CondPrint32(opcode);

	Imm = (opcode & 0xFFFFFF) << 2;
	if (Imm & (1 << 25)) Imm = ~(~Imm & 0xFFFFFF);
	Imm += sizeof(opcode);

	printf(" 0x%08X\n", a + Imm);
	return;
    }

    case 7: {// MRC
	bool L = (opcode >> 20) & 1;
	u32  CRm = opcode & 0xF;
	u32  CP = (opcode >> 5) & 0x7;
	u32  CP_num = (opcode >> 8) & 0xF;
	u32  Rd = (opcode >> 12) & 0xF;
	u32  CRn = (opcode >> 16) & 0xF;
	u32  CPOpc = (opcode >> 21) & 0x7;

	printf("mcr L=%d, CRm=%x, CP=%x, CP_num=%x, Rd=%x, CRn=%x, CPOpc=%x\n",
	       L, CRm, CP, CP_num, Rd, CRn, CPOpc);
	return;
    }
    }

    ERROR("unknown opcode (0x%08x)\n", opcode);
}

void Step16()
{
    u16 opcode;

    /* Read opcode */
    opcode = mem_Read16(*pc);

    /* Update PC */
    *pc += sizeof(opcode);

    if ((opcode >> 13) == 0) {
	u32 Imm = (opcode >> 6) & 0x1F;
	u32 Rn  = (opcode >> 6) & 7;
	u32 Rm  = (opcode >> 3) & 7;
	u32 Rd  = (opcode >> 0) & 7;

	switch ((opcode >> 11) & 3) {
	case 0: {// LSL
	    if (Imm > 0 && Imm <= 32) {
		cpsr.c = r[Rd] & (1 << (32 - Imm));
		r[Rd]  = LSL(r[Rd], Imm);
	    }

	    if (Imm > 32) {
		cpsr.c = 0;
		r[Rd]  = 0;
	    }

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 1: {// LSR
	    if (Imm > 0 && Imm <= 32) {
		cpsr.c = r[Rd] & (1 << (Imm - 1));
		r[Rd]  = LSR(r[Rd], Imm);
	    }

	    if (Imm > 32) {
		cpsr.c = 0;
		r[Rd]  = 0;
	    }

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 2: {// ASR
	    if (Imm > 0 && Imm <= 32) {
		cpsr.c = r[Rd] & (1 << (Imm - 1));
		r[Rd]  = ASR(r[Rd], Imm);
	    }

	    if (Imm > 32) {
		cpsr.c = 0;
		r[Rd]  = 0;
	    }

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 3: {// ADD, SUB
	    if (opcode & 0x400) {
		Imm &= 7;

		if (opcode & 0x200) {
		    r[Rd] = Substract(r[Rm], Imm);
		    return;
		} else {
		    r[Rd] = Addition(r[Rm], Imm);
		}
	    } else {
		if (opcode & 0x200) {
		    r[Rd] = Substract(r[Rm], r[Rn]);
		    return;
		} else {
		    r[Rd] = Addition(r[Rm], r[Rn]);
		    return;
		}
	    }

	    return;
	}
	}
    }

    if ((opcode >> 13) == 1) {
	u32 Imm = (opcode & 0xFF);
	u32 Rn  = (opcode >> 8) & 7;

	switch ((opcode >> 11) & 3) {
	case 0: {// MOV
	    r[Rn] = Imm;

	    cpsr.z = r[Rn] == 0;
	    cpsr.n = r[Rn] >> 31;
	    return;
	}

	case 1: {// CMP
	    Substract(r[Rn], Imm);
	    return;
	}

	case 2: {// ADD
	    r[Rn] = Addition(r[Rn], Imm);
	    return;
	}

	case 3: {// SUB
	    r[Rn] = Substract(r[Rn], Imm);
	    return;
	}
	}
    }

    if ((opcode >> 10) == 0x10) {
	u32 Rd = opcode & 7;
	u32 Rm = (opcode >> 3) & 7;

	switch ((opcode >> 6) & 0xF) {
	case 0: {// AND
	    r[Rd] &= r[Rm];

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 1: {// EOR
	    r[Rd] ^= r[Rm];

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 2: {// LSL
	    u8 shift = r[Rm] & 0xFF;

	    if (shift > 0 && shift <= 32) {
		cpsr.c = r[Rd] & (1 << (32 - shift));
		r[Rd]  = LSL(r[Rd], shift);
	    }

	    if (shift > 32) {
		cpsr.c = 0;
		r[Rd]  = 0;
	    }

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 3: {// LSR
	    u8 shift = r[Rm] & 0xFF;

	    if (shift > 0 && shift <= 32) {
		cpsr.c = r[Rd] & (1 << (shift - 1));
		r[Rd]  = LSR(r[Rd], shift);
	    }

	    if (shift > 32) {
		cpsr.c = 0;
		r[Rd]  = 0;
	    }

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 4: {// ASR
	    u8 shift = r[Rm] & 0xFF;

	    if (shift > 0 && shift < 32) {
		cpsr.c = r[Rd] & (1 << (shift - 1));
		r[Rd]  = ASR(r[Rd], shift);
	    }

	    if (shift == 32) {
		cpsr.c = r[Rd] >> 31;
		r[Rd]  = 0;
	    }

	    if (shift > 32) {
		cpsr.c = 0;
		r[Rd]  = 0;
	    }

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 5: {// ADC
	    r[Rd] = Addition(r[Rd], r[Rm]);
	    r[Rd] = Addition(r[Rd], cpsr.c);

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 6: {// SBC
	    r[Rd] = Substract(r[Rd], r[Rm]);
	    r[Rd] = Substract(r[Rd], !cpsr.c);

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}
	    
	case 7: {// ROR
	    u8 shift = r[Rm] & 0xFF;

	    while (shift >= 32)
		shift -= 32;

	    if (shift) {
		cpsr.c = r[Rd] & (1 << (shift - 1));
		r[Rd]  = ROR(r[Rd], shift);
	    }

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 8: {// TST
	    u32 result = r[Rd] & r[Rm];

	    cpsr.z = result == 0;
	    cpsr.n = result >> 31;
	    return;
	}

	case 9: {// NEG
	    r[Rd] = -r[Rm];

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 10: {// CMP
	    Substract(r[Rd], r[Rm]);
	    return;
	}

	case 11: {// CMN/MVN
	    if (opcode & 0x100) {
		r[Rd] = !r[Rm];

		cpsr.z = r[Rd] == 0;
		cpsr.n = r[Rd] >> 31;
	    } else {
		Addition(r[Rd], r[Rm]);
	    }

	    return;
	}

	case 12: {// ORR
	    r[Rd] |= r[Rm];

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 13: {// MUL
	    r[Rd] *= r[Rm];

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}

	case 14: {// BIC
	    r[Rd] &= ~r[Rm];

	    cpsr.z = r[Rd] == 0;
	    cpsr.n = r[Rd] >> 31;
	    return;
	}
	}
    }

    if ((opcode >> 7) == 0x8F) {
	u32 Rm = (opcode >> 3) & 0xF;

	*lr = *pc | 1;
	
	cpsr.t = r[Rm] & 1;
	*pc    = r[Rm] & ~1;
	return;
    }

    if ((opcode >> 10) == 0x11) {
	u32 Rd = ((opcode >> 4) & 8) | (opcode & 7);
	u32 Rm = ((opcode >> 3) & 0xF);

	switch ((opcode >> 8) & 3) {
	case 0: {// ADD
	    r[Rd] = Addition(r[Rd], r[Rm]);
	    return;
	}

	case 1: {// CMP
	    Substract(r[Rd], r[Rm]);
	    return;
	}

	case 2: {// MOV (NOP)
	    if (Rd == 8 && Rm == 8) {
		return;
	    }

	    r[Rd] = r[Rm];
	    return;
	}

	case 3: {// BX
	    cpsr.t = r[Rm] & 1;

	    if (Rm == 15)
		*pc += sizeof(opcode);
	    else
		*pc = r[Rm] & ~1;
	    return;
	}
	}
    }

    if ((opcode >> 11) == 9) {
	u32 Rd   = (opcode >> 8) & 7;
	u32 Imm  = (opcode & 0xFF);
	u32 addr = *pc + (Imm << 2) + sizeof(opcode);

	r[Rd] = mem_Read32(addr);
	return;
    }

    if ((opcode >> 12) == 5) {
	u32 Rd = (opcode >> 0) & 7;
	u32 Rn = (opcode >> 3) & 7;
	u32 Rm = (opcode >> 6) & 7;

	switch ((opcode >> 9) & 7) {
	case 0: {// STR
	    u32 addr  = r[Rn] + r[Rm];
	    u32 value = r[Rd];

	    mem_Write32(addr, value);
	    return;
	}

	case 2: {// STRB
	    u32 addr  = r[Rn] + r[Rm];
	    u8  value = r[Rd] & 0xFF;

	    mem_Write8(addr, value);
	    return;
	}

	case 4: {// LDR
	    u32 addr = r[Rn] + r[Rm];

	    r[Rd] = mem_Read32(addr);
	    return;
	}

	case 6: {// LDRB
	    u32 addr = r[Rn] + r[Rm];

	    r[Rd] = mem_Read8(addr);
	    return;
	}
	}
    }

    if ((opcode >> 13) == 3) {
	u32 Rd  = (opcode >> 0) & 7;
	u32 Rn  = (opcode >> 3) & 7;
	u32 Imm = (opcode >> 6) & 7;

	if (opcode & 0x1000) {
	    if (opcode & 0x800) {
		u32 addr = r[Rn] + (Imm << 2);

		r[Rd] = mem_Read8(addr);
	    } else {
		u32 addr  = r[Rn] + (Imm << 2);
		u8  value = r[Rd] & 0xFF;

		mem_Write8(addr, value);
	    }
	} else {
	    if (opcode & 0x800) {
		u32 addr = r[Rn] + (Imm << 2);

		r[Rd] = mem_Read32(addr);
	    } else {
		u32 addr  = r[Rn] + (Imm << 2);
		u32 value = r[Rd];

		mem_Write32(addr, value);
	    }
	}
	return;
    }

    if ((opcode >> 12) == 8) {
	u32 Rd  = (opcode >> 0) & 7;
	u32 Rn  = (opcode >> 3) & 7;
	u32 Imm = (opcode >> 6) & 7;

	if (opcode & 0x800) {
	    u32 addr = r[Rn] + (Imm << 1);

	    r[Rd] = mem_Read16(addr);
	} else {
	    u32 addr  = r[Rn] + (Imm << 1);
	    u16 value = r[Rd];

	    mem_Write16(addr, value);
	}

	return;
    }

    if ((opcode >> 12) == 9) {
	u32 Rd  = (opcode >> 8) & 7;
	u32 Imm = (opcode & 0xFF);

	if (opcode & 0x800) {
	    u32 addr = *sp + (Imm << 2);

	    r[Rd] = mem_Read32(addr);
	} else {
	    u32 addr  = *sp + (Imm << 2);
	    u32 value = r[Rd];

	    mem_Write32(addr, value);
	}

	return;
    }

    if ((opcode >> 12) == 10) {
	u32 Rd  = (opcode >> 8) & 7;
	u32 Imm = (opcode & 0xFF);

	if (opcode & 0x800) {
	    r[Rd] = *sp + (Imm << 2);
	} else {
	    r[Rd] = (*pc & ~2) + (Imm << 2);
	}

	return;
    }

    if ((opcode >> 12) == 11) {
	switch ((opcode >> 9) & 7) {
	case 0: {// ADD/SUB
	    u32 Imm = (opcode & 0x7F);

	    if (opcode & 0x80) {
		*sp -= Imm << 2;
	    } else {
		*sp += Imm << 2;
	    }

	    return;
	}

	case 2: {// PUSH
	    bool lrf = opcode & 0x100;
	    bool pf  = false;

	    if (lrf)
		Push(*lr);

	    s32 i;
	    for (i = 7; i >= 0; i--)
		if ((opcode >> i) & 1)
		    Push(r[i]);
	    return;
	}

	case 6: {// POP
	    bool pcf = opcode & 0x100;

	    s32 i;
	    for (i = 0; i < 8; i++) {
		if ((opcode >> i) & 1) {
		    r[i] = Pop();
		}
	    }

	    if (pcf) {
		*pc    = Pop();
		cpsr.t = *pc & 1;
	    }
	    return;
	}
	}
    }

    if ((opcode >> 12) == 12) {
	u32 Rn = (opcode >> 8) & 7;

	if (opcode & 0x800) {
	    u32 i;
	    for (i = 0; i < 8; i++) {
		if ((opcode >> i) & 1) {
		    r[i]   = mem_Read32(r[Rn]);
		    r[Rn] += sizeof(u32);
		}
	    }
	    return;
	} else {
	    u32 i;
	    for (i = 0; i < 8; i++) {
		if ((opcode >> i) & 1) {
		    mem_Write32(r[Rn], r[i]);
		    r[Rn] += sizeof(u32);
		}
	    }
	    return;
	}
    }

    if ((opcode >> 12) == 13) {
	u32 Imm = (opcode & 0xFF) << 1;

	if (Imm & 0x100)
	    Imm = ~((~Imm) & 0xFF);

	Imm += 2;

	if (CondCheck16(opcode))
	    *pc += Imm;
	return;
    }

    if ((opcode >> 11) == 28) {
	u32 Imm = (opcode & 0x7FF) << 1;

	if (Imm & (1 << 11)) {
	    Imm  = (~Imm) & 0xFFE;
	    *pc -= Imm;
	} else
	    *pc += Imm + 2;
	return;
    }

    if ((opcode >> 11) == 0x1E) {
	u32  opc = mem_Read16(*pc);

	u32  Imm = ((opcode & 0x7FF) << 12) | ((opc & 0x7FF) << 1);
	bool blx = ((opcode >> 11) & 3) == 3;

	*lr  = *pc + sizeof(opcode);
	*lr |= 1;

	if (Imm & (1 << 22)) {
	    Imm  = (~Imm) & 0x7FFFFE;
	    *pc -= Imm;
	} else
	    *pc += Imm + 2;

	if (blx) {
	    cpsr.t = 0;
	}

	return;
    }

    ERROR("unknown opcode (0x%04x)\n", opcode);
}

void arm11_Disasm16(u32 a)
{
    u16 opcode;

    opcode = mem_Read16(a);

    if ((opcode >> 13) == 0) {
	u32 Imm = (opcode >> 6) & 0x1F;
	u32 Rn  = (opcode >> 6) & 7;
	u32 Rm  = (opcode >> 3) & 7;
	u32 Rd  = (opcode >> 0) & 7;

	switch ((opcode >> 11) & 3) {
	case 0: {// LSL
	    printf("lsl r%d, r%d, #0x%02X\n", Rd, Rm, Imm);
	    return;
	}

	case 1: {// LSR
	    printf("lsr r%d, r%d, #0x%02X\n", Rd, Rm, Imm);
	    return;
	}

	case 2: {// ASR
	    printf("asr r%d, r%d, #0x%02X\n", Rd, Rm, Imm);
	    return;
	}

	case 3: {// ADD, SUB
	    if (opcode & 0x400) {
		Imm &= 7;

		if (opcode & 0x200) {
		    printf("sub r%d, r%d, #0x%02X\n", Rd, Rm, Imm);
		    return;
		} else {
		    printf("add r%d, r%d, #0x%02X\n", Rd, Rm, Imm);
		}
	    } else {
		if (opcode & 0x200) {
		    printf("sub r%d, r%d, r%d\n", Rd, Rm, Rn);
		    return;
		} else {
		    printf("add r%d, r%d, r%d\n", Rd, Rm, Rn);
		    return;
		}
	    }

	    return;
	}
	}
    }

    if ((opcode >> 13) == 1) {
	u32 Imm = (opcode & 0xFF);
	u32 Rn  = (opcode >> 8) & 7;

	switch ((opcode >> 11) & 3) {
	case 0: {// MOV
	    printf("mov r%d, #0x%02X\n", Rn, Imm);
	    return;
	}

	case 1: {// CMP
	    printf("cmp r%d, #0x%02X\n", Rn, Imm);
	    return;
	}

	case 2: {// ADD
	    printf("add r%d, #0x%02X\n", Rn, Imm);
	    return;
	}

	case 3: {// SUB
	    printf("sub r%d, #0x%02X\n", Rn, Imm);
	    return;
	}
	}
    }

    if ((opcode >> 10) == 0x10) {
	u32 Rd = opcode & 7;
	u32 Rm = (opcode >> 3) & 7;

	switch ((opcode >> 6) & 0xF) {
	case 0: {// AND
	    printf("and r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 1: {// EOR
	    printf("eor r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 2: {// LSL
	    printf("lsl r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 3: {// LSR
	    printf("lsr r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 4: {// ASR
	    printf("asr r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 5: {// ADC
	    printf("adc r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 6: {// SBC
	    printf("sbc r%d, r%d\n", Rd, Rm);
	    return;
	}
	    
	case 7: {// ROR
	    printf("ror r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 8: {// TST
	    printf("tst r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 9: {// NEG
	    printf("neg r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 10: {// CMP
	    printf("cmp r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 11: {// CMN/MVN
	    if (opcode & 0x100) {
		printf("mvn r%d, r%d\n", Rd, Rm);
	    } else {
		printf("cmn r%d, r%d\n", Rd, Rm);
	    }

	    return;
	}

	case 12: {// ORR
	    printf("orr r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 13: {// MUL
	    printf("mul r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 14: {// BIC
	    printf("bic r%d, r%d\n", Rd, Rm);
	    return;
	}
	}
    }

    if ((opcode >> 7) == 0x8F) {
	u32 Rm = (opcode >> 3) & 0xF;

	printf("blx r%d\n", Rm);
	return;
    }

    if ((opcode >> 10) == 0x11) {
	u32 Rd = ((opcode >> 4) & 8) | (opcode & 7);
	u32 Rm = ((opcode >> 3) & 0xF);

	switch ((opcode >> 8) & 3) {
	case 0: {// ADD
	    printf("add r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 1: {// CMP
	    printf("cmp r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 2: {// MOV (NOP)
	    if (Rd == 8 && Rm == 8) {
		printf("nop\n");
		return;
	    }
	    printf("mov r%d, r%d\n", Rd, Rm);
	    return;
	}

	case 3: {// BX
	    printf("bx r%d\n", Rm);
	    return;
	}
	}
    }

    if ((opcode >> 11) == 9) {
	u32 Rd   = (opcode >> 8) & 7;
	u32 Imm  = (opcode & 0xFF);
	u32 addr = a + (Imm << 2) + sizeof(opcode);

	//printf("ldr r%d, =0x%08X\n", Rd, r[Rd]);
	// XXX: print imm loads
	printf("ldr TODO\n");
	return;
    }

    if ((opcode >> 12) == 5) {
	u32 Rd = (opcode >> 0) & 7;
	u32 Rn = (opcode >> 3) & 7;
	u32 Rm = (opcode >> 6) & 7;

	switch ((opcode >> 9) & 7) {
	case 0: {// STR
	    printf("str r%d, [r%d, r%d]\n", Rd, Rn, Rm);
	    return;
	}

	case 2: {// STRB
	    printf("strb r%d, [r%d, r%d]\n", Rd, Rn, Rm);
	    return;
	}

	case 4: {// LDR
	    printf("ldr r%d, [r%d, r%d]\n", Rd, Rn, Rm);
	    return;
	}

	case 6: {// LDRB
	    printf("ldrb r%d, [r%d, r%d]\n", Rd, Rn, Rm);
	    return;
	}
	}
    }

    if ((opcode >> 13) == 3) {
	u32 Rd  = (opcode >> 0) & 7;
	u32 Rn  = (opcode >> 3) & 7;
	u32 Imm = (opcode >> 6) & 7;

	if (opcode & 0x1000) {
	    if (opcode & 0x800) {
		printf("ldrb r%d, [r%d, 0x%02X]\n", Rd, Rn, Imm);
	    } else {
		printf("strb r%d, [r%d, 0x%02X]\n", Rd, Rn, Imm);
	    }
	} else {
	    if (opcode & 0x800) {
		printf("ldr r%d, [r%d, 0x%02X]\n", Rd, Rn, Imm << 2);
	    } else {
		printf("str r%d, [r%d, 0x%02X]\n", Rd, Rn, Imm << 2);
	    }
	}
	return;
    }

    if ((opcode >> 12) == 8) {
	u32 Rd  = (opcode >> 0) & 7;
	u32 Rn  = (opcode >> 3) & 7;
	u32 Imm = (opcode >> 6) & 7;

	if (opcode & 0x800) {
	    printf("ldrh r%d, [r%d, 0x%02X]\n", Rd, Rn, Imm << 1);
	} else {
	    printf("strh r%d, [r%d, 0x%02X]\n", Rd, Rn, Imm << 1);
	}
	return;
    }

    if ((opcode >> 12) == 9) {
	// XXX: only sp (?)

	u32 Rd  = (opcode >> 8) & 7;
	u32 Imm = (opcode & 0xFF);

	if (opcode & 0x800) {
	    printf("ldr r%d, [sp, 0x%02X]\n", Rd, Imm << 2);
	} else {
	    printf("str r%d, [sp, 0x%02X]\n", Rd, Imm << 2);
	}
	return;
    }

    if ((opcode >> 12) == 10) {
	u32 Rd  = (opcode >> 8) & 7;
	u32 Imm = (opcode & 0xFF);

	if (opcode & 0x800) {
	    printf("add r%d, sp, #0x%02X\n", Rd, Imm << 2);
	} else {
	    printf("add r%d, pc, #0x%02X\n", Rd, Imm << 2);
	}
	return;
    }

    if ((opcode >> 12) == 11) {
	switch ((opcode >> 9) & 7) {
	case 0: {// ADD/SUB
	    u32 Imm = (opcode & 0x7F);

	    if (opcode & 0x80) {
		printf("sub sp, #0x%02X\n", Imm << 2);
	    } else {
		printf("add sp, #0x%02X\n", Imm << 2);
	    }
	    return;
	}

	case 2: {// PUSH
	    bool lrf = opcode & 0x100;
	    bool pf  = false;

	    printf("push {");

	    s32 i;
	    for (i = 0; i < 8; i++) {
		if ((opcode >> i) & 1) {
		    if (pf) printf(",");
		    printf("r%d", i);

		    pf = true;
		}
	    }

	    if (lrf) {
		if (pf) printf(",");
		printf("lr");
	    }

	    printf("}\n");
	    return;
	}

	case 6: {// POP
	    bool pcf = opcode & 0x100;
	    bool pf  = false;

	    printf("pop {");

	    s32 i;
	    for (i = 0; i < 8; i++) {
		if ((opcode >> i) & 1) {
		    if (pf) printf(",");
		    printf("r%d", i);
		}
	    }

	    if (pcf) {
		if (pf) printf(",");
		printf("pc");
	    }

	    printf("}\n");
	    return;
	}
	}
    }

    if ((opcode >> 12) == 12) {
	u32 Rn = (opcode >> 8) & 7;

	if (opcode & 0x800) {
	    printf("ldmia r%d!, {", Rn);

	    u32 i;
	    for (i = 0; i < 8; i++) {
		if ((opcode >> i) & 1) {
		    printf("r%d,", i);
		}
	    }
	    printf("}\n");
	    return;
	} else {
	    printf("stmia r%d!, {", Rn);

	    u32 i;
	    for (i = 0; i < 8; i++) {
		if ((opcode >> i) & 1) {
		    printf("r%d,", i);
		}
	    }
	    printf("}\n");
	    return;
	}
    }

    if ((opcode >> 12) == 13) {
	u32 Imm = (opcode & 0xFF) << 1;

	if (Imm & 0x100)
	    Imm = ~((~Imm) & 0xFF);

	Imm += 2;

	printf("b");
	CondPrint16(opcode);
	printf(" 0x%08X\n", a + Imm);

	return;
    }

    if ((opcode >> 11) == 28) {
	u32 Imm = (opcode & 0x7FF) << 1;

	if (Imm & (1 << 11)) {
	    Imm  = (~Imm) & 0xFFE;
	    a -= Imm;
	} else
	    a += Imm + 2;

	printf("b 0x%08X, 0x%X\n", a, Imm);
	return;
    }

    if ((opcode >> 11) == 0x1E) {
	u32  opc = mem_Read16(a);
	u32  Imm = ((opcode & 0x7FF) << 12) | ((opc & 0x7FF) << 1);
	bool blx = ((opcode >> 11) & 3) == 3;

	if (Imm & (1 << 22)) {
	    Imm  = (~Imm) & 0x7FFFFE;
	    a -= Imm;
	} else
	    a += Imm + 2;

	if (blx) {
	    cpsr.t = 0;
	    printf("blx 0x%08X\n", a);
	} else
	    printf("bl 0x%08X\n", a);

	return;
    }

    ERROR("unknown opcode (0x%04x)\n", opcode);
}

bool arm11_Step()
{
    bool ret;

    *pc &= ~1;

    cpsr.t ? Step16() : Step32();
    return true;
}

/*void ARM::BreakAdd(u32 address)
{
    bool ret;

    // Check if exists
    ret = BreakFind(address);

    // Add breakpoint
    if (!ret)
	breakpoint.push_back(address);
}

void ARM::BreakDel(u32 address)
{
    vector<u32>::iterator it;

    // Search breakpoint
    for (it = breakpoint.begin(); it != breakpoint.end(); it++) {
	// Delete breakpoint
	if (*it == address) {
	    breakpoint.erase(it);
	    return;
	}
    }
}

bool ARM::BreakFind(u32 address)
{
    vector<u32>::iterator it;

    // Search breakpoint
    for (it = breakpoint.begin(); it != breakpoint.end(); it++) {
	// Found breakpoint
	if (*it == address)
	    return true;
    }

    return false;
}*/

void arm11_Dump()
{
    printf("\n===========\n");
    printf("STACK DUMP:\n");
    printf("===========\n");

    // Print stack
    u32 i;
    for (i = 0; i < 16; i++) {
	u32 addr = *sp - (i << 2);
	u32 value;

	// Read stack
	value = mem_Read32(addr);

	// Print value
	printf("[-%02d] 0x%08X\n", i, value);
    }

    printf("===============\n");
    printf("REGISTERS DUMP:\n");
    printf("===============\n");

    // Print GPRs
    for (i = 0; i < 16; i += 2)
	printf("r%-2d: 0x%08X\t\tr%-2d: 0x%08X\n", i, r[i], i+1, r[i+1]);

    printf("\n");

    // Print CPSR
    printf("(z: %d, n: %d, c: %d, v: %d, I: %d, F: %d, t: %d, mode: 0x%x)\n",
	   cpsr.z, cpsr.n, cpsr.c, cpsr.v, cpsr.I, cpsr.F, cpsr.t, cpsr.mode);

    // Print SPSR
}
