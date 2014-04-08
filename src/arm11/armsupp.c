/*  armsupp.c -- ARMulator support code:  ARM6 Instruction Emulator.
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

#include "armdefs.h"
#include "armemu.h"
#include "ansidecl.h"

unsigned xscale_cp15_cp_access_allowed (ARMul_State * state, unsigned reg,
                                        unsigned cpnum);
//extern int skyeye_instr_debug;
/* Definitions for the support routines.  */

#define isize ((state->TFlag) ? 2 : 4)

static u32 ModeToBank (u32);
static void EnvokeList (ARMul_State *, unsigned int, unsigned int);

struct EventNode {
    /* An event list node.  */
    unsigned (*func) (ARMul_State *);	/* The function to call.  */
    struct EventNode *next;
};

/* This routine returns the value of a register from a mode.  */

u32
ARMul_GetReg (ARMul_State * state, unsigned mode, unsigned reg)
{
    mode &= MODEBITS;
    return (state->Reg[reg]);
}

/* This routine sets the value of a register for a mode.  */

void
ARMul_SetReg (ARMul_State * state, unsigned mode, unsigned reg, u32 value)
{
    mode &= MODEBITS;
    state->Reg[reg] = value;
}

/* This routine returns the value of the PC, mode independently.  */

u32
ARMul_GetPC (ARMul_State * state)
{
    return state->Reg[15];
}

/* This routine returns the value of the PC, mode independently.  */

u32
ARMul_GetNextPC (ARMul_State * state)
{
    return state->Reg[15] + isize;
}

/* This routine sets the value of the PC.  */

void
ARMul_SetPC (ARMul_State * state, u32 value)
{
    state->Reg[15] = value & PCBITS;
}

/* This routine returns the value of register 15, mode independently.  */

u32
ARMul_GetR15 (ARMul_State * state)
{
    return (state->Reg[15]);
}

/* This routine sets the value of Register 15.  */

void
ARMul_SetR15 (ARMul_State * state, u32 value)
{
    state->Reg[15] = value & PCBITS;
}


/* Returns the register number of the nth register in a reg list.  */

unsigned
ARMul_NthReg (u32 instr, unsigned number)
{
    unsigned bit, upto;

    for (bit = 0, upto = 0; upto <= number; bit++)
        if (BIT (bit))
            upto++;

    return (bit - 1);
}

/* Assigns the N and Z flags depending on the value of result.  */

void
ARMul_NegZero (ARMul_State * state, u32 result)
{
    if (NEG (result)) {
        SETN;
        CLEARZ;
    } else if (result == 0) {
        CLEARN;
        SETZ;
    } else {
        CLEARN;
        CLEARZ;
    }
}

/* Compute whether an addition of A and B, giving RESULT, overflowed.  */

int
AddOverflow (u32 a, u32 b, u32 result)
{
    return ((NEG (a) && NEG (b) && POS (result))
            || (POS (a) && POS (b) && NEG (result)));
}

/* Compute whether a subtraction of A and B, giving RESULT, overflowed.  */

int
SubOverflow (u32 a, u32 b, u32 result)
{
    return ((NEG (a) && POS (b) && POS (result))
            || (POS (a) && NEG (b) && NEG (result)));
}

/* Assigns the C flag after an addition of a and b to give result.  */

void
ARMul_AddCarry (ARMul_State * state, u32 a, u32 b, u32 result)
{
    ASSIGNC ((NEG (a) && NEG (b)) ||
             (NEG (a) && POS (result)) || (NEG (b) && POS (result)));
}

/* Assigns the V flag after an addition of a and b to give result.  */

void
ARMul_AddOverflow (ARMul_State * state, u32 a, u32 b, u32 result)
{
    ASSIGNV (AddOverflow (a, b, result));
}

/* Assigns the C flag after an subtraction of a and b to give result.  */

void
ARMul_SubCarry (ARMul_State * state, u32 a, u32 b, u32 result)
{
    ASSIGNC ((NEG (a) && POS (b)) ||
             (NEG (a) && POS (result)) || (POS (b) && POS (result)));
}

/* Assigns the V flag after an subtraction of a and b to give result.  */

void
ARMul_SubOverflow (ARMul_State * state, u32 a, u32 b, u32 result)
{
    ASSIGNV (SubOverflow (a, b, result));
}

/* This function does the work of generating the addresses used in an
   LDC instruction.  The code here is always post-indexed, it's up to the
   caller to get the input address correct and to handle base register
   modification. It also handles the Busy-Waiting.  */

void
ARMul_LDC (ARMul_State * state, u32 instr, u32 address)
{
    DEBUG("plutoo: LDC not implemented\n");
    exit(1);
    /*
    unsigned cpab;
    u32 data;

    UNDEF_LSCPCBaseWb;

    cpab = (state->LDC[CPNum]) (state, ARMul_FIRST, instr, 0);
    while (cpab == ARMul_BUSY) {
        if (IntPending (state)) {
            cpab = (state->LDC[CPNum]) (state, ARMul_INTERRUPT,
                                        instr, 0);
            return;
        } else
            cpab = (state->LDC[CPNum]) (state, ARMul_BUSY, instr,
                                        0);
    }
    if (cpab == ARMul_CANT) {
        CPTAKEABORT;
        return;
    }

    cpab = (state->LDC[CPNum]) (state, ARMul_TRANSFER, instr, 0);
    data = ARMul_LoadWordN (state, address);
    //chy 2004-05-25
    if (state->abortSig || state->Aborted)
        goto L_ldc_takeabort;

    BUSUSEDINCPCN;
//chy 2004-05-25

    cpab = (state->LDC[CPNum]) (state, ARMul_DATA, instr, data);

    while (cpab == ARMul_INC) {
        address += 4;
        data = ARMul_LoadWordN (state, address);
        //chy 2004-05-25
        if (state->abortSig || state->Aborted)
            goto L_ldc_takeabort;

        cpab = (state->LDC[CPNum]) (state, ARMul_DATA, instr, data);
    }

//chy 2004-05-25
L_ldc_takeabort:
    if (BIT (21)) {
        if (!
                ((state->abortSig || state->Aborted)
                 && state->lateabtSig == LOW))
            LSBase = state->Base;
    }

    if (state->abortSig || state->Aborted)
        TAKEABORT;
*/
}

/* This function does the work of generating the addresses used in an
   STC instruction.  The code here is always post-indexed, it's up to the
   caller to get the input address correct and to handle base register
   modification. It also handles the Busy-Waiting.  */

void
ARMul_STC (ARMul_State * state, u32 instr, u32 address)
{
    DEBUG("plutoo: STC not implemented\n");
    exit(1);

    /*
    unsigned cpab;
    u32 data;

    UNDEF_LSCPCBaseWb;

    //printf("SKYEYE ARMul_STC, CPnum is %x, instr %x, addr %x\n",CPNum, instr, address);
    //chy 2004-05-23 should update this function in the future,should concern dataabort
//  skyeye_instr_debug=0;printf("SKYEYE  debug end!!!!\n");
// chy 2004-05-25 , fix it now,so needn't printf
//  printf("SKYEYE ARMul_STC, should update this function!!!!!\n");

    //exit(-1);
    cpab = (state->STC[CPNum]) (state, ARMul_FIRST, instr, &data);
    while (cpab == ARMul_BUSY) {
        ARMul_Icycles (state, 1, 0);
        if (IntPending (state)) {
            cpab = (state->STC[CPNum]) (state, ARMul_INTERRUPT,
                                        instr, 0);
            return;
        } else
            cpab = (state->STC[CPNum]) (state, ARMul_BUSY, instr,
                                        &data);
    }

    if (cpab == ARMul_CANT) {
        CPTAKEABORT;
        return;
    }
#ifndef MODE32
    if (ADDREXCEPT (address) || VECTORACCESS (address))
        INTERNALABORT (address);
#endif
    BUSUSEDINCPCN;
//chy 2004-05-25
    cpab = (state->STC[CPNum]) (state, ARMul_DATA, instr, &data);
    ARMul_StoreWordN (state, address, data);
    //chy 2004-05-25
    if (state->abortSig || state->Aborted)
        goto L_stc_takeabort;

    while (cpab == ARMul_INC) {
        address += 4;
        cpab = (state->STC[CPNum]) (state, ARMul_DATA, instr, &data);
        ARMul_StoreWordN (state, address, data);
        //chy 2004-05-25
        if (state->abortSig || state->Aborted)
            goto L_stc_takeabort;
    }
//chy 2004-05-25
L_stc_takeabort:
    if (BIT (21)) {
        if (!
                ((state->abortSig || state->Aborted)
                 && state->lateabtSig == LOW))
            LSBase = state->Base;
    }

    if (state->abortSig || state->Aborted)
        TAKEABORT;
    */
}

/* This function does the Busy-Waiting for an MCR instruction.  */

void
ARMul_MCR (ARMul_State * state, u32 instr, u32 source)
{
    DEBUG("plutoo: MCR not implemented\n");
    exit(1);
    /*
    unsigned cpab;

    cpab = (state->MCR[CPNum]) (state, ARMul_FIRST, instr, source);

    while (cpab == ARMul_BUSY) {
        ARMul_Icycles (state, 1, 0);

        if (IntPending (state)) {
            cpab = (state->MCR[CPNum]) (state, ARMul_INTERRUPT,
                                        instr, 0);
            return;
        } else
            cpab = (state->MCR[CPNum]) (state, ARMul_BUSY, instr,
                                        source);
    }

    if (cpab == ARMul_CANT) {
        printf ("SKYEYE ARMul_MCR, CANT, UndefinedInstr %x CPnum is %x, source %x\n", instr, CPNum, source);
        ARMul_Abort (state, ARMul_UndefinedInstrV);
    } else {
        BUSUSEDINCPCN;
        ARMul_Ccycles (state, 1, 0);
    }*/
}

/* This function does the Busy-Waiting for an MCRR instruction.  */

void
ARMul_MCRR (ARMul_State * state, u32 instr, u32 source1, u32 source2)
{
    DEBUG("plutoo: MCRR not implemented\n");
    exit(1);
    /*
    unsigned cpab;

    cpab = (state->MCRR[CPNum]) (state, ARMul_FIRST, instr, source1, source2);

    while (cpab == ARMul_BUSY) {
        ARMul_Icycles (state, 1, 0);

        if (IntPending (state)) {
            cpab = (state->MCRR[CPNum]) (state, ARMul_INTERRUPT,
                                         instr, 0, 0);
            return;
        } else
            cpab = (state->MCRR[CPNum]) (state, ARMul_BUSY, instr,
                                         source1, source2);
    }
    if (cpab == ARMul_CANT) {
        printf ("In %s, CoProcesscor returned CANT, CPnum is %x, instr %x, source %x %x\n", __FUNCTION__, CPNum, instr, source1, source2);
        ARMul_Abort (state, ARMul_UndefinedInstrV);
    } else {
        BUSUSEDINCPCN;
        ARMul_Ccycles (state, 1, 0);
    }
    */
}

/* This function does the Busy-Waiting for an MRC instruction.  */

u32
ARMul_MRC (ARMul_State * state, u32 instr)
{
    DEBUG("plutoo: MRC not implemented\n");
    exit(1);

    /*
    unsigned cpab;
    u32 result = 0;

    cpab = (state->MRC[CPNum]) (state, ARMul_FIRST, instr, &result);
    while (cpab == ARMul_BUSY) {
        ARMul_Icycles (state, 1, 0);
        if (IntPending (state)) {
            cpab = (state->MRC[CPNum]) (state, ARMul_INTERRUPT,
                                        instr, 0);
            return (0);
        } else
            cpab = (state->MRC[CPNum]) (state, ARMul_BUSY, instr,
                                        &result);
    }
    if (cpab == ARMul_CANT) {
        printf ("SKYEYE ARMul_MRC,CANT UndefInstr  CPnum is %x, instr %x\n", CPNum, instr);
        ARMul_Abort (state, ARMul_UndefinedInstrV);
        // Parent will destroy the flags otherwise.
        result = ECC;
    } else {
        BUSUSEDINCPCN;
        ARMul_Ccycles (state, 1, 0);
        ARMul_Icycles (state, 1, 0);
    }

    return result;
    */
}

/* This function does the Busy-Waiting for an MRRC instruction. (to verify) */

void
ARMul_MRRC (ARMul_State * state, u32 instr, u32 * dest1, u32 * dest2)
{
    DEBUG("plutoo: MRRC not implemented\n");
    exit(1);

    /*
    unsigned cpab;
    u32 result1 = 0;
    u32 result2 = 0;

    cpab = (state->MRRC[CPNum]) (state, ARMul_FIRST, instr, &result1, &result2);
    while (cpab == ARMul_BUSY) {
        ARMul_Icycles (state, 1, 0);
        if (IntPending (state)) {
            cpab = (state->MRRC[CPNum]) (state, ARMul_INTERRUPT,
                                         instr, 0, 0);
            return;
        } else
            cpab = (state->MRRC[CPNum]) (state, ARMul_BUSY, instr,
                                         &result1, &result2);
    }
    if (cpab == ARMul_CANT) {
        printf ("In %s, CoProcesscor returned CANT, CPnum is %x, instr %x\n", __FUNCTION__, CPNum, instr);
        ARMul_Abort (state, ARMul_UndefinedInstrV);
    } else {
        BUSUSEDINCPCN;
        ARMul_Ccycles (state, 1, 0);
        ARMul_Icycles (state, 1, 0);
    }

    *dest1 = result1;
    *dest2 = result2;
    */
}

/* This function does the Busy-Waiting for an CDP instruction.  */

void
ARMul_CDP (ARMul_State * state, u32 instr)
{
    DEBUG("plutoo: CDP not implemented\n");
    exit(1);

    /*unsigned cpab;

    cpab = (state->CDP[CPNum]) (state, ARMul_FIRST, instr);
    while (cpab == ARMul_BUSY) {
        ARMul_Icycles (state, 1, 0);
        if (IntPending (state)) {
            cpab = (state->CDP[CPNum]) (state, ARMul_INTERRUPT,
                                        instr);
            return;
        } else
            cpab = (state->CDP[CPNum]) (state, ARMul_BUSY, instr);
    }
    if (cpab == ARMul_CANT)
        ARMul_Abort (state, ARMul_UndefinedInstrV);
    else
    BUSUSEDN;*/
}

/* This function handles Undefined instructions, as CP isntruction.  */

void
ARMul_UndefInstr (ARMul_State * state, u32 instr ATTRIBUTE_UNUSED)
{
    fprintf(stderr, "In %s, line = %d, undef instr: 0x%x\n",
            __func__, __LINE__, instr);
    //ARMul_Abort (state, ARMul_UndefinedInstrV);
}

/* Align a word access to a non word boundary.  */

u32
ARMul_Align (state, address, data)
ARMul_State *state ATTRIBUTE_UNUSED;
u32 address;
u32 data;
{
    /* This code assumes the address is really unaligned,
       as a shift by 32 is undefined in C.  */

    address = (address & 3) << 3;	/* Get the word address.  */
    return ((data >> address) | (data << (32 - address)));	/* rot right */
}
