/*  armdefs.h -- ARMulator common definitions:  ARM6 Instruction Emulator.
    Copyright (C) 1994 Advanced RISC Machines Ltd.
 
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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

#ifndef _ARMDEFS_H_
#define _ARMDEFS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <fcntl.h>
#include <signal.h>

#include <util.h>

#include "arm_regformat.h"

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#define LOW 0
#define HIGH 1
#define LOWHIGH 1
#define HIGHLOW 2

#define ARM_BYTE_TYPE 		0
#define ARM_HALFWORD_TYPE 	1
#define ARM_WORD_TYPE 		2

typedef struct ARMul_State ARMul_State;

typedef unsigned ARMul_CPInits (ARMul_State * state);
typedef unsigned ARMul_CPExits (ARMul_State * state);
typedef unsigned ARMul_LDCs (ARMul_State * state, unsigned type,
			     u32 instr, u32 value);
typedef unsigned ARMul_STCs (ARMul_State * state, unsigned type,
			     u32 instr, u32 * value);
typedef unsigned ARMul_MRCs (ARMul_State * state, unsigned type,
			     u32 instr, u32 * value);
typedef unsigned ARMul_MCRs (ARMul_State * state, unsigned type,
			     u32 instr, u32 value);
typedef unsigned ARMul_MRRCs (ARMul_State * state, unsigned type,
			     u32 instr, u32 * value1, u32 * value2);
typedef unsigned ARMul_MCRRs (ARMul_State * state, unsigned type,
			     u32 instr, u32 value1, u32 value2);
typedef unsigned ARMul_CDPs (ARMul_State * state, unsigned type,
			     u32 instr);
typedef unsigned ARMul_CPReads (ARMul_State * state, unsigned reg,
				u32 * value);
typedef unsigned ARMul_CPWrites (ARMul_State * state, unsigned reg,
				 u32 value);

#define VFP_REG_NUM 64
struct ARMul_State
{
	u32 Emulate;	/* to start and stop emulation */
	unsigned EndCondition;	/* reason for stopping */
	unsigned ErrorCode;	/* type of illegal instruction */

	u32 Reg[16]; /* the current register file */
	u32 Cpsr;
	u32 VFP[3];  /* FPSID, FPSCR, and FPEXC */
	/* VFPv2 and VFPv3-D16 has 16 doubleword registers (D0-D16 or S0-S31).
	   VFPv3-D32/ASIMD may have up to 32 doubleword registers (D0-D31),
		and only 32 singleword registers are accessible (S0-S31). */
	u32 ExtReg[VFP_REG_NUM];
	
	u32 NFlag, ZFlag, CFlag, VFlag, IFFlags;
	/* add armv6 flags dyf:2010-08-09 */
	u32 GEFlag, EFlag, AFlag, QFlags;
        u32 SFlag;

	/* Thumb state */
	u32 TFlag;

	u32 instr, pc, temp;	/* saved register state */
	u32 loaded, decoded;	/* saved pipeline state */
	//chy 2006-04-12 for ICE breakpoint
	u32 loaded_addr, decoded_addr;	/* saved pipeline state addr*/

	ARMul_CPInits *CPInit[16];	/* coprocessor initialisers */
	ARMul_CPExits *CPExit[16];	/* coprocessor finalisers */
	ARMul_LDCs *LDC[16];	/* LDC instruction */
	ARMul_STCs *STC[16];	/* STC instruction */
	ARMul_MRCs *MRC[16];	/* MRC instruction */
	ARMul_MCRs *MCR[16];	/* MCR instruction */
	ARMul_MRRCs *MRRC[16];	/* MRRC instruction */
	ARMul_MCRRs *MCRR[16];	/* MCRR instruction */
	ARMul_CDPs *CDP[16];	/* CDP instruction */
	ARMul_CPReads *CPRead[16];	/* Read CP register */
	ARMul_CPWrites *CPWrite[16];	/* Write CP register */
	unsigned char *CPData[16];	/* Coprocessor data */
	unsigned char const *CPRegWords[16];	/* map of coprocessor register sizes */

	unsigned Debug;		/* show instructions as they are executed */
	unsigned NresetSig;	/* reset the processor */
	unsigned NfiqSig;
	unsigned NirqSig;

	unsigned abortSig;
	unsigned NtransSig;
	unsigned bigendSig;
	unsigned prog32Sig;
	unsigned data32Sig;
	unsigned syscallSig;

/* 2004-05-09 chy
----------------------------------------------------------
read ARM Architecture Reference Manual
2.6.5 Data Abort
There are three Abort Model in ARM arch.

Early Abort Model: used in some ARMv3 and earlier implementations. In this
model, base register wirteback occurred for LDC,LDM,STC,STM instructions, and
the base register was unchanged for all other instructions. (oldest)

Base Restored Abort Model: If a Data Abort occurs in an instruction which
specifies base register writeback, the value in the base register is
unchanged. (strongarm, xscale)

Base Updated Abort Model: If a Data Abort occurs in an instruction which
specifies base register writeback, the base register writeback still occurs.
(arm720T)

read PART B
chap2 The System Control Coprocessor  CP15
2.4 Register1:control register
L(bit 6): in some ARMv3 and earlier implementations, the abort model of the
processor could be configured:
0=early Abort Model Selected(now obsolete)
1=Late Abort Model selceted(same as Base Updated Abort Model)

on later processors, this bit reads as 1 and ignores writes.
-------------------------------------------------------------
So, if lateabtSig=1, then it means Late Abort Model(Base Updated Abort Model)
    if lateabtSig=0, then it means Base Restored Abort Model
*/
	unsigned lateabtSig;

	u32 Vector;		/* synthesize aborts in cycle modes */
	u32 Aborted;	/* sticky flag for aborts */
	u32 Reseted;	/* sticky flag for Reset */
	u32 Inted, LastInted;	/* sticky flags for interrupts */
	u32 Base;		/* extra hand for base writeback */
	u32 AbortAddr;	/* to keep track of Prefetch aborts */

	int verbose;		/* non-zero means print various messages like the banner */

	uint32_t step;
	uint32_t cycle;
	int stop_simulator;

	uint32_t CurrInstr;
	uint32_t last_pc; /* the last pc executed */
	uint32_t last_instr; /* the last inst executed */
	uint32_t WriteAddr[17];
	uint32_t WriteData[17];
	uint32_t WritePc[17];
	uint32_t CurrWrite;
};

typedef ARMul_State arm_core_t;

/***************************************************************************\
*                   Macros to extract instruction fields                    *
\***************************************************************************/

#define BIT(n) ( (u32)(instr>>(n))&1)	/* bit n of instruction */
#define BITS(m,n) ( (u32)(instr<<(31-(n))) >> ((31-(n))+(m)) )	/* bits m to n of instr */
#define TOPBITS(n) (instr >> (n))	/* bits 31 to n of instr */

/***************************************************************************\
*                  Definitons of things in the emulator                     *
\***************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
extern void ARMul_EmulateInit (void);
extern void ARMul_Reset (ARMul_State * state);
#ifdef __cplusplus
	}
#endif
extern ARMul_State *ARMul_NewState (ARMul_State * state);
extern u32 ARMul_DoProg (ARMul_State * state);
extern u32 ARMul_DoInstr (ARMul_State * state);

/***************************************************************************\
*                          Useful support routines                          *
\***************************************************************************/

extern u32 ARMul_GetReg (ARMul_State * state, unsigned mode,
			     unsigned reg);
extern void ARMul_SetReg (ARMul_State * state, unsigned mode, unsigned reg,
			  u32 value);
extern u32 ARMul_GetPC (ARMul_State * state);
extern u32 ARMul_GetNextPC (ARMul_State * state);
extern void ARMul_SetPC (ARMul_State * state, u32 value);
extern u32 ARMul_GetR15 (ARMul_State * state);
extern void ARMul_SetR15 (ARMul_State * state, u32 value);

extern u32 ARMul_GetCPSR (ARMul_State * state);
extern void ARMul_SetCPSR (ARMul_State * state, u32 value);
extern u32 ARMul_GetSPSR (ARMul_State * state, u32 mode);
extern void ARMul_SetSPSR (ARMul_State * state, u32 mode, u32 value);

/***************************************************************************\
*              Definitons of things in the memory interface                 *
\***************************************************************************/

extern unsigned ARMul_MemoryInit (ARMul_State * state,
				  unsigned int initmemsize);
extern void ARMul_MemoryExit (ARMul_State * state);

extern u32 ARMul_LoadInstrS (ARMul_State * state, u32 address,
				 u32 isize);
extern u32 ARMul_LoadInstrN (ARMul_State * state, u32 address,
				 u32 isize);
#ifdef __cplusplus
extern "C" {
#endif
extern u32 ARMul_ReLoadInstr (ARMul_State * state, u32 address,
				  u32 isize);
#ifdef __cplusplus
	}
#endif
extern u32 ARMul_LoadWordS (ARMul_State * state, u32 address);
extern u32 ARMul_LoadWordN (ARMul_State * state, u32 address);
extern u32 ARMul_LoadHalfWord (ARMul_State * state, u32 address);
extern u32 ARMul_LoadByte (ARMul_State * state, u32 address);

extern void ARMul_StoreWordS (ARMul_State * state, u32 address,
			      u32 data);
extern void ARMul_StoreWordN (ARMul_State * state, u32 address,
			      u32 data);
extern void ARMul_StoreHalfWord (ARMul_State * state, u32 address,
				 u32 data);
extern void ARMul_StoreByte (ARMul_State * state, u32 address,
			     u32 data);

extern u32 ARMul_SwapWord (ARMul_State * state, u32 address,
			       u32 data);
extern u32 ARMul_SwapByte (ARMul_State * state, u32 address,
			       u32 data);

extern void ARMul_Icycles (ARMul_State * state, unsigned number,
			   u32 address);
extern void ARMul_Ccycles (ARMul_State * state, unsigned number,
			   u32 address);

extern u32 ARMul_ReadWord (ARMul_State * state, u32 address);
extern u32 ARMul_ReadByte (ARMul_State * state, u32 address);
extern void ARMul_WriteWord (ARMul_State * state, u32 address,
			     u32 data);
extern void ARMul_WriteByte (ARMul_State * state, u32 address,
			     u32 data);

extern u32 ARMul_MemAccess (ARMul_State * state, u32, u32,
				u32, u32, u32, u32, u32,
				u32, u32, u32);

/***************************************************************************\
*            Definitons of things in the co-processor interface             *
\***************************************************************************/

#define ARMul_FIRST 0
#define ARMul_TRANSFER 1
#define ARMul_BUSY 2
#define ARMul_DATA 3
#define ARMul_INTERRUPT 4
#define ARMul_DONE 0
#define ARMul_CANT 1
#define ARMul_INC 3

#define ARMul_CP13_R0_FIQ       0x1
#define ARMul_CP13_R0_IRQ       0x2
#define ARMul_CP13_R8_PMUS      0x1

#define ARMul_CP14_R0_ENABLE    0x0001
#define ARMul_CP14_R0_CLKRST    0x0004
#define ARMul_CP14_R0_CCD       0x0008
#define ARMul_CP14_R0_INTEN0    0x0010
#define ARMul_CP14_R0_INTEN1    0x0020
#define ARMul_CP14_R0_INTEN2    0x0040
#define ARMul_CP14_R0_FLAG0     0x0100
#define ARMul_CP14_R0_FLAG1     0x0200
#define ARMul_CP14_R0_FLAG2     0x0400
#define ARMul_CP14_R10_MOE_IB   0x0004
#define ARMul_CP14_R10_MOE_DB   0x0008
#define ARMul_CP14_R10_MOE_BT   0x000c
#define ARMul_CP15_R1_ENDIAN    0x0080
#define ARMul_CP15_R1_ALIGN     0x0002
#define ARMul_CP15_R5_X         0x0400
#define ARMul_CP15_R5_ST_ALIGN  0x0001
#define ARMul_CP15_R5_IMPRE     0x0406
#define ARMul_CP15_R5_MMU_EXCPT 0x0400
#define ARMul_CP15_DBCON_M      0x0100
#define ARMul_CP15_DBCON_E1     0x000c
#define ARMul_CP15_DBCON_E0     0x0003

extern unsigned ARMul_CoProInit (ARMul_State * state);
extern void ARMul_CoProExit (ARMul_State * state);
extern void ARMul_CoProAttach (ARMul_State * state, unsigned number,
			       ARMul_CPInits * init, ARMul_CPExits * exit,
			       ARMul_LDCs * ldc, ARMul_STCs * stc,
			       ARMul_MRCs * mrc, ARMul_MCRs * mcr,
			       ARMul_MRRCs * mrrc, ARMul_MCRRs * mcrr,
			       ARMul_CDPs * cdp,
			       ARMul_CPReads * read, ARMul_CPWrites * write);
extern void ARMul_CoProDetach (ARMul_State * state, unsigned number);

#define EQ 0
#define NE 1
#define CS 2
#define CC 3
#define MI 4
#define PL 5
#define VS 6
#define VC 7
#define HI 8
#define LS 9
#define GE 10
#define LT 11
#define GT 12
#define LE 13
#define AL 14
#define NV 15

#ifndef NFLAG
#define NFLAG	state->NFlag
#endif //NFLAG

#ifndef ZFLAG
#define ZFLAG	state->ZFlag
#endif //ZFLAG

#ifndef CFLAG
#define CFLAG	state->CFlag
#endif //CFLAG

#ifndef VFLAG
#define VFLAG	state->VFlag
#endif //VFLAG

#ifndef IFLAG
#define IFLAG	(state->IFFlags >> 1)
#endif //IFLAG

#ifndef FFLAG
#define FFLAG	(state->IFFlags & 1)
#endif //FFLAG

#ifndef IFFLAGS
#define IFFLAGS	state->IFFlags
#endif //VFLAG

#define FLAG_MASK	0xf0000000
#define NBIT_SHIFT	31
#define ZBIT_SHIFT	30
#define CBIT_SHIFT	29
#define VBIT_SHIFT	28

#endif /* _ARMDEFS_H_ */
