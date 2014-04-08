/*  armemu.c -- Main instruction emulation:  ARM7 Instruction Emulator.
    Copyright (C) 1994 Advanced RISC Machines Ltd.
    Modifications to add arch. v4 support by <jsmith@cygnus.com>.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <util.h>
#include <svc.h>

#include "arm_regformat.h"
#include "armdefs.h"
#include "armemu.h"

#define DASM(...) do {                           \
        fprintf(stderr, "%08x: ", pc);           \
        fprintf(stderr, __VA_ARGS__);            \
    } while(0)


// PLUTO: TODO
ARMul_State s;

void arm11_Init()
{
    ARMul_EmulateInit ();

    memset(&s, 0, sizeof(s));
}

void arm11_SetPCSP(u32 pc, u32 sp)
{
    s.Reg[15] = pc;
    s.Reg[13] = sp;
}

void arm11_Step()
{
    ARMul_Emulate32 (&s);
    s.Reg[15] += 4;
}

u32 arm11_R(u32 n)
{
    if(n >= 16) {
        DEBUG("Invalid reg num.\n");
        return 0;
    }

    return s.Reg[n];
}

void arm11_SetR(u32 n, u32 val)
{
    if(n >= 16) {
        DEBUG("Invalid reg num.\n");
        return 0;
    }

    s.Reg[n] = val;
}

void arm11_SaveContext()
{

}

void arm11_LoadContext()
{

}

void arm11_Dump()
{
    DEBUG("Reg dump:\n");

    u32 i;
    for(i=0; i<4; i++) {
        DEBUG("r%02d: %08x r%02d: %08x r%02d: %08x r%02d: %08x\n",
              4*i, s.Reg[4*i], 4*i+1, s.Reg[4*i+1], 4*i+2, s.Reg[4*i+2], 4*i+3, s.Reg[4*i+3]);
    }

    DEBUG("\n");
}

void arm11_Disasm32()
{
    DEBUG("!\n");
}

u32 ARMul_LoadWordS (ARMul_State * state, u32 address)
{
    return mem_Read32(address);
}
u32 ARMul_LoadWordN (ARMul_State * state, u32 address)
{
    return mem_Read32(address);
}
u32 ARMul_LoadHalfWord (ARMul_State * state, u32 address)
{
    return mem_Read16(address);
}
u32 ARMul_LoadByte (ARMul_State * state, u32 address)
{
    return mem_Read8(address);
}
void ARMul_StoreWordS (ARMul_State * state, u32 address, u32 data)
{
    mem_Write32(address, data);
}
void ARMul_StoreWordN (ARMul_State * state, u32 address, u32 data)
{
    mem_Write32(address, data);
}
void ARMul_StoreHalfWord (ARMul_State * state, u32 address, u32 data)
{
    mem_Write16(address, data);
}
void ARMul_StoreByte (ARMul_State * state, u32 address, u32 data)
{
    mem_Write8(address, data);
}
u32 ARMul_SwapWord (ARMul_State * state, u32 address, u32 data)
{
    u32 ret = mem_Read32(address);
    mem_Write32(address, data);
    return ret;
}
u32 ARMul_SwapByte (ARMul_State * state, u32 address, u32 data)
{
    u32 ret = mem_Read8(address);
    mem_Write8(address, data);
    return ret;
}

unsigned ARMul_MultTable[32] = {
    1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,
    10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 16
};
u32 ARMul_ImmedTable[4096];	/* immediate DP LHS values */
char ARMul_BitList[256];	/* number of bits in a byte table */

void ARMul_EmulateInit (void)
{
    unsigned int i, j;

    for (i = 0; i < 4096; i++) { /* the values of 12 bit dp rhs's */
        ARMul_ImmedTable[i] = ROTATER (i & 0xffL, (i >> 7L) & 0x1eL);
    }

    for (i = 0; i < 256; ARMul_BitList[i++] = 0); /* how many bits in LSM */
    for (j = 1; j < 256; j <<= 1)
        for (i = 0; i < 256; i++)
            if ((i & j) > 0)
                ARMul_BitList[i]++;

    for (i = 0; i < 256; i++)
        ARMul_BitList[i] *= 4;	/* you always need 4 times these values */
}

static u32 GetDPRegRHS (ARMul_State *, u32);
static u32 GetDPSRegRHS (ARMul_State *, u32);
static void WriteR15 (ARMul_State *, u32);
static void WriteSR15 (ARMul_State *, u32);
static void WriteR15Branch (ARMul_State *, u32);
static u32 GetLSRegRHS (ARMul_State *, u32);
static u32 GetLS7RHS (ARMul_State *, u32);
static unsigned LoadWord (ARMul_State *, u32, u32);
static unsigned LoadHalfWord (ARMul_State *, u32, u32, int);
static unsigned LoadByte (ARMul_State *, u32, u32, int);
static unsigned StoreWord (ARMul_State *, u32, u32);
static unsigned StoreHalfWord (ARMul_State *, u32, u32);
static unsigned StoreByte (ARMul_State *, u32, u32);
static void LoadMult (ARMul_State *, u32, u32, u32);
static void StoreMult (ARMul_State *, u32, u32, u32);
static void LoadSMult (ARMul_State *, u32, u32, u32);
static void StoreSMult (ARMul_State *, u32, u32, u32);
static unsigned Multiply64 (ARMul_State *, u32, int, int);
static unsigned MultiplyAdd64 (ARMul_State *, u32, int, int);
static void Handle_Load_Double (ARMul_State *, u32);
static void Handle_Store_Double (ARMul_State *, u32);

static int handle_v6_insn (ARMul_State * state, u32 instr);

#define LUNSIGNED (0)		/* unsigned operation */
#define LSIGNED   (1)		/* signed operation */
#define LDEFAULT  (0)		/* default : do nothing */
#define LSCC      (1)		/* set condition codes on result */


/* Short-hand macros for LDR/STR.  */

/* Store post decrement writeback.  */
#define SHDOWNWB()                                      \
  lhs = LHS ;                                           \
  if (StoreHalfWord (state, instr, lhs))                \
     LSBase = lhs - GetLS7RHS (state, instr);

/* Store post increment writeback.  */
#define SHUPWB()                                        \
  lhs = LHS ;                                           \
  if (StoreHalfWord (state, instr, lhs))                \
     LSBase = lhs + GetLS7RHS (state, instr);

/* Store pre decrement.  */
#define SHPREDOWN()                                     \
  (void)StoreHalfWord (state, instr, LHS - GetLS7RHS (state, instr));

/* Store pre decrement writeback.  */
#define SHPREDOWNWB()                                   \
  temp = LHS - GetLS7RHS (state, instr);                \
  if (StoreHalfWord (state, instr, temp))               \
     LSBase = temp;

/* Store pre increment.  */
#define SHPREUP()                                       \
  (void)StoreHalfWord (state, instr, LHS + GetLS7RHS (state, instr));

/* Store pre increment writeback.  */
#define SHPREUPWB()                                     \
  temp = LHS + GetLS7RHS (state, instr);                \
  if (StoreHalfWord (state, instr, temp))               \
     LSBase = temp;

/* Load post decrement writeback.  */
#define LHPOSTDOWN()                                    \
{                                                       \
  int done = 1;                                        	\
  lhs = LHS;						\
  temp = lhs - GetLS7RHS (state, instr);		\
  							\
  switch (BITS (5, 6))					\
    {                                  			\
    case 1: /* H */                                     \
      if (LoadHalfWord (state, instr, lhs, LUNSIGNED))  \
         LSBase = temp;        				\
      break;                                           	\
    case 2: /* SB */                                    \
      if (LoadByte (state, instr, lhs, LSIGNED))        \
         LSBase = temp;        				\
      break;                                           	\
    case 3: /* SH */                                    \
      if (LoadHalfWord (state, instr, lhs, LSIGNED))    \
         LSBase = temp;        				\
      break;                                           	\
    case 0: /* SWP handled elsewhere.  */               \
    default:                                            \
      done = 0;                                        	\
      break;                                           	\
    }                                                   \
  if (done)                                             \
     break;                                            	\
}

/* Load post increment writeback.  */
#define LHPOSTUP()                                      \
{                                                       \
  int done = 1;                                        	\
  lhs = LHS;                                           	\
  temp = lhs + GetLS7RHS (state, instr);		\
  							\
  switch (BITS (5, 6))					\
    {                                  			\
    case 1: /* H */                                     \
      if (LoadHalfWord (state, instr, lhs, LUNSIGNED))  \
         LSBase = temp;        				\
      break;                                           	\
    case 2: /* SB */                                    \
      if (LoadByte (state, instr, lhs, LSIGNED))        \
         LSBase = temp;        				\
      break;                                           	\
    case 3: /* SH */                                    \
      if (LoadHalfWord (state, instr, lhs, LSIGNED))    \
         LSBase = temp;        				\
      break;                                           	\
    case 0: /* SWP handled elsewhere.  */               \
    default:                                            \
      done = 0;                                        	\
      break;                                           	\
    }                                                   \
  if (done)                                             \
     break;                                            	\
}

/* Load pre decrement.  */
#define LHPREDOWN()                                     	\
{                                                       	\
  int done = 1;                                        		\
								\
  temp = LHS - GetLS7RHS (state, instr);                 	\
  switch (BITS (5, 6))						\
    {                                  				\
    case 1: /* H */                                     	\
      (void) LoadHalfWord (state, instr, temp, LUNSIGNED);  	\
      break;                                           		\
    case 2: /* SB */                                    	\
      (void) LoadByte (state, instr, temp, LSIGNED);        	\
      break;                                           		\
    case 3: /* SH */                                    	\
      (void) LoadHalfWord (state, instr, temp, LSIGNED);    	\
      break;                                           		\
    case 0:							\
      /* SWP handled elsewhere.  */                 		\
    default:                                            	\
      done = 0;                                        		\
      break;                                           		\
    }                                                   	\
  if (done)                                             	\
     break;                                            		\
}

/* Load pre decrement writeback.  */
#define LHPREDOWNWB()                                   	\
{                                                       	\
  int done = 1;                                        		\
								\
  temp = LHS - GetLS7RHS (state, instr);                	\
  switch (BITS (5, 6))						\
    {                                  				\
    case 1: /* H */                                     	\
      if (LoadHalfWord (state, instr, temp, LUNSIGNED))     	\
         LSBase = temp;                                		\
      break;                                           		\
    case 2: /* SB */                                    	\
      if (LoadByte (state, instr, temp, LSIGNED))           	\
         LSBase = temp;                                		\
      break;                                           		\
    case 3: /* SH */                                    	\
      if (LoadHalfWord (state, instr, temp, LSIGNED))       	\
         LSBase = temp;                                		\
      break;                                           		\
    case 0:							\
      /* SWP handled elsewhere.  */                 		\
    default:                                            	\
      done = 0;                                        		\
      break;                                           		\
    }                                                   	\
  if (done)                                             	\
     break;                                            		\
}

/* Load pre increment.  */
#define LHPREUP()                                       	\
{                                                       	\
  int done = 1;                                        		\
								\
  temp = LHS + GetLS7RHS (state, instr);                 	\
  switch (BITS (5, 6))						\
    {                                  				\
    case 1: /* H */                                     	\
      (void) LoadHalfWord (state, instr, temp, LUNSIGNED);  	\
      break;                                           		\
    case 2: /* SB */                                    	\
      (void) LoadByte (state, instr, temp, LSIGNED);        	\
      break;                                           		\
    case 3: /* SH */                                    	\
      (void) LoadHalfWord (state, instr, temp, LSIGNED);    	\
      break;                                           		\
    case 0:							\
      /* SWP handled elsewhere.  */                 		\
    default:                                            	\
      done = 0;                                        		\
      break;                                           		\
    }                                                   	\
  if (done)                                             	\
     break;                                            		\
}

/* Load pre increment writeback.  */
#define LHPREUPWB()                                     	\
{                                                       	\
  int done = 1;                                        		\
								\
  temp = LHS + GetLS7RHS (state, instr);                	\
  switch (BITS (5, 6))						\
    {                                  				\
    case 1: /* H */                                     	\
      if (LoadHalfWord (state, instr, temp, LUNSIGNED))     	\
	LSBase = temp;                                		\
      break;                                           		\
    case 2: /* SB */                                    	\
      if (LoadByte (state, instr, temp, LSIGNED))           	\
	LSBase = temp;                                		\
      break;                                           		\
    case 3: /* SH */                                    	\
      if (LoadHalfWord (state, instr, temp, LSIGNED))       	\
	LSBase = temp;                                		\
      break;                                           		\
    case 0:							\
      /* SWP handled elsewhere.  */                 		\
    default:                                            	\
      done = 0;                                        		\
      break;                                           		\
    }                                                   	\
  if (done)                                             	\
     break;                                            		\
}

// return nonzero if mem breakpoint
int ARMul_ICE_debug(ARMul_State *state,u32 instr,u32 addr)
{
    return 0;
}

#define isize ((state->TFlag) ? 2 : 4)

u32
ARMul_Emulate32 (ARMul_State * state)
{
    u32 instr;		/* The current instruction.  */
    u32 dest = 0;	/* Almost the DestBus.  */
    u32 temp;		/* Ubiquitous third hand.  */
    u32 pc = 0;		/* The address of the current instruction.  */
    u32 lhs;		/* Almost the ABus and BBus.  */
    u32 rhs;
    u32 decoded = 0;	/* Instruction pipeline.  */
    u32 loaded = 0;
    u32 decoded_addr=0;
    u32 loaded_addr=0;
    u32 have_bp=0;

    int reg_index = 0;

    pc = state->Reg[15];

    if (state->TFlag) {
        DEBUG("--- Thumb ---\n");
        u32 new;

        instr = mem_Read16(pc & ~1);

        /* Check if in Thumb mode.  */
        switch (ARMul_ThumbDecode (state, pc, instr, &new)) {
        case t_undefined:
            /* Thumb instruction not available. */
            return 1;

        case t_branch:
            /* Already processed.  */
            return 0;

        case t_decoded:
            /* Thumb translated to ARM32, continue..  */
            instr = new;
            break;
        default:
            break;
        }
    } else {
        //DEBUG("ARM32-mode.\n");
        instr = mem_Read32(pc & ~3);
    }

    /* Vile deed in the need for speed. */
    if ((temp = TOPBITS (28)) == AL) {
        goto mainswitch;
    }

    /* Check the condition code. */
    switch ((int) TOPBITS (28)) {
    case AL:
        temp = TRUE;
        break;
    case NV:
        /* ARMv7:
           if ((instr & 0x0fffff00) == 0x057ff000) {
           switch((instr >> 4) & 0xf) {
           case 4: // dsb
           case 5: // dmb
           case 6: // isb
           // TODO: do no implemented thes instr
           goto donext;
           break;
           }
           }
        */
        if (instr == 0xf57ff01f) {
            DASM("clrex\n");
            return 0;
        }

        /* ????:
           if (BITS(20, 27) == 0x10) {
           if (BIT(19)) {
           if (BIT(8)) {
           if (BIT(18))
           state->Cpsr |= 1<<8;
           else
           state->Cpsr &= ~(1<<8);
           }
           if (BIT(7)) {
           if (BIT(18))
           state->Cpsr |= 1<<7;
           else
           state->Cpsr &= ~(1<<7);
           ASSIGNINT (state->Cpsr & INTBITS);
           }
           if (BIT(6)) {
           if (BIT(18))
           state->Cpsr |= 1<<6;
           else
           state->Cpsr &= ~(1<<6);
           ASSIGNINT (state->Cpsr & INTBITS);
           }
           }
           if (BIT(17)) {
           state->Cpsr |= BITS(0, 4);
           }
           goto donext;
           }
        */

        /* BLX(1) */
        if (BITS (25, 27) == 5) {
            DASM("blx\n");
            u32 dest;

            state->Reg[14] = pc + 4;

            /* Force entry into Thumb mode.  */
            dest = pc + 4 + 1;
            if (BIT (23))
                dest += (NEGBRANCH + (BIT (24) << 1));
            else
                dest += POSBRANCH + (BIT (24) << 1);

            WriteR15Branch (state, dest);

            return 0;
        } else if ((instr & 0xFC70F000) == 0xF450F000) {
            DASM("pld\n");
            return 0;
        } else if (((instr & 0xfe500f00) == 0xfc100100) || ((instr & 0xfe500f00) == 0xfc000100)) {
            /* wldrw and wstrw are unconditional.  */
            goto mainswitch;
        } else {
            DEBUG("Invalid NV-instr.\n");
            return 1;
        }

        temp = FALSE;
        break;
    case EQ:
        temp = ZFLAG;
        break;
    case NE:
        temp = !ZFLAG;
        break;
    case VS:
        temp = VFLAG;
        break;
    case VC:
        temp = !VFLAG;
        break;
    case MI:
        temp = NFLAG;
        break;
    case PL:
        temp = !NFLAG;
        break;
    case CS:
        temp = CFLAG;
        break;
    case CC:
        temp = !CFLAG;
        break;
    case HI:
        temp = (CFLAG && !ZFLAG);
        break;
    case LS:
        temp = (!CFLAG || ZFLAG);
        break;
    case GE:
        temp = ((!NFLAG && !VFLAG) || (NFLAG && VFLAG));
        break;
    case LT:
        temp = ((NFLAG && !VFLAG) || (!NFLAG && VFLAG));
        break;
    case GT:
        temp = ((!NFLAG && !VFLAG && !ZFLAG)
                || (NFLAG && VFLAG && !ZFLAG));
        break;
    case LE:
        temp = ((NFLAG && !VFLAG) || (!NFLAG && VFLAG))
               || ZFLAG;
        break;
    }

    /* Actual execution of instructions begins here.  */
    /* If the condition codes don't match, stop here.  */
    if (temp) {
        unsigned int m, lsb, width, Rd, Rn, data;

mainswitch:
        /* shenoubang sbfx and ubfx instr 2012-3-16 */
        Rd = Rn = lsb = width = data = m = 0;

        if ((((int) BITS (21, 27)) == 0x3f) && (((int) BITS (4, 6)) == 0x5)) {
            DASM("sbfx\n");

            m = (unsigned)BITS(7, 11);
            width = (unsigned)BITS(16, 20);
            Rd = (unsigned)BITS(12, 15);
            Rn = (unsigned)BITS(0, 3);
            if ((Rd == 15) || (Rn == 15)) {
                return 1;
            } else if ((m + width) < 32) {
                data = state->Reg[Rn];
                state->Reg[Rd] ^= state->Reg[Rd];
                state->Reg[Rd] = ((u32)(data << (31 -(m + width))) >> ((31 - (m + width)) + (m)));
                return 0;
            }
        } // ubfx instr
        else if ((((int) BITS (21, 27)) == 0x3d) && (((int) BITS (4, 6)) == 0x5)) {
            DASM("ubfx\n");

            int tmp = 0;
            Rd = BITS(12, 15);
            Rn = BITS(0, 3);
            lsb = BITS(7, 11);
            width = BITS(16, 20);
            if ((Rd == 15) || (Rn == 15)) {
                ERROR("Undefined when using R15.\n");
                return 1;
            } else if ((lsb + width) < 32) {
                state->Reg[Rd] ^= state->Reg[Rd];
                data = state->Reg[Rn];
                tmp = (data << (32 - (lsb + width + 1)));
                state->Reg[Rd] = (tmp >> (32 - (lsb + width + 1)));
                return 0;
            }
        } // sbfx instr
        else if ((((int)BITS(21, 27)) == 0x3e) && ((int)BITS(4, 6) == 0x1)) {
            DASM("sbfx\n");

            unsigned msb ,tmp_rn, tmp_rd, dst;
            msb = tmp_rd = tmp_rn = dst = 0;
            Rd = BITS(12, 15);
            Rn = BITS(0, 3);
            lsb = BITS(7, 11);
            msb = BITS(16, 20);
            if ((Rd == 15)) {
                DEBUG("Undefined when using R15.\n");
                return 1;
            } else if ((Rn == 15)) {
                data = state->Reg[Rd];
                tmp_rd = ((u32)(data << (31 - lsb)) >> (31 - lsb));
                dst = ((data >> msb) << (msb - lsb));
                dst = (dst << lsb) | tmp_rd;
                DASM("BFC instr: msb = %d, lsb = %d, Rd[%d] : 0x%x, dst = 0x%x\n",
                     msb, lsb, Rd, state->Reg[Rd], dst);
                return 0;
            } // bfc instr
            else if (((msb >= lsb) && (msb < 32))) {
                data = state->Reg[Rn];
                tmp_rn = ((u32)(data << (31 - (msb - lsb))) >> (31 - (msb - lsb)));
                data = state->Reg[Rd];
                tmp_rd = ((u32)(data << (31 - lsb)) >> (31 - lsb));
                dst = ((data >> msb) << (msb - lsb)) | tmp_rn;
                dst = (dst << lsb) | tmp_rd;
                DASM("BFI instr: msb = %d, lsb = %d, Rd[%d] : 0x%x, Rn[%d]: 0x%x, dst = 0x%x\n",
                     msb, lsb, Rd, state->Reg[Rd], Rn, state->Reg[Rn], dst);
                return 0;
            } // bfi instr
        }

        switch ((int) BITS (20, 27)) {
        /* Data Processing Register RHS Instructions.  */

        case 0x00:	/* AND reg and MUL */
            if (BITS (4, 11) == 0xB) {
                /* STRH register offset, no write-back, down, post indexed.  */
                DASM("strh\n");
                SHDOWNWB ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("ldrd\n");
                Handle_Store_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 9) {
                DASM("mul\n");
                /* MUL */
                rhs = state->Reg[MULRHSReg];
                if (MULDESTReg != 15)
                    state->Reg[MULDESTReg] = state->Reg[MULLHSReg] * rhs;
                else {
                    DEBUG("Undefined MUL destination.\n");
                    return 1;
                }

                for (dest = 0, temp = 0; dest < 32; dest++)
                    if (rhs & (1L << dest))
                        temp = dest;
            } else {
                DASM("and\n");
                /* AND reg.  */
                rhs = DPRegRHS;
                dest = LHS & rhs;
                WRITEDEST (dest);
            }
            break;

        case 0x01:	/* ANDS reg and MULS */
            if ((BITS (4, 11) & 0xF9) == 0x9) {
                /* LDR register offset, no write-back, down, post indexed.  */
                LHPOSTDOWN ();
            }

            /* Fall through to rest of decoding.  */
            if (BITS (4, 7) == 9) {
                DASM("muls\n");
                /* MULS */
                rhs = state->Reg[MULRHSReg];

                if (MULDESTReg != 15) {
                    dest = state->Reg[MULLHSReg] * rhs;
                    ARMul_NegZero (state, dest);
                    state->Reg[MULDESTReg] = dest;
                } else {
                    DEBUG("Undefined MUL destination.\n");
                    return 1;
                }

                for (dest = 0, temp = 0; dest < 32; dest++)
                    if (rhs & (1L << dest))
                        temp = dest;
            } else {
                DASM("ands\n");
                /* ANDS reg.  */
                rhs = DPSRegRHS;
                dest = LHS & rhs;
                WRITESDEST (dest);
            }
            break;

        case 0x02:	/* EOR reg and MLA */
            if (BITS (4, 11) == 0xB) {
                DASM("strh\n");
                /* STRH register offset, write-back, down, post indexed.  */
                SHDOWNWB ();
                break;
            }
            if (BITS (4, 7) == 9) {
                DASM("mla\n");

                /* MLA */
                rhs = state->Reg[MULRHSReg];
                if (MULDESTReg != 15) {
                    state->Reg[MULDESTReg] =
                        state->
                        Reg[MULLHSReg] * rhs +
                        state->Reg[MULACCReg];
                } else {
                    DEBUG("Undefined MUL destination.\n");
                    return 1;
                }

                for (dest = 0, temp = 0; dest < 32; dest++)
                    if (rhs & (1L << dest))
                        temp = dest;
            } else {
                DASM("eor\n");
                rhs = DPRegRHS;
                dest = LHS ^ rhs;
                WRITEDEST (dest);
            }
            break;

        case 0x03:	/* EORS reg and MLAS */

            if ((BITS (4, 11) & 0xF9) == 0x9) {
                DASM("ldr\n");
                /* LDR register offset, write-back, down, post-indexed.  */
                LHPOSTDOWN ();
            }

            /* Fall through to rest of the decoding.  */
            if (BITS (4, 7) == 9) {
                DASM("mlas\n");

                /* MLAS */
                rhs = state->Reg[MULRHSReg];
                if (MULDESTReg != 15) {
                    dest = state->Reg[MULLHSReg] * rhs + state->Reg[MULACCReg];
                    ARMul_NegZero (state, dest);
                    state->Reg[MULDESTReg] = dest;
                } else {
                    DEBUG("Undefined MUL destination.\n");
                    return 1;
                }

                for (dest = 0, temp = 0; dest < 32; dest++)
                    if (rhs & (1L << dest))
                        temp = dest;
            } else {
                DASM("eors\n");
                /* EORS Reg.  */
                rhs = DPSRegRHS;
                dest = LHS ^ rhs;
                WRITESDEST (dest);
            }
            break;

        case 0x04:	/* SUB reg */
            if (BITS (4, 7) == 0xB) {
                DASM("strh\n");

                /* STRH immediate offset, no write-back, down, post indexed.  */
                SHDOWNWB ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }

            DASM("sub\n");
            rhs = DPRegRHS;
            dest = LHS - rhs;
            WRITEDEST (dest);
            break;

        case 0x05:	/* SUBS reg */
            if ((BITS (4, 7) & 0x9) == 0x9) {
                DASM("ldr\n");
                /* LDR immediate offset, no write-back, down, post indexed.  */
                LHPOSTDOWN ();
            }

            DASM("subs\n");
            /* Fall through to the rest of the instruction decoding.  */
            lhs = LHS;
            rhs = DPRegRHS;
            dest = lhs - rhs;

            if ((lhs >= rhs) || ((rhs | lhs) >> 31)) {
                ARMul_SubCarry (state, lhs, rhs, dest);
                ARMul_SubOverflow (state, lhs, rhs, dest);
            } else {
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x06:	/* RSB reg */

            if (BITS (4, 7) == 0xB) {
                DASM("strh\n");

                /* STRH immediate offset, write-back, down, post indexed.  */
                SHDOWNWB ();
                break;
            }

            DASM("rsb\n");
            rhs = DPRegRHS;
            dest = rhs - LHS;
            WRITEDEST (dest);
            break;

        case 0x07:	/* RSBS reg */
            if ((BITS (4, 7) & 0x9) == 0x9) {
                DASM("ldr\n");

                /* LDR immediate offset, write-back, down, post indexed.  */
                LHPOSTDOWN ();
            }

            DASM("rsbs\n");

            /* Fall through to remainder of instruction decoding.  */
            lhs = LHS;
            rhs = DPRegRHS;
            dest = rhs - lhs;

            if ((rhs >= lhs) || ((rhs | lhs) >> 31)) {
                ARMul_SubCarry (state, rhs, lhs, dest);
                ARMul_SubOverflow (state, rhs, lhs, dest);
            } else {
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x08:	/* ADD reg */
            if (BITS (4, 11) == 0xB) {
                DASM("strh\n");

                /* STRH register offset, no write-back, up, post indexed.  */
                SHUPWB ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0x9) {
                DASM("mull\n");

                /* MULL */
                /* 32x32 = 64 */
                Multiply64 (state, instr, LUNSIGNED, LDEFAULT);
                break;
            }

            DASM("add\n");
            rhs = DPRegRHS;
            dest = LHS + rhs;
            WRITEDEST (dest);
            break;

        case 0x09:	/* ADDS reg */
            if ((BITS (4, 11) & 0xF9) == 0x9) {
                DASM("ldr\n");
                /* LDR register offset, no write-back, up, post indexed.  */
                LHPOSTUP ();
            }

            /* Fall through to remaining instruction decoding.  */
            if (BITS (4, 7) == 0x9) {
                DASM("mull\n");
                /* MULL */
                /* 32x32=64 */
                Multiply64 (state, instr, LUNSIGNED, LSCC);
                break;
            }

            DASM("adds\n");
            lhs = LHS;
            rhs = DPRegRHS;
            dest = lhs + rhs;
            ASSIGNZ (dest == 0);

            if ((lhs | rhs) >> 30) {
                /* Possible C,V,N to set.  */
                ASSIGNN (NEG (dest));
                ARMul_AddCarry (state, lhs, rhs, dest);
                ARMul_AddOverflow (state, lhs, rhs, dest);
            } else {
                CLEARN;
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x0a:	/* ADC reg */
            if (BITS (4, 11) == 0xB) {
                /* STRH register offset, write-back, up, post-indexed.  */
                DASM("strh\n");
                SHUPWB ();
                break;
            }
            if (BITS (4, 7) == 0x9) {
                /* MULL */
                DASM("mull\n");
                /* 32x32=64 */
                MultiplyAdd64 (state, instr, LUNSIGNED, LDEFAULT);
                break;
            }
            rhs = DPRegRHS;
            dest = LHS + rhs + CFLAG;
            WRITEDEST (dest);
            break;

        case 0x0b:	/* ADCS reg */
            if ((BITS (4, 11) & 0xF9) == 0x9) {
                /* LDR register offset, write-back, up, post indexed.  */
                DASM("ldr\n");
                LHPOSTUP ();
            }
            /* Fall through to remaining instruction decoding.  */
            if (BITS (4, 7) == 0x9) {
                DASM("mull\n");
                /* MULL */
                /* 32x32=64 */
                MultiplyAdd64 (state, instr, LUNSIGNED, LSCC);
                break;
            }

            DASM("adcs\n");
            lhs = LHS;
            rhs = DPRegRHS;
            dest = lhs + rhs + CFLAG;
            ASSIGNZ (dest == 0);

            if ((lhs | rhs) >> 30) {
                /* Possible C,V,N to set.  */
                ASSIGNN (NEG (dest));
                ARMul_AddCarry (state, lhs, rhs, dest);
                ARMul_AddOverflow (state, lhs, rhs, dest);
            } else {
                CLEARN;
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x0c:	/* SBC reg */
            if (BITS (4, 7) == 0xB) {
                DASM("strh\n");

                /* STRH immediate offset, no write-back, up post indexed.  */
                SHUPWB ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0x9) {
                DASM("mull\n");
                /* MULL */
                /* 32x32=64 */
                Multiply64 (state,
                            instr,
                            LSIGNED,
                            LDEFAULT);
                break;
            }

            DASM("sbc\n");
            rhs = DPRegRHS;
            dest = LHS - rhs - !CFLAG;
            WRITEDEST (dest);
            break;

        case 0x0d:	/* SBCS reg */
            if ((BITS (4, 7) & 0x9) == 0x9) {
                DASM("ldr\n");
                /* LDR immediate offset, no write-back, up, post indexed.  */
                LHPOSTUP ();
            }

            if (BITS (4, 7) == 0x9) {
                DASM("mull\n");
                /* MULL */
                /* 32x32=64 */
                Multiply64 (state, instr, LSIGNED, LSCC);
                break;
            }

            DASM("sbcs\n");
            lhs = LHS;
            rhs = DPRegRHS;
            dest = lhs - rhs - !CFLAG;
            if ((lhs >= rhs) || ((rhs | lhs) >> 31)) {
                ARMul_SubCarry (state, lhs, rhs, dest);
                ARMul_SubOverflow (state, lhs, rhs, dest);
            } else {
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x0e:	/* RSC reg */
            if (BITS (4, 7) == 0xB) {
                DASM("strh\n");
                /* STRH immediate offset, write-back, up, post indexed.  */
                SHUPWB ();
                break;
            }

            if (BITS (4, 7) == 0x9) {
                DASM("mull\n");
                /* MULL */
                /* 32x32=64 */
                MultiplyAdd64 (state, instr, LSIGNED, LDEFAULT);
                break;
            }
            rhs = DPRegRHS;
            dest = rhs - LHS - !CFLAG;
            WRITEDEST (dest);
            break;

        case 0x0f:	/* RSCS reg */
            if ((BITS (4, 7) & 0x9) == 0x9) {
                DASM("ldr\n");
                /* LDR immediate offset, write-back, up, post indexed.  */
                LHPOSTUP ();
            }

            /* Fall through to remaining instruction decoding.  */
            if (BITS (4, 7) == 0x9) {
                DASM("mull\n");
                /* MULL */
                /* 32x32=64 */
                MultiplyAdd64 (state, instr, LSIGNED, LSCC);
                break;
            }

            DASM("rscs\n");
            lhs = LHS;
            rhs = DPRegRHS;
            dest = rhs - lhs - !CFLAG;

            if ((rhs >= lhs) || ((rhs | lhs) >> 31)) {
                ARMul_SubCarry (state, rhs, lhs,
                                dest);
                ARMul_SubOverflow (state, rhs, lhs,
                                   dest);
            } else {
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x10:	/* TST reg and MRS CPSR and SWP word.  */
            if (BIT (4) == 0 && BIT (7) == 1) {
                DASM("smla\n");

                /* ElSegundo SMLAxy insn.  */
                u32 op1 = state->Reg[BITS (0, 3)];
                u32 op2 = state->Reg[BITS (8, 11)];
                u32 Rn = state->Reg[BITS (12, 15)];

                if (BIT (5))
                    op1 >>= 16;
                if (BIT (6))
                    op2 >>= 16;
                op1 &= 0xFFFF;
                op2 &= 0xFFFF;
                if (op1 & 0x8000)
                    op1 -= 65536;
                if (op2 & 0x8000)
                    op2 -= 65536;
                op1 *= op2;

                if (AddOverflow(op1, Rn, op1 + Rn))
                    SETS;
                state->Reg[BITS (16, 19)] = op1 + Rn;
                break;
            }

            if (BITS (4, 11) == 5) {
                DASM("qadd\n");

                /* ElSegundo QADD insn.  */
                u32 op1 = state->Reg[BITS (0, 3)];
                u32 op2 = state->Reg[BITS (16, 19)];
                u32 result = op1 + op2;
                if (AddOverflow(op1, op2, result)) {
                    result = POS (result) ? 0x80000000 : 0x7fffffff;
                    SETS;
                }
                state->Reg[BITS (12, 15)] = result;
                break;
            }

            if (BITS (4, 11) == 0xB) {
                DASM("strh\n");
                /* STRH register offset, no write-back, down, pre indexed.  */
                SHPREDOWN ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }

            if (BITS (4, 11) == 9) {
                DASM("swp\n");
                /* SWP */
                UNDEF_SWPPC;
                temp = LHS;
                BUSUSEDINCPCS;

                dest = ARMul_SwapWord (state, temp, state->Reg[RHSReg]);
                if (temp & 3)
                    DEST = ARMul_Align (state, temp, dest);
                else
                    DEST = dest;
            } else if ((BITS (0, 11) == 0) && (LHSReg == 15)) {	/* MRS CPSR */
                DASM("Undefined MRS.\n");
                UNDEF_MRSPC;
                DEST = 0;//ECC | EINT | EMODE;
                return 1;
            } else {
                DEBUG("Undefined ???.\n");
                UNDEF_Test;
                return 1;
            }
            break;

        case 0x11:	/* TSTP reg */
            if ((BITS (4, 11) & 0xF9) == 0x9) {
                DASM("ldr\n");
                /* LDR register offset, no write-back, down, pre indexed.  */
                LHPREDOWN ();
            }

            /* Continue with remaining instruction decode.  */
            if (DESTReg == 15) {
                DEBUG("Undefined TSTP, using R15.\n");
                UNDEF_MRSPC;
                return 1;
            } else {
                DASM("tst\n");
                /* TST reg */
                rhs = DPSRegRHS;
                dest = LHS & rhs;
                ARMul_NegZero (state, dest);
            }
            break;

        case 0x12:	/* TEQ reg and MSR reg to CPSR (ARM6).  */
            if (BITS (4, 7) == 3) {
                DASM("blx\n");

                /* BLX(2) */
                u32 temp;

                if (TFLAG)
                    temp = (pc + 2) | 1;
                else
                    temp = pc + 4;

                WriteR15Branch (state, state->Reg[RHSReg]);
                state->Reg[14] = temp;
                break;
            }

            if (BIT (4) == 0 && BIT (7) == 1 && (BIT (5) == 0 || BITS (12, 15) == 0)) {
                DASM("smla/smul\n");

                /* ElSegundo SMLAWy/SMULWy insn.  */
                unsigned long long op1 = state->Reg[BITS (0, 3)];
                unsigned long long op2 = state->Reg[BITS (8, 11)];
                unsigned long long result;

                if (BIT (6))
                    op2 >>= 16;
                if (op1 & 0x80000000)
                    op1 -= 1ULL << 32;
                op2 &= 0xFFFF;
                if (op2 & 0x8000)
                    op2 -= 65536;
                result = (op1 * op2) >> 16;

                if (BIT (5) == 0) {
                    u32 Rn = state->Reg[BITS(12, 15)];

                    if (AddOverflow(result, Rn, result + Rn))
                        SETS;
                    result += Rn;
                }
                state->Reg[BITS (16, 19)] = result;
                break;
            }

            if (BITS (4, 11) == 5) {
                DASM("qsub\n");

                /* ElSegundo QSUB insn.  */
                u32 op1 = state->Reg[BITS (0, 3)];
                u32 op2 = state->Reg[BITS (16, 19)];
                u32 result = op1 - op2;

                if (SubOverflow(op1, op2, result)) {
                    result = POS (result) ? 0x80000000 : 0x7fffffff;
                    SETS;
                }

                state->Reg[BITS (12, 15)] = result;
                break;
            }

            if (BITS (4, 11) == 0xB) {
                DASM("strh\n");

                /* STRH register offset, write-back, down, pre indexed.  */
                SHPREDOWNWB ();
                break;
            }
            if (BITS (4, 27) == 0x12FFF1) {
                DASM("bx\n");

                /* BX */
                WriteR15Branch (state, state->Reg[RHSReg]);
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }
            if (DESTReg == 15) {
                DEBUG("Undefined MSR.\n");
                UNDEF_MRSPC;
                return 1;
                /*
                // MSR reg to CPSR.
                UNDEF_MSRPC;
                temp = DPRegRHS;
                // Don't allow TBIT to be set by MSR.
                temp &= ~TBIT;
                ARMul_FixCPSR (state, instr, temp);
                */
            } else {
                DASM("Undefined ???.\n");
                UNDEF_Test;
                return 1;
            }
            break;

        case 0x13:	/* TEQP reg */
            if ((BITS (4, 11) & 0xF9) == 0x9) {
                DASM("ldr\n");
                /* LDR register offset, write-back, down, pre indexed.  */
                LHPREDOWNWB ();
            }

            /* Continue with remaining instruction decode.  */
            if (DESTReg == 15) {
                /* TEQP reg */
                DASM("Undefined TEQP.\n");
                UNDEF_MRSPC;
                return 1;
            } else {
                DASM("teq\n");
                /* TEQ Reg.  */
                rhs = DPSRegRHS;
                dest = LHS ^ rhs;
                ARMul_NegZero (state, dest);
            }
            break;

        case 0x14:	/* CMP reg and MRS SPSR and SWP byte.  */
            if (BIT (4) == 0 && BIT (7) == 1) {
                DASM("smlal\n");

                /* ElSegundo SMLALxy insn.  */
                unsigned long long op1 = state-> Reg[BITS (0, 3)];
                unsigned long long op2 = state->Reg[BITS (8, 11)];
                unsigned long long dest;
                unsigned long long result;

                if (BIT (5))
                    op1 >>= 16;
                if (BIT (6))
                    op2 >>= 16;
                op1 &= 0xFFFF;
                if (op1 & 0x8000)
                    op1 -= 65536;
                op2 &= 0xFFFF;
                if (op2 & 0x8000)
                    op2 -= 65536;

                dest = (unsigned long long) state->Reg[BITS (16, 19)] << 32;
                dest |= state->Reg[BITS (12, 15)];
                dest += op1 * op2;
                state->Reg[BITS (12, 15)] = dest;
                state->Reg[BITS (16, 19)] = dest >> 32;
                break;
            }

            if (BITS (4, 11) == 5) {
                DASM("qdadd\n");

                /* ElSegundo QDADD insn.  */
                u32 op1 = state->Reg[BITS (0, 3)];
                u32 op2 = state->Reg[BITS (16, 19)];
                u32 op2d = op2 + op2;
                u32 result;

                if (AddOverflow(op2, op2, op2d)) {
                    SETS;
                    op2d = POS (op2d) ? 0x80000000 : 0x7fffffff;
                }

                result = op1 + op2d;
                if (AddOverflow(op1, op2d, result)) {
                    SETS;
                    result = POS (result) ? 0x80000000 : 0x7fffffff;
                }

                state->Reg[BITS (12, 15)] = result;
                break;
            }

            if (BITS (4, 7) == 0xB) {
                DASM("strh\n");

                /* STRH immediate offset, no write-back, down, pre indexed.  */
                SHPREDOWN ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }

            if (BITS (4, 11) == 9) {
                DASM("swp\n");

                /* SWP */
                UNDEF_SWPPC;
                temp = LHS;
                BUSUSEDINCPCS;
                DEST = ARMul_SwapByte (state, temp, state->Reg[RHSReg]);
            } else if ((BITS (0, 11) == 0) && (LHSReg == 15)) {
                /* MRS SPSR */
                DEBUG("Undefined MRS\n");
                UNDEF_MRSPC;
                return 1;
            } else {
                DEBUG("Undefined ???\n");
                UNDEF_Test;
                return 1;
            }
            break;

        case 0x15:	/* CMPP reg.  */
            if ((BITS (4, 7) & 0x9) == 0x9) {
                DASM("ldr\n");
                /* LDR immediate offset, no write-back, down, pre indexed.  */
                LHPREDOWN ();
            }

            /* Continue with remaining instruction decode.  */
            if (DESTReg == 15) {
                /* CMPP reg.  */
                DEBUG("Undefined CMPP\n");
                return 1;
            } else {
                DASM("cmp\n");

                /* CMP reg.  */
                lhs = LHS;
                rhs = DPRegRHS;
                dest = lhs - rhs;
                ARMul_NegZero (state, dest);
                if ((lhs >= rhs) || ((rhs | lhs) >> 31)) {
                    ARMul_SubCarry (state, lhs, rhs, dest);
                    ARMul_SubOverflow (state, lhs, rhs, dest);
                } else {
                    CLEARC;
                    CLEARV;
                }
            }
            break;

        case 0x16:	/* CMN reg and MSR reg to SPSR */
            if (BIT (4) == 0 && BIT (7) == 1 && BITS (12, 15) == 0) {
                DASM("smul\n");

                /* ElSegundo SMULxy insn.  */
                u32 op1 = state->Reg[BITS (0, 3)];
                u32 op2 = state->Reg[BITS (8, 11)];
                u32 Rn = state->Reg[BITS (12, 15)];

                if (BIT (5))
                    op1 >>= 16;
                if (BIT (6))
                    op2 >>= 16;
                op1 &= 0xFFFF;
                op2 &= 0xFFFF;
                if (op1 & 0x8000)
                    op1 -= 65536;
                if (op2 & 0x8000)
                    op2 -= 65536;

                state->Reg[BITS (16, 19)] = op1 * op2;
                break;
            }

            if (BITS (4, 11) == 5) {
                DASM("qdsub\n");

                /* ElSegundo QDSUB insn.  */
                u32 op1 = state->Reg[BITS (0, 3)];
                u32 op2 = state->Reg[BITS (16, 19)];
                u32 op2d = op2 + op2;
                u32 result;

                if (AddOverflow(op2, op2, op2d)) {
                    SETS;
                    op2d = POS (op2d) ? 0x80000000 : 0x7fffffff;
                }

                result = op1 - op2d;
                if (SubOverflow(op1, op2d, result)) {
                    SETS;
                    result = POS (result) ? 0x80000000 : 0x7fffffff;
                }

                state->Reg[BITS (12, 15)] = result;
                break;
            }
            if (BITS (4, 11) == 0xF1 && BITS (16, 19) == 0xF) {
                DASM("clz\n");

                /* ARM5 CLZ insn.  */
                u32 op1 = state->Reg[BITS (0, 3)];
                int result = 32;

                if (op1)
                    for (result = 0; (op1 & 0x80000000) == 0; op1 <<= 1)
                        result++;
                state->Reg[BITS (12, 15)] = result;
                break;
            }

            if (BITS (4, 7) == 0xB) {
                DASM("strh\n");

                /* STRH immediate offset, write-back, down, pre indexed.  */
                SHPREDOWNWB ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }

            if (DESTReg == 15) {
                /* MSR */
                DEBUG("Undefined MSR\n");
                UNDEF_MSRPC;
                /*ARMul_FixSPSR (state, instr, DPRegRHS);*/
                return 1;
            } else {
                DEBUG("Undefined ???\n");
                UNDEF_Test;
                return 1;
            }
            break;

        case 0x17:	/* CMNP reg */
            if ((BITS (4, 7) & 0x9) == 0x9) {
                DASM("ldr");
                /* LDR immediate offset, write-back, down, pre indexed.  */
                LHPREDOWNWB ();
            }

            /* Continue with remaining instruction decoding.  */
            if (DESTReg == 15) {
                DEBUG("Undefined cmpn\n");
                return 1;
            } else {
                DASM("cmn\n");

                /* CMN reg.  */
                lhs = LHS;
                rhs = DPRegRHS;
                dest = lhs + rhs;
                ASSIGNZ (dest == 0);

                if ((lhs | rhs) >> 30) {
                    /* Possible C,V,N to set.  */
                    ASSIGNN (NEG (dest));
                    ARMul_AddCarry (state, lhs, rhs, dest);
                    ARMul_AddOverflow (state, lhs, rhs, dest);
                } else {
                    CLEARN;
                    CLEARC;
                    CLEARV;
                }
            }
            break;

        case 0x18:	/* ORR reg */
            /* dyf add armv6 instr strex  2010.9.17 */
            if (BITS (4, 7) == 0x9)
                if (handle_v6_insn (state, instr))
                    break;

            if (BITS (4, 11) == 0xB) {
                DASM("strh\n");
                /* STRH register offset, no write-back, up, pre indexed.  */
                SHPREUP ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }

            DASM("orr\n");
            rhs = DPRegRHS;
            dest = LHS | rhs;
            WRITEDEST (dest);
            break;

        case 0x19:	/* ORRS reg */
            /* dyf add armv6 instr ldrex */
            if (BITS (4, 7) == 0x9) {
                if (handle_v6_insn (state, instr))
                    break;
            }
            if ((BITS (4, 11) & 0xF9) == 0x9) {
                DASM("ldr\n");
                /* LDR register offset, no write-back, up, pre indexed.  */
                LHPREUP ();
            }

            DASM("orrs");
            /* Continue with remaining instruction decoding.  */
            rhs = DPSRegRHS;
            dest = LHS | rhs;
            WRITESDEST (dest);
            break;

        case 0x1a:	/* MOV reg */
            if (BITS (4, 11) == 0xB) {
                DASM("strh\n");
                /* STRH register offset, write-back, up, pre indexed.  */
                SHPREUPWB ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }

            DASM("mov\n");
            dest = DPRegRHS;
            WRITEDEST (dest);
            break;

        case 0x1b:	/* MOVS reg */
            if ((BITS (4, 11) & 0xF9) == 0x9) {
                DASM("ldr\n");
                /* LDR register offset, write-back, up, pre indexed.  */
                LHPREUPWB ();
            }

            DASM("movs\n");
            /* Continue with remaining instruction decoding.  */
            dest = DPSRegRHS;
            WRITESDEST (dest);
            break;

        case 0x1c:	/* BIC reg */
            /* dyf add for STREXB */
            if (BITS (4, 7) == 0x9) {
                if (handle_v6_insn (state, instr))
                    break;
            }
            if (BITS (4, 7) == 0xB) {
                DASM("strh\n");
                /* STRH immediate offset, no write-back, up, pre indexed.  */
                SHPREUP ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            } else if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }

            DASM("bic\n");
            rhs = DPRegRHS;
            dest = LHS & ~rhs;
            WRITEDEST (dest);
            break;

        case 0x1d:	/* BICS reg */
            /* ladsh P=1 U=1 W=0 L=1 S=1 H=1 */
            if (BITS(4, 7) == 0xF) {
                DASM("ladsh\n");
                temp = LHS + GetLS7RHS (state, instr);
                LoadHalfWord (state, instr, temp, LSIGNED);
                break;

            }
            if (BITS (4, 7) == 0xb) {
                DASM("ldrh\n");
                /* LDRH immediate offset, no write-back, up, pre indexed.  */
                temp = LHS + GetLS7RHS (state, instr);
                LoadHalfWord (state, instr, temp, LUNSIGNED);
                break;
            }
            if (BITS (4, 7) == 0xd) {
                DASM("ldrsb\n");
                // alex-ykl fix: 2011-07-20 missing ldrsb instruction
                temp = LHS + GetLS7RHS (state, instr);
                LoadByte (state, instr, temp, LSIGNED);
                break;
            }

            /* Continue with instruction decoding.  */
            /*if ((BITS (4, 7) & 0x9) == 0x9) */
            if ((BITS (4, 7)) == 0x9) {
                /* ldrexb */
                if (handle_v6_insn (state, instr))
                    break;

                DASM("ldr\n");
                /* LDR immediate offset, no write-back, up, pre indexed.  */
                LHPREUP ();

            }

            DASM("bics\n");
            rhs = DPSRegRHS;
            dest = LHS & ~rhs;
            WRITESDEST (dest);
            break;

        case 0x1e:	/* MVN reg */
            if (BITS (4, 7) == 0xB) {
                DASM("strh\n");

                /* STRH immediate offset, write-back, up, pre indexed.  */
                SHPREUPWB ();
                break;
            }
            if (BITS (4, 7) == 0xD) {
                DASM("ldrd\n");
                Handle_Load_Double (state, instr);
                break;
            }
            if (BITS (4, 7) == 0xF) {
                DASM("strd\n");
                Handle_Store_Double (state, instr);
                break;
            }

            DASM("mvn\n");
            dest = ~DPRegRHS;
            WRITEDEST (dest);
            break;

        case 0x1f:	/* MVNS reg */
            if ((BITS (4, 7) & 0x9) == 0x9) {
                DASM("ldr\n");
                /* LDR immediate offset, write-back, up, pre indexed.  */
                LHPREUPWB ();
            }
            DASM("mvns\n");

            /* Continue instruction decoding.  */
            dest = ~DPSRegRHS;
            WRITESDEST (dest);
            break;


        /* Data Processing Immediate RHS Instructions.  */

        case 0x20:	/* AND immed */
            DASM("and\n");
            dest = LHS & DPImmRHS;
            WRITEDEST (dest);
            break;

        case 0x21:	/* ANDS immed */
            DASM("ands\n");
            DPSImmRHS;
            dest = LHS & rhs;
            WRITESDEST (dest);
            break;

        case 0x22:	/* EOR immed */
            DASM("eor\n");
            dest = LHS ^ DPImmRHS;
            WRITEDEST (dest);
            break;

        case 0x23:	/* EORS immed */
            DASM("eors\n");
            DPSImmRHS;
            dest = LHS ^ rhs;
            WRITESDEST (dest);
            break;

        case 0x24:	/* SUB immed */
            DASM("sub\n");
            dest = LHS - DPImmRHS;
            WRITEDEST (dest);
            break;

        case 0x25:	/* SUBS immed */
            DASM("subs\n");
            lhs = LHS;
            rhs = DPImmRHS;
            dest = lhs - rhs;

            if ((lhs >= rhs) || ((rhs | lhs) >> 31)) {
                ARMul_SubCarry (state, lhs, rhs, dest);
                ARMul_SubOverflow (state, lhs, rhs, dest);
            } else {
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x26:	/* RSB immed */
            DASM("rsb\n");
            dest = DPImmRHS - LHS;
            WRITEDEST (dest);
            break;

        case 0x27:	/* RSBS immed */
            DASM("rsbs\n");
            lhs = LHS;
            rhs = DPImmRHS;
            dest = rhs - lhs;

            if ((rhs >= lhs) || ((rhs | lhs) >> 31)) {
                ARMul_SubCarry (state, rhs, lhs, dest);
                ARMul_SubOverflow (state, rhs, lhs, dest);
            } else {
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x28:	/* ADD immed */
            DASM("add\n");
            dest = LHS + DPImmRHS;
            WRITEDEST (dest);
            break;

        case 0x29:	/* ADDS immed */
            DASM("adds\n");
            lhs = LHS;
            rhs = DPImmRHS;
            dest = lhs + rhs;
            ASSIGNZ (dest == 0);

            if ((lhs | rhs) >> 30) {
                /* Possible C,V,N to set.  */
                ASSIGNN (NEG (dest));
                ARMul_AddCarry (state, lhs, rhs, dest);
                ARMul_AddOverflow (state, lhs, rhs, dest);
            } else {
                CLEARN;
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x2a:	/* ADC immed */
            DASM("adc\n");
            dest = LHS + DPImmRHS + CFLAG;
            WRITEDEST (dest);
            break;

        case 0x2b:	/* ADCS immed */
            DASM("adcs\n");
            lhs = LHS;
            rhs = DPImmRHS;
            dest = lhs + rhs + CFLAG;
            ASSIGNZ (dest == 0);
            if ((lhs | rhs) >> 30) {
                /* Possible C,V,N to set.  */
                ASSIGNN (NEG (dest));
                ARMul_AddCarry (state, lhs, rhs, dest);
                ARMul_AddOverflow (state, lhs, rhs, dest);
            } else {
                CLEARN;
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x2c:	/* SBC immed */
            DASM("sbc\n");
            dest = LHS - DPImmRHS - !CFLAG;
            WRITEDEST (dest);
            break;

        case 0x2d:	/* SBCS immed */
            DASM("sbcs\n");
            lhs = LHS;
            rhs = DPImmRHS;
            dest = lhs - rhs - !CFLAG;
            if ((lhs >= rhs) || ((rhs | lhs) >> 31)) {
                ARMul_SubCarry (state, lhs, rhs, dest);
                ARMul_SubOverflow (state, lhs, rhs, dest);
            } else {
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x2e:	/* RSC immed */
            DASM("rsc\n");
            dest = DPImmRHS - LHS - !CFLAG;
            WRITEDEST (dest);
            break;

        case 0x2f:	/* RSCS immed */
            DASM("rscs\n");
            lhs = LHS;
            rhs = DPImmRHS;
            dest = rhs - lhs - !CFLAG;
            if ((rhs >= lhs) || ((rhs | lhs) >> 31)) {
                ARMul_SubCarry (state, rhs, lhs, dest);
                ARMul_SubOverflow (state, rhs, lhs, dest);
            } else {
                CLEARC;
                CLEARV;
            }
            WRITESDEST (dest);
            break;

        case 0x30:	/* TST immed */
            DASM("movw\n");
            DEBUG("DOUBLE CHECK THIS PLS?????\n");

            /* movw, ARMV6, ARMv7 */
            dest ^= dest;
            dest = BITS(16, 19);
            dest = ((dest<<12) | BITS(0, 11));
            WRITEDEST(dest);

        case 0x31:	/* TSTP immed */
            if (DESTReg == 15) {
                /* TSTP immed.  */
                DEBUG("TSTP undefined.\n");
                return 1;
            } else {
                DASM("tst\n");
                /* TST immed.  */
                DPSImmRHS;
                dest = LHS & rhs;
                ARMul_NegZero (state, dest);
            }
            break;

        case 0x32:	/* TEQ immed and MSR immed to CPSR */
            //if (DESTReg == 15)
            DEBUG("MSR undefined.\n");
            return 1;

        case 0x33:	/* TEQP immed */
            if (DESTReg == 15) {
                /* TEQP immed.  */
                DEBUG("TEQP undefined for R15.\n");
                return 1;
            } else {
                DASM("teq\n");
                DPSImmRHS;	/* TEQ immed */
                dest = LHS ^ rhs;
                ARMul_NegZero (state, dest);
            }
            break;

        case 0x34:	/* CMP immed */
            DEBUG("Undefined CMP??\n");
            UNDEF_Test;
            return 1;

        case 0x35:	/* CMPP immed */
            if (DESTReg == 15) {
                /* CMPP immed.  */
                DEBUG("CMPP undefined for R15.\n");
                return 1;
            } else {
                DASM("cmp\n");

                /* CMP immed.  */
                lhs = LHS;
                rhs = DPImmRHS;
                dest = lhs - rhs;
                ARMul_NegZero (state, dest);

                if ((lhs >= rhs) || ((rhs | lhs) >> 31)) {
                    ARMul_SubCarry (state, lhs, rhs, dest);
                    ARMul_SubOverflow (state, lhs, rhs, dest);
                } else {
                    CLEARC;
                    CLEARV;
                }
            }
            break;

        case 0x36:	/* CMN immed and MSR immed to SPSR */
            if (DESTReg == 15) {
                DEBUG("MSR undefined for R15.\n");
                return 1;
            } else {
                DEBUG("CMN not implemented???\n");
                UNDEF_Test;
                return 1;
            }
            break;

        case 0x37:	/* CMNP immed.  */
            if (DESTReg == 15) {
                /* CMNP immed.  */
                DEBUG("CMNP undefined for R15.\n");
                return 1;
            } else {
                DASM("cmn\n");

                /* CMN immed.  */
                lhs = LHS;
                rhs = DPImmRHS;
                dest = lhs + rhs;
                ASSIGNZ (dest == 0);

                if ((lhs | rhs) >> 30) {
                    /* Possible C,V,N to set.  */
                    ASSIGNN (NEG (dest));
                    ARMul_AddCarry (state, lhs, rhs, dest);
                    ARMul_AddOverflow (state, lhs, rhs, dest);
                } else {
                    CLEARN;
                    CLEARC;
                    CLEARV;
                }
            }
            break;

        case 0x38:	/* ORR immed.  */
            DASM("orr\n");
            dest = LHS | DPImmRHS;
            WRITEDEST (dest);
            break;

        case 0x39:	/* ORRS immed.  */
            DASM("orrs\n");
            DPSImmRHS;
            dest = LHS | rhs;
            WRITESDEST (dest);
            break;

        case 0x3a:	/* MOV immed.  */
            DASM("mov\n");
            dest = DPImmRHS;
            WRITEDEST (dest);
            break;

        case 0x3b:	/* MOVS immed.  */
            DASM("movs\n");
            DPSImmRHS;
            WRITESDEST (rhs);
            break;

        case 0x3c:	/* BIC immed.  */
            DASM("bic\n");
            dest = LHS & ~DPImmRHS;
            WRITEDEST (dest);
            break;

        case 0x3d:	/* BICS immed.  */
            DASM("bics\n");
            DPSImmRHS;
            dest = LHS & ~rhs;
            WRITESDEST (dest);
            break;

        case 0x3e:	/* MVN immed.  */
            DASM("mvn\n");
            dest = ~DPImmRHS;
            WRITEDEST (dest);
            break;

        case 0x3f:	/* MVNS immed.  */
            DASM("mvns\n");
            DPSImmRHS;
            WRITESDEST (~rhs);
            break;


        /* Single Data Transfer Immediate RHS Instructions.  */

        case 0x40:	/* Store Word, No WriteBack, Post Dec, Immed.  */
            DASM("str\n");
            lhs = LHS;
            if (StoreWord (state, instr, lhs))
                LSBase = lhs - LSImmRHS;
            break;

        case 0x41:	/* Load Word, No WriteBack, Post Dec, Immed.  */
            DASM("ldr\n");
            lhs = LHS;
            if (LoadWord (state, instr, lhs))
                LSBase = lhs - LSImmRHS;
            break;

        case 0x42:	/* Store Word, WriteBack, Post Dec, Immed.  */
            DASM("str\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            lhs = LHS;
            temp = lhs - LSImmRHS;
            if (StoreWord (state, instr, lhs))
                LSBase = temp;
            break;

        case 0x43:	/* Load Word, WriteBack, Post Dec, Immed.  */
            DASM("ldr\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            lhs = LHS;

            if (LoadWord (state, instr, lhs))
                LSBase = lhs - LSImmRHS;
            break;

        case 0x44:	/* Store Byte, No WriteBack, Post Dec, Immed.  */
            DASM("strb\n");
            lhs = LHS;
            if (StoreByte (state, instr, lhs))
                LSBase = lhs - LSImmRHS;
            break;

        case 0x45:	/* Load Byte, No WriteBack, Post Dec, Immed.  */
            DASM("ldrb\n");
            lhs = LHS;
            if (LoadByte (state, instr, lhs, LUNSIGNED))
                LSBase = lhs - LSImmRHS;
            break;

        case 0x46:	/* Store Byte, WriteBack, Post Dec, Immed.  */
            DASM("strb\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            lhs = LHS;
            if (StoreByte (state, instr, lhs))
                LSBase = lhs - LSImmRHS;
            break;

        case 0x47:	/* Load Byte, WriteBack, Post Dec, Immed.  */
            DASM("ldrb\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            lhs = LHS;

            if (LoadByte (state, instr, lhs, LUNSIGNED))
                LSBase = lhs - LSImmRHS;
            break;

        case 0x48:	/* Store Word, No WriteBack, Post Inc, Immed.  */
            DASM("str\n");
            lhs = LHS;
            if (StoreWord (state, instr, lhs))
                LSBase = lhs + LSImmRHS;
            break;

        case 0x49:	/* Load Word, No WriteBack, Post Inc, Immed.  */
            DASM("ldr\n");
            lhs = LHS;
            if (LoadWord (state, instr, lhs))
                LSBase = lhs + LSImmRHS;
            break;

        case 0x4a:	/* Store Word, WriteBack, Post Inc, Immed.  */
            DASM("str\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            lhs = LHS;

            if (StoreWord (state, instr, lhs))
                LSBase = lhs + LSImmRHS;
            break;

        case 0x4b:	/* Load Word, WriteBack, Post Inc, Immed.  */
            DASM("ldr\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            lhs = LHS;

            if (LoadWord (state, instr, lhs))
                LSBase = lhs + LSImmRHS;
            break;

        case 0x4c:	/* Store Byte, No WriteBack, Post Inc, Immed.  */
            DASM("strb\n");
            lhs = LHS;
            if (StoreByte (state, instr, lhs))
                LSBase = lhs + LSImmRHS;
            break;

        case 0x4d:	/* Load Byte, No WriteBack, Post Inc, Immed.  */
            DASM("ldrb\n");
            lhs = LHS;
            if (LoadByte (state, instr, lhs, LUNSIGNED))
                LSBase = lhs + LSImmRHS;
            break;

        case 0x4e:	/* Store Byte, WriteBack, Post Inc, Immed.  */
            DASM("strb\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            lhs = LHS;

            if (StoreByte (state, instr, lhs))
                LSBase = lhs + LSImmRHS;
            break;

        case 0x4f:	/* Load Byte, WriteBack, Post Inc, Immed.  */
            DASM("ldrb\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            lhs = LHS;

            if (LoadByte (state, instr, lhs, LUNSIGNED))
                LSBase = lhs + LSImmRHS;
            break;


        case 0x50:	/* Store Word, No WriteBack, Pre Dec, Immed.  */
            DASM("str\n");
            (void) StoreWord (state, instr, LHS - LSImmRHS);
            break;

        case 0x51:	/* Load Word, No WriteBack, Pre Dec, Immed.  */
            DASM("ldr\n");
            (void) LoadWord (state, instr, LHS - LSImmRHS);
            break;

        case 0x52:	/* Store Word, WriteBack, Pre Dec, Immed.  */
            DASM("str\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            temp = LHS - LSImmRHS;
            if (StoreWord (state, instr, temp))
                LSBase = temp;
            break;

        case 0x53:	/* Load Word, WriteBack, Pre Dec, Immed.  */
            DASM("ldr\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            temp = LHS - LSImmRHS;
            if (LoadWord (state, instr, temp))
                LSBase = temp;
            break;

        case 0x54:	/* Store Byte, No WriteBack, Pre Dec, Immed.  */
            DASM("strb\n");
            (void) StoreByte (state, instr,
                              LHS - LSImmRHS);
            break;

        case 0x55:	/* Load Byte, No WriteBack, Pre Dec, Immed.  */
            DASM("ldrb\n");
            (void) LoadByte (state, instr, LHS - LSImmRHS,
                             LUNSIGNED);
            break;

        case 0x56:	/* Store Byte, WriteBack, Pre Dec, Immed.  */
            DASM("strb\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            temp = LHS - LSImmRHS;
            if (StoreByte (state, instr, temp))
                LSBase = temp;
            break;

        case 0x57:	/* Load Byte, WriteBack, Pre Dec, Immed.  */
            DASM("ldrb\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            temp = LHS - LSImmRHS;
            if (LoadByte (state, instr, temp, LUNSIGNED))
                LSBase = temp;
            break;

        case 0x58:	/* Store Word, No WriteBack, Pre Inc, Immed.  */
            DASM("str\n");
            (void) StoreWord (state, instr,
                              LHS + LSImmRHS);
            break;

        case 0x59:	/* Load Word, No WriteBack, Pre Inc, Immed.  */
            DASM("ldr\n");
            (void) LoadWord (state, instr,
                             LHS + LSImmRHS);
            break;

        case 0x5a:	/* Store Word, WriteBack, Pre Inc, Immed.  */
            DASM("str\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            temp = LHS + LSImmRHS;
            if (StoreWord (state, instr, temp))
                LSBase = temp;
            break;

        case 0x5b:	/* Load Word, WriteBack, Pre Inc, Immed.  */
            DASM("ldr\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            temp = LHS + LSImmRHS;
            if (LoadWord (state, instr, temp))
                LSBase = temp;
            break;

        case 0x5c:	/* Store Byte, No WriteBack, Pre Inc, Immed.  */
            DASM("strb\n");
            (void) StoreByte (state, instr,
                              LHS + LSImmRHS);
            break;

        case 0x5d:	/* Load Byte, No WriteBack, Pre Inc, Immed.  */
            DASM("ldrb\n");
            (void) LoadByte (state, instr, LHS + LSImmRHS,
                             LUNSIGNED);
            break;

        case 0x5e:	/* Store Byte, WriteBack, Pre Inc, Immed.  */
            DASM("strb\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            temp = LHS + LSImmRHS;
            if (StoreByte (state, instr, temp))
                LSBase = temp;
            break;

        case 0x5f:	/* Load Byte, WriteBack, Pre Inc, Immed.  */
            DASM("ldrb\n");
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            temp = LHS + LSImmRHS;
            if (LoadByte (state, instr, temp, LUNSIGNED))
                LSBase = temp;
            break;


        /* Single Data Transfer Register RHS Instructions.  */

        case 0x60:	/* Store Word, No WriteBack, Post Dec, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("str\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            if (StoreWord (state, instr, lhs))
                LSBase = lhs - LSRegRHS;
            break;

        case 0x61:	/* Load Word, No WriteBack, Post Dec, Reg.  */
            if (BIT (4)) {
                if (handle_v6_insn (state, instr))
                    break;

                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldr\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            temp = lhs - LSRegRHS;
            if (LoadWord (state, instr, lhs))
                LSBase = temp;
            break;

        case 0x62:	/* Store Word, WriteBack, Post Dec, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn (state, instr))
                    break;

                ARMul_UndefInstr (state, instr);
                break;
            }

            DASM("str\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;

            if (StoreWord (state, instr, lhs))
                LSBase = lhs - LSRegRHS;
            break;

        case 0x63:	/* Load Word, WriteBack, Post Dec, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn (state, instr))
                    break;

                ARMul_UndefInstr (state, instr);
                break;
            }

            DASM("ldr\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            temp = lhs - LSRegRHS;

            if (LoadWord (state, instr, lhs))
                LSBase = temp;
            break;

        case 0x64:	/* Store Byte, No WriteBack, Post Dec, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("strb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            if (StoreByte (state, instr, lhs))
                LSBase = lhs - LSRegRHS;
            break;

        case 0x65:	/* Load Byte, No WriteBack, Post Dec, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldrb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            temp = lhs - LSRegRHS;
            if (LoadByte (state, instr, lhs, LUNSIGNED))
                LSBase = temp;
            break;

        case 0x66:	/* Store Byte, WriteBack, Post Dec, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("strb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;

            if (StoreByte (state, instr, lhs))
                LSBase = lhs - LSRegRHS;
            break;

        case 0x67:	/* Load Byte, WriteBack, Post Dec, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldrb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            temp = lhs - LSRegRHS;

            if (LoadByte (state, instr, lhs, LUNSIGNED))
                LSBase = temp;
            break;

        case 0x68:	/* Store Word, No WriteBack, Post Inc, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("str\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            if (StoreWord (state, instr, lhs))
                LSBase = lhs + LSRegRHS;
            break;

        case 0x69:	/* Load Word, No WriteBack, Post Inc, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldr\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            temp = lhs + LSRegRHS;
            if (LoadWord (state, instr, lhs))
                LSBase = temp;
            break;

        case 0x6a:	/* Store Word, WriteBack, Post Inc, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("str\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;

            if (StoreWord (state, instr, lhs))
                LSBase = lhs + LSRegRHS;
            break;

        case 0x6b:	/* Load Word, WriteBack, Post Inc, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldr\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            temp = lhs + LSRegRHS;

            if (LoadWord (state, instr, lhs))
                LSBase = temp;
            break;

        case 0x6c:	/* Store Byte, No WriteBack, Post Inc, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("strb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            if (StoreByte (state, instr, lhs))
                LSBase = lhs + LSRegRHS;
            break;

        case 0x6d:	/* Load Byte, No WriteBack, Post Inc, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldrb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            temp = lhs + LSRegRHS;
            if (LoadByte (state, instr, lhs, LUNSIGNED))
                LSBase = temp;
            break;

        case 0x6e:	/* Store Byte, WriteBack, Post Inc, Reg.  */
#if 0
            if (state->is_v6) {
                int Rm = 0;
                /* utxb */
                if (BITS(15, 19) == 0xf && BITS(4, 7) == 0x7) {

                    Rm = (RHS >> (8 * BITS(10, 11))) & 0xff;
                    DEST = Rm;
                }

            }
#endif
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("strb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;

            if (StoreByte (state, instr, lhs))
                LSBase = lhs + LSRegRHS;
            break;

        case 0x6f:	/* Load Byte, WriteBack, Post Inc, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldrb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            lhs = LHS;
            temp = lhs + LSRegRHS;

            if (LoadByte (state, instr, lhs, LUNSIGNED))
                LSBase = temp;
            break;


        case 0x70:	/* Store Word, No WriteBack, Pre Dec, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("str\n");
            (void) StoreWord (state, instr,
                              LHS - LSRegRHS);
            break;

        case 0x71:	/* Load Word, No WriteBack, Pre Dec, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldr\n");
            (void) LoadWord (state, instr,
                             LHS - LSRegRHS);
            break;

        case 0x72:	/* Store Word, WriteBack, Pre Dec, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("str\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            temp = LHS - LSRegRHS;
            if (StoreWord (state, instr, temp))
                LSBase = temp;
            break;

        case 0x73:	/* Load Word, WriteBack, Pre Dec, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldr\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            temp = LHS - LSRegRHS;
            if (LoadWord (state, instr, temp))
                LSBase = temp;
            break;

        case 0x74:	/* Store Byte, No WriteBack, Pre Dec, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("strb\n");
            (void) StoreByte (state, instr,
                              LHS - LSRegRHS);
            break;

        case 0x75:	/* Load Byte, No WriteBack, Pre Dec, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldrb\n");
            (void) LoadByte (state, instr, LHS - LSRegRHS,
                             LUNSIGNED);
            break;

        case 0x76:	/* Store Byte, WriteBack, Pre Dec, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("strb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            temp = LHS - LSRegRHS;
            if (StoreByte (state, instr, temp))
                LSBase = temp;
            break;

        case 0x77:	/* Load Byte, WriteBack, Pre Dec, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldrb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            temp = LHS - LSRegRHS;
            if (LoadByte (state, instr, temp, LUNSIGNED))
                LSBase = temp;
            break;

        case 0x78:	/* Store Word, No WriteBack, Pre Inc, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("str\n");
            (void) StoreWord (state, instr,
                              LHS + LSRegRHS);
            break;

        case 0x79:	/* Load Word, No WriteBack, Pre Inc, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldr\n");
            (void) LoadWord (state, instr,
                             LHS + LSRegRHS);
            break;

        case 0x7a:	/* Store Word, WriteBack, Pre Inc, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("str\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            temp = LHS + LSRegRHS;
            if (StoreWord (state, instr, temp))
                LSBase = temp;
            break;

        case 0x7b:	/* Load Word, WriteBack, Pre Inc, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldr\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            temp = LHS + LSRegRHS;
            if (LoadWord (state, instr, temp))
                LSBase = temp;
            break;

        case 0x7c:	/* Store Byte, No WriteBack, Pre Inc, Reg.  */
            if (BIT (4)) {
                if(handle_v6_insn(state, instr))
                    break;
                DEBUG("Undefined.\n");
                return 1;
            }
            DASM("strb\n");
            (void) StoreByte (state, instr, LHS + LSRegRHS);
            break;

        case 0x7d:	/* Load Byte, No WriteBack, Pre Inc, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("ldrb\n");
            (void) LoadByte (state, instr, LHS + LSRegRHS,
                             LUNSIGNED);
            break;

        case 0x7e:	/* Store Byte, WriteBack, Pre Inc, Reg.  */
            if (BIT (4)) {
                DEBUG("Undefined.\n");
                return 1;
            }

            DASM("strb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            temp = LHS + LSRegRHS;
            if (StoreByte (state, instr, temp))
                LSBase = temp;
            break;

        case 0x7f:	/* Load Byte, WriteBack, Pre Inc, Reg.  */
            if (BIT (4)) {
                /* Check for the special breakpoint opcode.
                   This value should correspond to the value defined
                   as ARM_BE_BREAKPOINT in gdb/arm/tm-arm.h.  */
                // PLUTO: TODO
                /*
                  if (BITS (0, 19) == 0xfdefe) {
                  if (!ARMul_OSHandleSWI
                  (state, SWI_Breakpoint))
                  ARMul_Abort (state,
                  ARMul_SWIV);
                  }
                  else*/
                DEBUG("Undef instruction???");
                return 1;
            }

            DASM("ldrb\n");
            UNDEF_LSRBaseEQOffWb;
            UNDEF_LSRBaseEQDestWb;
            UNDEF_LSRPCBaseWb;
            UNDEF_LSRPCOffWb;
            temp = LHS + LSRegRHS;
            if (LoadByte (state, instr, temp, LUNSIGNED))
                LSBase = temp;
            break;


        /* Multiple Data Transfer Instructions.  */

        case 0x80:	/* Store, No WriteBack, Post Dec.  */
            STOREMULT (instr, LSBase - LSMNumRegs + 4L,
                       0L);
            DASM("storemult\n");
            break;

        case 0x81:	/* Load, No WriteBack, Post Dec.  */
            LOADMULT (instr, LSBase - LSMNumRegs + 4L,
                      0L);
            DASM("loadmult\n");
            break;

        case 0x82:	/* Store, WriteBack, Post Dec.  */
            temp = LSBase - LSMNumRegs;
            STOREMULT (instr, temp + 4L, temp);
            DASM("storemult\n");
            break;

        case 0x83:	/* Load, WriteBack, Post Dec.  */
            temp = LSBase - LSMNumRegs;
            LOADMULT (instr, temp + 4L, temp);
            DASM("loadmult\n");
            break;

        case 0x84:	/* Store, Flags, No WriteBack, Post Dec.  */
            STORESMULT (instr, LSBase - LSMNumRegs + 4L,
                        0L);
            DASM("storemult\n");
            break;

        case 0x85:	/* Load, Flags, No WriteBack, Post Dec.  */
            LOADSMULT (instr, LSBase - LSMNumRegs + 4L,
                       0L);
            DASM("loadmult\n");
            break;

        case 0x86:	/* Store, Flags, WriteBack, Post Dec.  */
            temp = LSBase - LSMNumRegs;
            STORESMULT (instr, temp + 4L, temp);
            DASM("storemult\n");
            break;

        case 0x87:	/* Load, Flags, WriteBack, Post Dec.  */
            temp = LSBase - LSMNumRegs;
            LOADSMULT (instr, temp + 4L, temp);
            DASM("loadmult\n");
            break;

        case 0x88:	/* Store, No WriteBack, Post Inc.  */
            STOREMULT (instr, LSBase, 0L);
            DASM("storemult\n");
            break;

        case 0x89:	/* Load, No WriteBack, Post Inc.  */
            LOADMULT (instr, LSBase, 0L);
            DASM("loadmult\n");
            break;

        case 0x8a:	/* Store, WriteBack, Post Inc.  */
            temp = LSBase;
            STOREMULT (instr, temp, temp + LSMNumRegs);
            DASM("storemult\n");
            break;

        case 0x8b:	/* Load, WriteBack, Post Inc.  */
            temp = LSBase;
            LOADMULT (instr, temp, temp + LSMNumRegs);
            DASM("loadmult\n");
            break;

        case 0x8c:	/* Store, Flags, No WriteBack, Post Inc.  */
            STORESMULT (instr, LSBase, 0L);
            DASM("storemult\n");
            break;

        case 0x8d:	/* Load, Flags, No WriteBack, Post Inc.  */
            LOADSMULT (instr, LSBase, 0L);
            DASM("loadmult\n");
            break;

        case 0x8e:	/* Store, Flags, WriteBack, Post Inc.  */
            temp = LSBase;
            STORESMULT (instr, temp, temp + LSMNumRegs);
            DASM("storemult\n");
            break;

        case 0x8f:	/* Load, Flags, WriteBack, Post Inc.  */
            temp = LSBase;
            LOADSMULT (instr, temp, temp + LSMNumRegs);
            DASM("loadmult\n");
            break;

        case 0x90:	/* Store, No WriteBack, Pre Dec.  */
            STOREMULT (instr, LSBase - LSMNumRegs, 0L);
            DASM("storemult\n");
            break;

        case 0x91:	/* Load, No WriteBack, Pre Dec.  */
            LOADMULT (instr, LSBase - LSMNumRegs, 0L);
            DASM("loadmult\n");
            break;

        case 0x92:	/* Store, WriteBack, Pre Dec.  */
            temp = LSBase - LSMNumRegs;
            STOREMULT (instr, temp, temp);
            DASM("storemult\n");
            break;

        case 0x93:	/* Load, WriteBack, Pre Dec.  */
            temp = LSBase - LSMNumRegs;
            LOADMULT (instr, temp, temp);
            DASM("loadmult\n");
            break;

        case 0x94:	/* Store, Flags, No WriteBack, Pre Dec.  */
            STORESMULT (instr, LSBase - LSMNumRegs, 0L);
            DASM("storemult\n");
            break;

        case 0x95:	/* Load, Flags, No WriteBack, Pre Dec.  */
            LOADSMULT (instr, LSBase - LSMNumRegs, 0L);
            DASM("loadmult\n");
            break;

        case 0x96:	/* Store, Flags, WriteBack, Pre Dec.  */
            temp = LSBase - LSMNumRegs;
            STORESMULT (instr, temp, temp);
            DASM("storemult\n");
            break;

        case 0x97:	/* Load, Flags, WriteBack, Pre Dec.  */
            temp = LSBase - LSMNumRegs;
            LOADSMULT (instr, temp, temp);
            DASM("loadmult\n");
            break;

        case 0x98:	/* Store, No WriteBack, Pre Inc.  */
            STOREMULT (instr, LSBase + 4L, 0L);
            DASM("storemult\n");
            break;

        case 0x99:	/* Load, No WriteBack, Pre Inc.  */
            LOADMULT (instr, LSBase + 4L, 0L);
            DASM("loadmult\n");
            break;

        case 0x9a:	/* Store, WriteBack, Pre Inc.  */
            temp = LSBase;
            STOREMULT (instr, temp + 4L,
                       temp + LSMNumRegs);
            DASM("storemult\n");
            break;

        case 0x9b:	/* Load, WriteBack, Pre Inc.  */
            temp = LSBase;
            LOADMULT (instr, temp + 4L,
                      temp + LSMNumRegs);
            DASM("loadmult\n");
            break;

        case 0x9c:	/* Store, Flags, No WriteBack, Pre Inc.  */
            STORESMULT (instr, LSBase + 4L, 0L);
            DASM("storemult\n");
            break;

        case 0x9d:	/* Load, Flags, No WriteBack, Pre Inc.  */
            LOADSMULT (instr, LSBase + 4L, 0L);
            DASM("loadmult\n");
            break;

        case 0x9e:	/* Store, Flags, WriteBack, Pre Inc.  */
            temp = LSBase;
            STORESMULT (instr, temp + 4L,
                        temp + LSMNumRegs);
            DASM("storemult\n");
            break;

        case 0x9f:	/* Load, Flags, WriteBack, Pre Inc.  */
            temp = LSBase;
            LOADSMULT (instr, temp + 4L,
                       temp + LSMNumRegs);
            DASM("loadmult\n");
            break;


        /* Branch forward.  */
        case 0xa0:
        case 0xa1:
        case 0xa2:
        case 0xa3:
        case 0xa4:
        case 0xa5:
        case 0xa6:
        case 0xa7:
            DASM("b\n");
            state->Reg[15] = pc + 4 + POSBRANCH;
            break;


        /* Branch backward.  */
        case 0xa8:
        case 0xa9:
        case 0xaa:
        case 0xab:
        case 0xac:
        case 0xad:
        case 0xae:
        case 0xaf:
            DASM("b\n");
            state->Reg[15] = pc + 4 + NEGBRANCH;
            break;


        /* Branch and Link forward.  */
        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xb5:
        case 0xb6:
        case 0xb7:
            DASM("bl\n");
            /* Put PC into Link.  */
            state->Reg[14] = pc;
            state->Reg[15] = pc + 4 + POSBRANCH;
            break;


        /* Branch and Link backward.  */
        case 0xb8:
        case 0xb9:
        case 0xba:
        case 0xbb:
        case 0xbc:
        case 0xbd:
        case 0xbe:
        case 0xbf:
            DASM("bl\n");
            /* Put PC into Link.  */
            state->Reg[14] = pc;
            state->Reg[15] = pc + 4 + NEGBRANCH;
            break;


        /* Co-Processor Data Transfers.  */
        case 0xc4:
            DASM("mcrr not implemented\n");
            return 1;
            /* MCRR, ARMv5TE and up */
            ARMul_MCRR (state, instr, DEST, state->Reg[LHSReg]);
            break;
        /* Drop through.  */

        case 0xc0:	/* Store , No WriteBack , Post Dec.  */
            DASM("stc\n");
            ARMul_STC (state, instr, LHS);
            break;

        case 0xc5:
            DEBUG("mrrc not implemented\n");
            return 1;
            /* MRRC, ARMv5TE and up */
            ARMul_MRRC (state, instr, &DEST, &(state->Reg[LHSReg]));
            break;
        /* Drop through.  */

        case 0xc1:	/* Load , No WriteBack , Post Dec.  */
            DASM("ldc\n");
            ARMul_LDC (state, instr, LHS);
            break;

        case 0xc2:
        case 0xc6:	/* Store , WriteBack , Post Dec.  */
            DASM("stc\n");
            lhs = LHS;
            state->Base = lhs - LSCOff;
            ARMul_STC (state, instr, lhs);
            break;

        case 0xc3:
        case 0xc7:	/* Load , WriteBack , Post Dec.  */
            DASM("ldc\n");
            lhs = LHS;
            state->Base = lhs - LSCOff;
            ARMul_LDC (state, instr, lhs);
            break;

        case 0xc8:
        case 0xcc:	/* Store , No WriteBack , Post Inc.  */
            DASM("stc\n");
            ARMul_STC (state, instr, LHS);
            break;

        case 0xc9:
        case 0xcd:	/* Load , No WriteBack , Post Inc.  */
            DASM("ldc\n");
            ARMul_LDC (state, instr, LHS);
            break;

        case 0xca:
        case 0xce:	/* Store , WriteBack , Post Inc.  */
            DASM("stc\n");
            lhs = LHS;
            state->Base = lhs + LSCOff;
            ARMul_STC (state, instr, LHS);
            break;

        case 0xcb:
        case 0xcf:	/* Load , WriteBack , Post Inc.  */
            DASM("ldc\n");
            lhs = LHS;
            state->Base = lhs + LSCOff;
            ARMul_LDC (state, instr, LHS);
            break;

        case 0xd0:
        case 0xd4:	/* Store , No WriteBack , Pre Dec.  */
            DASM("stc\n");
            ARMul_STC (state, instr, LHS - LSCOff);
            break;

        case 0xd1:
        case 0xd5:	/* Load , No WriteBack , Pre Dec.  */
            DASM("ldc\n");
            ARMul_LDC (state, instr, LHS - LSCOff);
            break;

        case 0xd2:
        case 0xd6:	/* Store , WriteBack , Pre Dec.  */
            DASM("stc\n");
            lhs = LHS - LSCOff;
            state->Base = lhs;
            ARMul_STC (state, instr, lhs);
            break;

        case 0xd3:
        case 0xd7:	/* Load , WriteBack , Pre Dec.  */
            DASM("ldc\n");
            lhs = LHS - LSCOff;
            state->Base = lhs;
            ARMul_LDC (state, instr, lhs);
            break;

        case 0xd8:
        case 0xdc:	/* Store , No WriteBack , Pre Inc.  */
            DASM("stc\n");
            ARMul_STC (state, instr, LHS + LSCOff);
            break;

        case 0xd9:
        case 0xdd:	/* Load , No WriteBack , Pre Inc.  */
            DASM("ldc\n");
            ARMul_LDC (state, instr, LHS + LSCOff);
            break;

        case 0xda:
        case 0xde:	/* Store , WriteBack , Pre Inc.  */
            DASM("stc\n");
            lhs = LHS + LSCOff;
            state->Base = lhs;
            ARMul_STC (state, instr, lhs);
            break;

        case 0xdb:
        case 0xdf:	/* Load , WriteBack , Pre Inc.  */
            DASM("ldc\n");
            lhs = LHS + LSCOff;
            state->Base = lhs;
            ARMul_LDC (state, instr, lhs);
            break;


        /* Co-Processor Register Transfers (MCR) and Data Ops.  */

        case 0xe2:
        case 0xe0:
        case 0xe4:
        case 0xe6:
        case 0xe8:
        case 0xea:
        case 0xec:
        case 0xee:
            if (BIT (4)) {
                DEBUG("Unimplemnted MCR.\n");
                return 1;
                /* MCR.  */
                if (DESTReg == 15) {
                    UNDEF_MCRPC;
#ifdef MODE32
                    ARMul_MCR (state, instr,
                               state->Reg[15] +
                               isize);
#else
                    ARMul_MCR (state, instr,
                               ECC | ER15INT |
                               EMODE |
                               ((state->Reg[15] +
                                 isize) &
                                R15PCBITS));
#endif
                } else
                    ARMul_MCR (state, instr,
                               DEST);
            } else {
                DASM("cdp\n");
                /* CDP Part 1.  */
                ARMul_CDP (state, instr);
            }
            break;


        /* Co-Processor Register Transfers (MRC) and Data Ops.  */
        case 0xe1:
        case 0xe3:
        case 0xe5:
        case 0xe7:
        case 0xe9:
        case 0xeb:
        case 0xed:
        case 0xef:
            if (BIT (4)) {
                DEBUG("Unimplemnted MRC.\n");
                return 1;
                /* MRC */
                temp = ARMul_MRC (state, instr);
                if (DESTReg == 15) {
                    ASSIGNN ((temp & NBIT) != 0);
                    ASSIGNZ ((temp & ZBIT) != 0);
                    ASSIGNC ((temp & CBIT) != 0);
                    ASSIGNV ((temp & VBIT) != 0);
                } else
                    DEST = temp;
            } else
                DASM("cdp\n");
            /* CDP Part 2.  */
            ARMul_CDP (state, instr);
            break;


        /* SWI instruction.  */
        case 0xf0:
        case 0xf1:
        case 0xf2:
        case 0xf3:
        case 0xf4:
        case 0xf5:
        case 0xf6:
        case 0xf7:
        case 0xf8:
        case 0xf9:
        case 0xfa:
        case 0xfb:
        case 0xfc:
        case 0xfd:
        case 0xfe:
        case 0xff:
            DASM("swi\n");
            // PLUTO: TODO, send "state" var as arg also
            svc_Execute(BITS (0, 23));
            break;
        }
    }

do_next:
    return 0;
}

static volatile void (*gen_func) (void);
static volatile uint32_t tmp_st;
static volatile uint32_t save_st;
static volatile uint32_t save_T0;
static volatile uint32_t save_T1;
static volatile uint32_t save_T2;

/* This routine evaluates most Data Processing register RHS's with the S
   bit clear.  It is intended to be called from the macro DPRegRHS, which
   filters the common case of an unshifted register with in line code.  */

static u32
GetDPRegRHS (ARMul_State * state, u32 instr)
{
    u32 shamt, base;

    base = RHSReg;
    if (BIT (4)) {
        /* Shift amount in a register.  */
        UNDEF_Shift;
        INCPC;
#ifndef MODE32
        if (base == 15)
            base = ECC | ER15INT | R15PC | EMODE;
        else
#endif
            base = state->Reg[base];

        shamt = state->Reg[BITS (8, 11)] & 0xff;
        switch ((int) BITS (5, 6)) {
        case LSL:
            if (shamt == 0)
                return (base);
            else if (shamt >= 32)
                return (0);
            else
                return (base << shamt);
        case LSR:
            if (shamt == 0)
                return (base);
            else if (shamt >= 32)
                return (0);
            else
                return (base >> shamt);
        case ASR:
            if (shamt == 0)
                return (base);
            else if (shamt >= 32)
                return ((u32) ((int) base >> 31L));
            else
                return ((u32)
                        (( int) base >> (int) shamt));
        case ROR:
            shamt &= 0x1f;
            if (shamt == 0)
                return (base);
            else
                return ((base << (32 - shamt)) |
                        (base >> shamt));
        }
    } else {
        /* Shift amount is a constant.  */
#ifndef MODE32
        if (base == 15)
            base = ECC | ER15INT | R15PC | EMODE;
        else
#endif
            base = state->Reg[base];
        shamt = BITS (7, 11);
        switch ((int) BITS (5, 6)) {
        case LSL:
            return (base << shamt);
        case LSR:
            if (shamt == 0)
                return (0);
            else
                return (base >> shamt);
        case ASR:
            if (shamt == 0)
                return ((u32) (( int) base >> 31L));
            else
                return ((u32)
                        (( int) base >> (int) shamt));
        case ROR:
            if (shamt == 0)
                /* It's an RRX.  */
                return ((base >> 1) | (CFLAG << 31));
            else
                return ((base << (32 - shamt)) |
                        (base >> shamt));
        }
    }

    return 0;
}

/* This routine evaluates most Logical Data Processing register RHS's
   with the S bit set.  It is intended to be called from the macro
   DPSRegRHS, which filters the common case of an unshifted register
   with in line code.  */

static u32
GetDPSRegRHS (ARMul_State * state, u32 instr)
{
    u32 shamt, base;

    base = RHSReg;
    if (BIT (4)) {
        /* Shift amount in a register.  */
        UNDEF_Shift;
        INCPC;
#ifndef MODE32
        if (base == 15)
            base = ECC | ER15INT | R15PC | EMODE;
        else
#endif
            base = state->Reg[base];

        shamt = state->Reg[BITS (8, 11)] & 0xff;
        switch ((int) BITS (5, 6)) {
        case LSL:
            if (shamt == 0)
                return (base);
            else if (shamt == 32) {
                ASSIGNC (base & 1);
                return (0);
            } else if (shamt > 32) {
                CLEARC;
                return (0);
            } else {
                ASSIGNC ((base >> (32 - shamt)) & 1);
                return (base << shamt);
            }
        case LSR:
            if (shamt == 0)
                return (base);
            else if (shamt == 32) {
                ASSIGNC (base >> 31);
                return (0);
            } else if (shamt > 32) {
                CLEARC;
                return (0);
            } else {
                ASSIGNC ((base >> (shamt - 1)) & 1);
                return (base >> shamt);
            }
        case ASR:
            if (shamt == 0)
                return (base);
            else if (shamt >= 32) {
                ASSIGNC (base >> 31L);
                return ((u32) (( int) base >> 31L));
            } else {
                ASSIGNC ((u32)
                         (( int) base >>
                          (int) (shamt - 1)) & 1);
                return ((u32)
                        ((int) base >> (int) shamt));
            }
        case ROR:
            if (shamt == 0)
                return (base);
            shamt &= 0x1f;
            if (shamt == 0) {
                ASSIGNC (base >> 31);
                return (base);
            } else {
                ASSIGNC ((base >> (shamt - 1)) & 1);
                return ((base << (32 - shamt)) |
                        (base >> shamt));
            }
        }
    } else {
        /* Shift amount is a constant.  */
#ifndef MODE32
        if (base == 15)
            base = ECC | ER15INT | R15PC | EMODE;
        else
#endif
            base = state->Reg[base];
        shamt = BITS (7, 11);

        switch ((int) BITS (5, 6)) {
        case LSL:
            ASSIGNC ((base >> (32 - shamt)) & 1);
            return (base << shamt);
        case LSR:
            if (shamt == 0) {
                ASSIGNC (base >> 31);
                return (0);
            } else {
                ASSIGNC ((base >> (shamt - 1)) & 1);
                return (base >> shamt);
            }
        case ASR:
            if (shamt == 0) {
                ASSIGNC (base >> 31L);
                return ((u32) ((int) base >> 31L));
            } else {
                ASSIGNC ((u32)
                         ((int) base >>
                          (int) (shamt - 1)) & 1);
                return ((u32)
                        (( int) base >> (int) shamt));
            }
        case ROR:
            if (shamt == 0) {
                /* It's an RRX.  */
                shamt = CFLAG;
                ASSIGNC (base & 1);
                return ((base >> 1) | (shamt << 31));
            } else {
                ASSIGNC ((base >> (shamt - 1)) & 1);
                return ((base << (32 - shamt)) |
                        (base >> shamt));
            }
        }
    }

    return 0;
}

/* This routine handles writes to register 15 when the S bit is not set.  */

static void
WriteR15 (ARMul_State * state, u32 src)
{
    /* The ARM documentation states that the two least significant bits
       are discarded when setting PC, except in the cases handled by
       WriteR15Branch() below.  It's probably an oversight: in THUMB
       mode, the second least significant bit should probably not be
       discarded.  */
    if (TFLAG)
        src &= 0xfffffffe;
    else
        src &= 0xfffffffc;

    state->Reg[15] = src & PCBITS;
}

/* This routine handles writes to register 15 when the S bit is set.  */

static void
WriteSR15 (ARMul_State * state, u32 src)
{
    if (TFLAG)
        src &= 0xfffffffe;
    else
        src &= 0xfffffffc;

    state->Reg[15] = src & PCBITS;
}

/* In machines capable of running in Thumb mode, BX, BLX, LDR and LDM
   will switch to Thumb mode if the least significant bit is set.  */

static void
WriteR15Branch (ARMul_State * state, u32 src)
{
    if (src & 1) {
        /* Thumb bit.  */
        SETT;
        state->Reg[15] = src & 0xfffffffe;
    } else {
        CLEART;
        state->Reg[15] = src & 0xfffffffc;
    }
}

/* This routine evaluates most Load and Store register RHS's.  It is
   intended to be called from the macro LSRegRHS, which filters the
   common case of an unshifted register with in line code.  */

static u32
GetLSRegRHS (ARMul_State * state, u32 instr)
{
    u32 shamt, base;

    base = RHSReg;
#ifndef MODE32
    if (base == 15)
        /* Now forbidden, but ...  */
        base = ECC | ER15INT | R15PC | EMODE;
    else
#endif
        base = state->Reg[base];

    shamt = BITS (7, 11);
    switch ((int) BITS (5, 6)) {
    case LSL:
        return (base << shamt);
    case LSR:
        if (shamt == 0)
            return (0);
        else
            return (base >> shamt);
    case ASR:
        if (shamt == 0)
            return ((u32) (( int) base >> 31L));
        else
            return ((u32) (( int) base >> (int) shamt));
    case ROR:
        if (shamt == 0)
            /* It's an RRX.  */
            return ((base >> 1) | (CFLAG << 31));
        else
            return ((base << (32 - shamt)) | (base >> shamt));
    default:
        break;
    }
    return 0;
}

/* This routine evaluates the ARM7T halfword and signed transfer RHS's.  */

static u32
GetLS7RHS (ARMul_State * state, u32 instr)
{
    if (BIT (22) == 0) {
        /* Register.  */
#ifndef MODE32
        if (RHSReg == 15)
            /* Now forbidden, but ...  */
            return ECC | ER15INT | R15PC | EMODE;
#endif
        return state->Reg[RHSReg];
    }

    /* Immediate.  */
    return BITS (0, 3) | (BITS (8, 11) << 4);
}

/* This function does the work of loading a word for a LDR instruction.  */
#define MEM_LOAD_LOG(description) 0
#define MEM_STORE_LOG(description) 0


static unsigned
LoadWord (ARMul_State * state, u32 instr, u32 address)
{
    u32 dest;

    BUSUSEDINCPCS;
#ifndef MODE32
    if (ADDREXCEPT (address))
        INTERNALABORT (address);
#endif

    dest = ARMul_LoadWordN (state, address + 8);

    if (address & 3)
        dest = ARMul_Align (state, address + 8, dest);

    WRITEDESTB (dest);
    return (DESTReg != LHSReg);
}

#ifdef MODET
/* This function does the work of loading a halfword.  */

static unsigned
LoadHalfWord (ARMul_State * state, u32 instr, u32 address,
              int signextend)
{
    u32 dest;

    BUSUSEDINCPCS;

    dest = ARMul_LoadHalfWord (state, address);
    if (state->Aborted) {
        TAKEABORT;
        return state->lateabtSig;
    }
    UNDEF_LSRBPC;
    if (signextend)
        if (dest & 1 << (16 - 1))
            dest = (dest & ((1 << 16) - 1)) - (1 << 16);

    WRITEDEST (dest);

    return (DESTReg != LHSReg);
}

#endif /* MODET */

/* This function does the work of loading a byte for a LDRB instruction.  */

static unsigned
LoadByte (ARMul_State * state, u32 instr, u32 address, int signextend)
{
    u32 dest;

    BUSUSEDINCPCS;

    dest = ARMul_LoadByte (state, address);
    if (state->Aborted) {
        TAKEABORT;
        return state->lateabtSig;
    }
    UNDEF_LSRBPC;
    if (signextend)
        if (dest & 1 << (8 - 1))
            dest = (dest & ((1 << 8) - 1)) - (1 << 8);

    WRITEDEST (dest);
    return (DESTReg != LHSReg);
}

/* This function does the work of loading two words for a LDRD instruction.  */

static void
Handle_Load_Double (ARMul_State * state, u32 instr)
{
    u32 dest_reg;
    u32 addr_reg;
    u32 write_back = BIT (21);
    u32 immediate = BIT (22);
    u32 add_to_base = BIT (23);
    u32 pre_indexed = BIT (24);
    u32 offset;
    u32 addr;
    u32 sum;
    u32 base;
    u32 value1;
    u32 value2;

    BUSUSEDINCPCS;

    /* If the writeback bit is set, the pre-index bit must be clear.  */
    if (write_back && !pre_indexed) {
        ARMul_UndefInstr (state, instr);
        return;
    }

    /* Extract the base address register.  */
    addr_reg = LHSReg;

    /* Extract the destination register and check it.  */
    dest_reg = DESTReg;

    /* Destination register must be even.  */
    if ((dest_reg & 1)
            /* Destination register cannot be LR.  */
            || (dest_reg == 14)) {
        ARMul_UndefInstr (state, instr);
        return;
    }

    /* Compute the base address.  */
    base = state->Reg[addr_reg];

    /* Compute the offset.  */
    offset = immediate ? ((BITS (8, 11) << 4) | BITS (0, 3)) : state->
             Reg[RHSReg];

    /* Compute the sum of the two.  */
    if (add_to_base)
        sum = base + offset;
    else
        sum = base - offset;

    /* If this is a pre-indexed mode use the sum.  */
    if (pre_indexed)
        addr = sum;
    else
        addr = base;

    /* The address must be aligned on a 8 byte boundary.  */
    if (addr & 0x7) {
#ifdef ABORTS
        ARMul_DATAABORT (addr);
#else
        ARMul_UndefInstr (state, instr);
#endif
        return;
    }

    /* For pre indexed or post indexed addressing modes,
       check that the destination registers do not overlap
       the address registers.  */
    if ((!pre_indexed || write_back)
            && (addr_reg == dest_reg || addr_reg == dest_reg + 1)) {
        ARMul_UndefInstr (state, instr);
        return;
    }

    /* Load the words.  */
    value1 = ARMul_LoadWordN (state, addr);
    value2 = ARMul_LoadWordN (state, addr + 4);

    /* Check for data aborts.  */
    if (state->Aborted) {
        TAKEABORT;
        return;
    }

    /* Store the values.  */
    state->Reg[dest_reg] = value1;
    state->Reg[dest_reg + 1] = value2;

    /* Do the post addressing and writeback.  */
    if (!pre_indexed)
        addr = sum;

    if (!pre_indexed || write_back)
        state->Reg[addr_reg] = addr;
}

/* This function does the work of storing two words for a STRD instruction.  */

static void
Handle_Store_Double (ARMul_State * state, u32 instr)
{
    u32 src_reg;
    u32 addr_reg;
    u32 write_back = BIT (21);
    u32 immediate = BIT (22);
    u32 add_to_base = BIT (23);
    u32 pre_indexed = BIT (24);
    u32 offset;
    u32 addr;
    u32 sum;
    u32 base;

    BUSUSEDINCPCS;

    /* If the writeback bit is set, the pre-index bit must be clear.  */
    if (write_back && !pre_indexed) {
        ARMul_UndefInstr (state, instr);
        return;
    }

    /* Extract the base address register.  */
    addr_reg = LHSReg;

    /* Base register cannot be PC.  */
    if (addr_reg == 15) {
        ARMul_UndefInstr (state, instr);
        return;
    }

    /* Extract the source register.  */
    src_reg = DESTReg;

    /* Source register must be even.  */
    if (src_reg & 1) {
        ARMul_UndefInstr (state, instr);
        return;
    }

    /* Compute the base address.  */
    base = state->Reg[addr_reg];

    /* Compute the offset.  */
    offset = immediate ? ((BITS (8, 11) << 4) | BITS (0, 3)) : state->
             Reg[RHSReg];

    /* Compute the sum of the two.  */
    if (add_to_base)
        sum = base + offset;
    else
        sum = base - offset;

    /* If this is a pre-indexed mode use the sum.  */
    if (pre_indexed)
        addr = sum;
    else
        addr = base;

    /* The address must be aligned on a 8 byte boundary.  */
    if (addr & 0x7) {
#ifdef ABORTS
        ARMul_DATAABORT (addr);
#else
        ARMul_UndefInstr (state, instr);
#endif
        return;
    }

    /* For pre indexed or post indexed addressing modes,
       check that the destination registers do not overlap
       the address registers.  */
    if ((!pre_indexed || write_back)
            && (addr_reg == src_reg || addr_reg == src_reg + 1)) {
        ARMul_UndefInstr (state, instr);
        return;
    }

    /* Load the words.  */
    ARMul_StoreWordN (state, addr, state->Reg[src_reg]);
    ARMul_StoreWordN (state, addr + 4, state->Reg[src_reg + 1]);

    if (state->Aborted) {
        TAKEABORT;
        return;
    }

    /* Do the post addressing and writeback.  */
    if (!pre_indexed)
        addr = sum;

    if (!pre_indexed || write_back)
        state->Reg[addr_reg] = addr;
}

/* This function does the work of storing a word from a STR instruction.  */

static unsigned
StoreWord (ARMul_State * state, u32 instr, u32 address)
{
    //MEM_STORE_LOG("WORD");

    BUSUSEDINCPCN;
#ifndef MODE32
    if (DESTReg == 15)
        state->Reg[15] = ECC | ER15INT | R15PC | EMODE;
#endif
#ifdef MODE32
    ARMul_StoreWordN (state, address, DEST);
#else
    if (VECTORACCESS (address) || ADDREXCEPT (address)) {
        INTERNALABORT (address);
        (void) ARMul_LoadWordN (state, address);
    } else
        ARMul_StoreWordN (state, address, DEST);
#endif
    if (state->Aborted) {
        TAKEABORT;
        return state->lateabtSig;
    }

    return TRUE;
}

#ifdef MODET
/* This function does the work of storing a byte for a STRH instruction.  */

static unsigned
StoreHalfWord (ARMul_State * state, u32 instr, u32 address)
{
    //MEM_STORE_LOG("HALFWORD");

    BUSUSEDINCPCN;

#ifndef MODE32
    if (DESTReg == 15)
        state->Reg[15] = ECC | ER15INT | R15PC | EMODE;
#endif

#ifdef MODE32
    ARMul_StoreHalfWord (state, address, DEST);
#else
    if (VECTORACCESS (address) || ADDREXCEPT (address)) {
        INTERNALABORT (address);
        (void) ARMul_LoadHalfWord (state, address);
    } else
        ARMul_StoreHalfWord (state, address, DEST);
#endif

    if (state->Aborted) {
        TAKEABORT;
        return state->lateabtSig;
    }
    return TRUE;
}

#endif /* MODET */

/* This function does the work of storing a byte for a STRB instruction.  */

static unsigned
StoreByte (ARMul_State * state, u32 instr, u32 address)
{
    //MEM_STORE_LOG("BYTE");

    BUSUSEDINCPCN;
#ifndef MODE32
    if (DESTReg == 15)
        state->Reg[15] = ECC | ER15INT | R15PC | EMODE;
#endif
#ifdef MODE32
    ARMul_StoreByte (state, address, DEST);
#else
    if (VECTORACCESS (address) || ADDREXCEPT (address)) {
        INTERNALABORT (address);
        (void) ARMul_LoadByte (state, address);
    } else
        ARMul_StoreByte (state, address, DEST);
#endif
    if (state->Aborted) {
        TAKEABORT;
        return state->lateabtSig;
    }
    //UNDEF_LSRBPC;
    return TRUE;
}

/* This function does the work of loading the registers listed in an LDM
   instruction, when the S bit is clear.  The code here is always increment
   after, it's up to the caller to get the input address correct and to
   handle base register modification.  */

static void
LoadMult (ARMul_State * state, u32 instr, u32 address, u32 WBBase)
{
    u32 dest, temp;

    //UNDEF_LSMNoRegs;
    //UNDEF_LSMPCBase;
    //UNDEF_LSMBaseInListWb;
    BUSUSEDINCPCS;

    /* N cycle first.  */
    for (temp = 0; !BIT (temp); temp++);

    dest = ARMul_LoadWordN (state, address);

    state->Reg[temp++] = dest;

    /* S cycles from here on.  */
    for (; temp < 16; temp++)
        if (BIT (temp)) {
            /* Load this register.  */
            address += 4;
            dest = ARMul_LoadWordS (state, address);

            state->Reg[temp] = dest;
        }

    if (BIT (15) && !state->Aborted)
        /* PC is in the reg list.  */
        WriteR15Branch (state, PC);
}

/* This function does the work of loading the registers listed in an LDM
   instruction, when the S bit is set. The code here is always increment
   after, it's up to the caller to get the input address correct and to
   handle base register modification.  */

static void
LoadSMult (ARMul_State * state,
           u32 instr, u32 address, u32 WBBase)
{
    u32 dest, temp;

    //UNDEF_LSMNoRegs;
    //UNDEF_LSMPCBase;
    //UNDEF_LSMBaseInListWb;

    BUSUSEDINCPCS;

    /* N cycle first.  */
    for (temp = 0; !BIT (temp); temp++);

    dest = ARMul_LoadWordN (state, address);

    state->Reg[temp++] = dest;

    /* S cycles from here on.  */
    for (; temp < 16; temp++)
        if (BIT (temp)) {
            /* Load this register.  */
            address += 4;
            dest = ARMul_LoadWordS (state, address);

            state->Reg[temp] = dest;
        }

    if (BIT (21) && LHSReg != 15)
        LSBase = WBBase;

    if (BIT (15)) {
        /* PC is in the reg list.  */
        //chy 2006-02-16 , should not consider system mode, don't conside 26bit mode
        WriteR15 (state, PC);
    }
}

/* This function does the work of storing the registers listed in an STM
   instruction, when the S bit is clear.  The code here is always increment
   after, it's up to the caller to get the input address correct and to
   handle base register modification.  */

static void
StoreMult (ARMul_State * state,
           u32 instr, u32 address, u32 WBBase)
{
    u32 temp;

    UNDEF_LSMNoRegs;
    UNDEF_LSMPCBase;
    UNDEF_LSMBaseInListWb;

    if (!TFLAG)
        /* N-cycle, increment the PC and update the NextInstr state.  */
        BUSUSEDINCPCN;

    if (BIT (15))
        PATCHR15;

    /* N cycle first.  */
    for (temp = 0; !BIT (temp); temp++);

    ARMul_StoreWordN (state, address, state->Reg[temp++]);

    /* S cycles from here on.  */
    for (; temp < 16; temp++)
        if (BIT (temp)) {
            /* Save this register.  */
            address += 4;

            ARMul_StoreWordS (state, address, state->Reg[temp]);
        }

    if (BIT (21) && LHSReg != 15) {
        LSBase = WBBase;
    }
}

/* This function does the work of storing the registers listed in an STM
   instruction when the S bit is set.  The code here is always increment
   after, it's up to the caller to get the input address correct and to
   handle base register modification.  */

static void
StoreSMult (ARMul_State * state,
            u32 instr, u32 address, u32 WBBase)
{
    u32 temp;

    UNDEF_LSMNoRegs;
    UNDEF_LSMPCBase;
    UNDEF_LSMBaseInListWb;

    BUSUSEDINCPCN;

    if (BIT (15))
        PATCHR15;

    for (temp = 0; !BIT (temp); temp++);	/* N cycle first.  */

    ARMul_StoreWordN (state, address, state->Reg[temp++]);

    /* S cycles from here on.  */
    for (; temp < 16; temp++)
        if (BIT (temp)) {
            /* Save this register.  */
            address += 4;

            ARMul_StoreWordS (state, address, state->Reg[temp]);
        }

    if (BIT (21) && LHSReg != 15) {
        LSBase = WBBase;
    }
}

/* This function does the work of adding two 32bit values
   together, and calculating if a carry has occurred.  */

static u32
Add32 (u32 a1, u32 a2, int *carry)
{
    u32 result = (a1 + a2);
    unsigned int uresult = (unsigned int) result;
    unsigned int ua1 = (unsigned int) a1;

    /* If (result == RdLo) and (state->Reg[nRdLo] == 0),
       or (result > RdLo) then we have no carry.  */
    if ((uresult == ua1) ? (a2 != 0) : (uresult < ua1))
        *carry = 1;
    else
        *carry = 0;

    return result;
}

/* This function does the work of multiplying
   two 32bit values to give a 64bit result.  */

static unsigned
Multiply64 (ARMul_State * state, u32 instr, int msigned, int scc)
{
    /* Operand register numbers.  */
    int nRdHi, nRdLo, nRs, nRm;
    u32 RdHi = 0, RdLo = 0, Rm;
    /* Cycle count.  */
    int scount;

    nRdHi = BITS (16, 19);
    nRdLo = BITS (12, 15);
    nRs = BITS (8, 11);
    nRm = BITS (0, 3);

    /* Needed to calculate the cycle count.  */
    Rm = state->Reg[nRm];

    /* Check for illegal operand combinations first.  */
    if (nRdHi != 15
            && nRdLo != 15
            && nRs != 15
            //&& nRm != 15 && nRdHi != nRdLo && nRdHi != nRm && nRdLo != nRm) {
            && nRm != 15 && nRdHi != nRdLo ) {
        /* Intermediate results.  */
        u32 lo, mid1, mid2, hi;
        int carry;
        u32 Rs = state->Reg[nRs];
        int sign = 0;

        if (msigned) {
            /* Compute sign of result and adjust operands if necessary.  */
            sign = (Rm ^ Rs) & 0x80000000;

            if (((signed int) Rm) < 0)
                Rm = -Rm;

            if (((signed int) Rs) < 0)
                Rs = -Rs;
        }

        /* We can split the 32x32 into four 16x16 operations. This
           ensures that we do not lose precision on 32bit only hosts.  */
        lo = ((Rs & 0xFFFF) * (Rm & 0xFFFF));
        mid1 = ((Rs & 0xFFFF) * ((Rm >> 16) & 0xFFFF));
        mid2 = (((Rs >> 16) & 0xFFFF) * (Rm & 0xFFFF));
        hi = (((Rs >> 16) & 0xFFFF) * ((Rm >> 16) & 0xFFFF));

        /* We now need to add all of these results together, taking
           care to propogate the carries from the additions.  */
        RdLo = Add32 (lo, (mid1 << 16), &carry);
        RdHi = carry;
        RdLo = Add32 (RdLo, (mid2 << 16), &carry);
        RdHi += (carry + ((mid1 >> 16) & 0xFFFF) +
                 ((mid2 >> 16) & 0xFFFF) + hi);

        if (sign) {
            /* Negate result if necessary.  */
            RdLo = ~RdLo;
            RdHi = ~RdHi;
            if (RdLo == 0xFFFFFFFF) {
                RdLo = 0;
                RdHi += 1;
            } else
                RdLo += 1;
        }

        state->Reg[nRdLo] = RdLo;
        state->Reg[nRdHi] = RdHi;
    } else {
        fprintf (stderr, "sim: MULTIPLY64 - INVALID ARGUMENTS, instr=0x%x\n", instr);
    }
    if (scc)
        /* Ensure that both RdHi and RdLo are used to compute Z,
           but don't let RdLo's sign bit make it to N.  */
        ARMul_NegZero (state, RdHi | (RdLo >> 16) | (RdLo & 0xFFFF));

    /* The cycle count depends on whether the instruction is a signed or
       unsigned multiply, and what bits are clear in the multiplier.  */
    if (msigned && (Rm & ((unsigned) 1 << 31)))
        /* Invert the bits to make the check against zero.  */
        Rm = ~Rm;

    if ((Rm & 0xFFFFFF00) == 0)
        scount = 1;
    else if ((Rm & 0xFFFF0000) == 0)
        scount = 2;
    else if ((Rm & 0xFF000000) == 0)
        scount = 3;
    else
        scount = 4;

    return 2 + scount;
}

/* This function does the work of multiplying two 32bit
   values and adding a 64bit value to give a 64bit result.  */

static unsigned
MultiplyAdd64 (ARMul_State * state, u32 instr, int msigned, int scc)
{
    unsigned scount;
    u32 RdLo, RdHi;
    int nRdHi, nRdLo;
    int carry = 0;

    nRdHi = BITS (16, 19);
    nRdLo = BITS (12, 15);

    RdHi = state->Reg[nRdHi];
    RdLo = state->Reg[nRdLo];

    scount = Multiply64 (state, instr, msigned, LDEFAULT);

    RdLo = Add32 (RdLo, state->Reg[nRdLo], &carry);
    RdHi = (RdHi + state->Reg[nRdHi]) + carry;

    state->Reg[nRdLo] = RdLo;
    state->Reg[nRdHi] = RdHi;

    if (scc)
        /* Ensure that both RdHi and RdLo are used to compute Z,
           but don't let RdLo's sign bit make it to N.  */
        ARMul_NegZero (state, RdHi | (RdLo >> 16) | (RdLo & 0xFFFF));

    /* Extra cycle for addition.  */
    return scount + 1;
}

/* Attempt to emulate an ARMv6 instruction.
   Returns non-zero upon success.  */

static int
handle_v6_insn (ARMul_State * state, u32 instr)
{
    switch (BITS (20, 27)) {
#if 0
    case 0x03:
        printf ("Unhandled v6 insn: ldr\n");
        break;
    case 0x04:
        printf ("Unhandled v6 insn: umaal\n");
        break;
    case 0x06:
        printf ("Unhandled v6 insn: mls/str\n");
        break;
    case 0x16:
        printf ("Unhandled v6 insn: smi\n");
        break;
    case 0x18:
        printf ("Unhandled v6 insn: strex\n");
        break;
    case 0x19:
        printf ("Unhandled v6 insn: ldrex\n");
        break;
    case 0x1a:
        printf ("Unhandled v6 insn: strexd\n");
        break;
    case 0x1b:
        printf ("Unhandled v6 insn: ldrexd\n");
        break;
    case 0x1c:
        printf ("Unhandled v6 insn: strexb\n");
        break;
    case 0x1d:
        printf ("Unhandled v6 insn: ldrexb\n");
        break;
    case 0x1e:
        printf ("Unhandled v6 insn: strexh\n");
        break;
    case 0x1f:
        printf ("Unhandled v6 insn: ldrexh\n");
        break;
    case 0x30:
        printf ("Unhandled v6 insn: movw\n");
        break;
    case 0x32:
        printf ("Unhandled v6 insn: nop/sev/wfe/wfi/yield\n");
        break;
    case 0x34:
        printf ("Unhandled v6 insn: movt\n");
        break;
    case 0x3f:
        printf ("Unhandled v6 insn: rbit\n");
        break;
    case 0x61:
        printf ("Unhandled v6 insn: sadd/ssub\n");
        break;
    case 0x62:
        printf ("Unhandled v6 insn: qadd/qsub\n");
        break;
    case 0x63:
        printf ("Unhandled v6 insn: shadd/shsub\n");
        break;
    case 0x65:
        printf ("Unhandled v6 insn: uadd/usub\n");
        break;
    case 0x66:
        printf ("Unhandled v6 insn: uqadd/uqsub\n");
        break;
    case 0x67:
        printf ("Unhandled v6 insn: uhadd/uhsub\n");
        break;
    case 0x68:
        printf ("Unhandled v6 insn: pkh/sxtab/selsxtb\n");
        break;
#endif
    case 0x6c:
        printf ("Unhandled v6 insn: uxtb16/uxtab16\n");
        break;
    case 0x70:
        printf ("Unhandled v6 insn: smuad/smusd/smlad/smlsd\n");
        break;
    case 0x74:
        printf ("Unhandled v6 insn: smlald/smlsld\n");
        break;
    case 0x75:
        printf ("Unhandled v6 insn: smmla/smmls/smmul\n");
        break;
    case 0x78:
        printf ("Unhandled v6 insn: usad/usada8\n");
        break;
#if 0
    case 0x7a:
        printf ("Unhandled v6 insn: usbfx\n");
        break;
    case 0x7c:
        printf ("Unhandled v6 insn: bfc/bfi\n");
        break;
#endif


        /* add new instr for arm v6. */
        u32 lhs, temp;
    case 0x18: {	/* ORR reg */
        /* dyf add armv6 instr strex  2010.9.17 */
        if (BITS (4, 7) == 0x9) {
            lhs = LHS;
            ARMul_StoreWordS(state, lhs, RHS);
            //StoreWord(state, lhs, RHS)
            if (state->Aborted) {
                TAKEABORT;
            }

            return 1;
        }
        break;
    }

    case 0x19: {	/* orrs reg */
        /* dyf add armv6 instr ldrex  */
        if (BITS (4, 7) == 0x9) {
            lhs = LHS;
            LoadWord (state, instr, lhs);
            return 1;
        }
        break;
    }

    case 0x1c: {	/* BIC reg */
        /* dyf add for STREXB */
        if (BITS (4, 7) == 0x9) {
            lhs = LHS;
            ARMul_StoreByte (state, lhs, RHS);
            BUSUSEDINCPCN;
            if (state->Aborted) {
                TAKEABORT;
            }

            //printf("In %s, strexb not implemented\n", __FUNCTION__);
            UNDEF_LSRBPC;
            /* WRITESDEST (dest); */
            return 1;
        }
        break;
    }

    case 0x1d: {	/* BICS reg */
        if ((BITS (4, 7)) == 0x9) {
            /* ldrexb */
            temp = LHS;
            LoadByte (state, instr, temp, LUNSIGNED);
            //state->Reg[BITS(12, 15)] = ARMul_LoadByte(state, state->Reg[BITS(16, 19)]);
            //printf("ldrexb\n");
            //printf("instr is %x rm is %d\n", instr, BITS(16, 19));
            //exit(-1);

            //printf("In %s, ldrexb not implemented\n", __FUNCTION__);
            return 1;
        }
        break;
    }
    /* add end */

    case 0x6a: {
        u32 Rm;
        int ror = -1;

        switch (BITS (4, 11)) {
        case 0x07:
            ror = 0;
            break;
        case 0x47:
            ror = 8;
            break;
        case 0x87:
            ror = 16;
            break;
        case 0xc7:
            ror = 24;
            break;

        case 0x01:
        case 0xf3:
            printf ("Unhandled v6 insn: ssat\n");
            return 0;
        default:
            break;
        }

        if (ror == -1) {
            if (BITS (4, 6) == 0x7) {
                printf ("Unhandled v6 insn: ssat\n");
                return 0;
            }
            break;
        }

        Rm = ((state->Reg[BITS (0, 3)] >> ror) & 0xFF);
        if (Rm & 0x80)
            Rm |= 0xffffff00;

        if (BITS (16, 19) == 0xf)
            /* SXTB */
            state->Reg[BITS (12, 15)] = Rm;
        else
            /* SXTAB */
            state->Reg[BITS (12, 15)] += Rm;
    }
    return 1;

    case 0x6b: {
        u32 Rm;
        int ror = -1;

        switch (BITS (4, 11)) {
        case 0x07:
            ror = 0;
            break;
        case 0x47:
            ror = 8;
            break;
        case 0x87:
            ror = 16;
            break;
        case 0xc7:
            ror = 24;
            break;

        case 0xf3:
            DEST = ((RHS & 0xFF) << 24) | ((RHS & 0xFF00)) << 8 | ((RHS & 0xFF0000) >> 8) | ((RHS & 0xFF000000) >> 24);
            return 1;
        case 0xfb:
            DEST = ((RHS & 0xFF) << 8) | ((RHS & 0xFF00)) >> 8 | ((RHS & 0xFF0000) << 8) | ((RHS & 0xFF000000) >> 8);
            return 1;
        default:
            break;
        }

        if (ror == -1)
            break;

        Rm = ((state->Reg[BITS (0, 3)] >> ror) & 0xFFFF);
        if (Rm & 0x8000)
            Rm |= 0xffff0000;

        if (BITS (16, 19) == 0xf)
            /* SXTH */
            state->Reg[BITS (12, 15)] = Rm;
        else
            /* SXTAH */
            state->Reg[BITS (12, 15)] = state->Reg[BITS (16, 19)] + Rm;
    }
    return 1;

    case 0x6e: {
        u32 Rm;
        int ror = -1;

        switch (BITS (4, 11)) {
        case 0x07:
            ror = 0;
            break;
        case 0x47:
            ror = 8;
            break;
        case 0x87:
            ror = 16;
            break;
        case 0xc7:
            ror = 24;
            break;

        case 0x01:
        case 0xf3:
            printf ("Unhandled v6 insn: usat\n");
            return 0;
        default:
            break;
        }

        if (ror == -1) {
            if (BITS (4, 6) == 0x7) {
                printf ("Unhandled v6 insn: usat\n");
                return 0;
            }
            break;
        }

        Rm = ((state->Reg[BITS (0, 3)] >> ror) & 0xFF);

        if (BITS (16, 19) == 0xf)
            /* UXTB */
            state->Reg[BITS (12, 15)] = Rm;
        else
            /* UXTAB */
            state->Reg[BITS (12, 15)] = state->Reg[BITS (16, 19)] + Rm;
    }
    return 1;

    case 0x6f: {
        u32 Rm;
        int ror = -1;

        switch (BITS (4, 11)) {
        case 0x07:
            ror = 0;
            break;
        case 0x47:
            ror = 8;
            break;
        case 0x87:
            ror = 16;
            break;
        case 0xc7:
            ror = 24;
            break;

        case 0xfb:
            printf ("Unhandled v6 insn: revsh\n");
            return 0;
        default:
            break;
        }

        if (ror == -1)
            break;

        Rm = ((state->Reg[BITS (0, 3)] >> ror) & 0xFFFF);

        /* UXT */
        /* state->Reg[BITS (12, 15)] = Rm; */
        /* dyf add */
        if (BITS (16, 19) == 0xf) {
            state->Reg[BITS (12, 15)] = (Rm >> (8 * BITS(10, 11))) & 0x0000FFFF;
        } else {
            /* UXTAH */
            /* state->Reg[BITS (12, 15)] = state->Reg [BITS (16, 19)] + Rm; */
            //            printf("rd is %x rn is %x rm is %x rotate is %x\n", state->Reg[BITS (12, 15)], state->Reg[BITS (16, 19)]
            //                   , Rm, BITS(10, 11));
            //            printf("icounter is %lld\n", state->NumInstrs);
            state->Reg[BITS (12, 15)] = (state->Reg[BITS (16, 19)] >> (8 * (BITS(10, 11)))) + Rm;
            //        printf("rd is %x\n", state->Reg[BITS (12, 15)]);
            //        exit(-1);
        }
    }
    return 1;

#if 0
    case 0x84:
        printf ("Unhandled v6 insn: srs\n");
        break;
#endif
    default:
        break;
    }
    printf ("Unhandled v6 insn: UNKNOWN: %08x\n", instr);
    return 0;
}
