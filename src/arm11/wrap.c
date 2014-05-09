#include <util.h>
#include <arm11.h>
#include <mem.h>

#include "armdefs.h"
#include "armemu.h"
#include "threads.h"

#define dumpstack 1
#define dumpstacksize 500
#define maxdmupaddr 0x0033a850

/*ARMword ARMul_GetCPSR (ARMul_State * state) {
    return 0;
}
ARMword ARMul_GetSPSR (ARMul_State * state, ARMword mode) {
    return 0;
}
void ARMul_SetCPSR (ARMul_State * state, ARMword value) {

}
void ARMul_SetSPSR (ARMul_State * state, ARMword mode, ARMword value) {

}*/

void ARMul_Icycles (ARMul_State * state, unsigned number,
                    ARMword address)
{
}
void ARMul_Ccycles (ARMul_State * state, unsigned number,
                    ARMword address)
{
}
ARMword
ARMul_LoadInstrS (ARMul_State * state, ARMword address, ARMword isize)
{
    state->NumScycles++;

#ifdef HOURGLASS
    if ((state->NumScycles & HOURGLASS_RATE) == 0) {
        HOURGLASS;
    }
#endif
    if(isize == 2)
        return (u16) mem_Read16 (address);
    else
        return (u32) mem_Read32 (address);
}
ARMword
ARMul_LoadInstrN (ARMul_State * state, ARMword address, ARMword isize)
{
    state->NumNcycles++;

    if(isize == 2)
        return (u16) mem_Read16 (address);
    else
        return (u32) mem_Read32 (address);
}

ARMword
ARMul_ReLoadInstr (ARMul_State * state, ARMword address, ARMword isize)
{
    ARMword data;

    if ((isize == 2) && (address & 0x2)) {
        ARMword lo;
        lo = (u16)mem_Read16 (address);
        /*if (fault) {
        	ARMul_PREFETCHABORT (address);
        	return ARMul_ABORTWORD;
        }
        else {
        	ARMul_CLEARABORT;
                        }*/
        return lo;
    }

    data = (u32) mem_Read32(address);

    /*if (fault) {
    	ARMul_PREFETCHABORT (address);
    	return ARMul_ABORTWORD;
    }
    else {
    	ARMul_CLEARABORT;
                }*/

    return data;
}
ARMword ARMul_ReadWord (ARMul_State * state, ARMword address)
{
    ARMword data;

    data = mem_Read32(address);
    /*if (fault) {
    	ARMul_DATAABORT (address);
    	return ARMul_ABORTWORD;
    }
    else {
    	ARMul_CLEARABORT;
                }*/
    return data;
}
ARMword ARMul_LoadWordS (ARMul_State * state, ARMword address)
{
    state->NumScycles++;
    return ARMul_ReadWord (state, address);
}
ARMword ARMul_LoadWordN (ARMul_State * state, ARMword address)
{
    state->NumNcycles++;
    return ARMul_ReadWord (state, address);
}
ARMword ARMul_LoadHalfWord (ARMul_State * state, ARMword address)
{
    ARMword data;

    state->NumNcycles++;
    data = (u16) mem_Read16 (address);

    /*if (fault) {
    	ARMul_DATAABORT (address);
    	return ARMul_ABORTWORD;
    }
    else {
    	ARMul_CLEARABORT;
                }*/

    return data;

}
ARMword ARMul_ReadByte (ARMul_State * state, ARMword address)
{
    ARMword data;
    data = (u8) mem_Read8(address);

    /*if (fault) {
    	ARMul_DATAABORT (address);
    	return ARMul_ABORTWORD;
    }
    else {
    	ARMul_CLEARABORT;
                }*/

    return data;

}
ARMword ARMul_LoadByte (ARMul_State * state, ARMword address)
{
    state->NumNcycles++;
    return ARMul_ReadByte (state, address);
}
void ARMul_StoreHalfWord (ARMul_State * state, ARMword address, ARMword data)
{
    state->NumNcycles++;
    mem_Write16(address, data);
    /*if (fault) {
    	ARMul_DATAABORT (address);
    	return;
    }
    else {
    	ARMul_CLEARABORT;
                }*/
}
void ARMul_StoreByte (ARMul_State * state, ARMword address, ARMword data)
{
    state->NumNcycles++;

#ifdef VALIDATE
    if (address == TUBE) {
        if (data == 4)
            state->Emulate = FALSE;
        else
            (void) putc ((char) data, stderr);	/* Write Char */
        return;
    }
#endif

    ARMul_WriteByte (state, address, data);
}

ARMword ARMul_SwapWord (ARMul_State * state, ARMword address, ARMword data)
{
    ARMword temp;
    state->NumNcycles++;
    temp = ARMul_ReadWord (state, address);
    state->NumNcycles++;
    mem_Write32(address, data);
    return temp;
}
ARMword ARMul_SwapByte (ARMul_State * state, ARMword address, ARMword data)
{
    ARMword temp;
    temp = ARMul_LoadByte (state, address);
    mem_Write8(address, data);
    return temp;
}
void ARMul_WriteWord (ARMul_State * state, ARMword address, ARMword data)
{
    mem_Write32(address, data);
    /*if (fault) {
      ARMul_DATAABORT (address);
      return;
      }
      else {
      ARMul_CLEARABORT;
      }*/
}
void ARMul_WriteByte (ARMul_State * state, ARMword address, ARMword data)
{
    mem_Write8(address, data);
}
void ARMul_StoreWordS (ARMul_State * state, ARMword address, ARMword data)
{
    state->NumScycles++;
    ARMul_WriteWord (state, address, data);
}
void ARMul_StoreWordN (ARMul_State * state, ARMword address, ARMword data)
{
    state->NumNcycles++;
    ARMul_WriteWord (state, address, data);
}

/* WRAPPER */
ARMul_State s;
void arm11_Disasm32(u32 opc)
{

}

void arm11_Init()
{
    ARMul_EmulateInit();

    memset(&s, 0, sizeof(s));
    ARMul_NewState(&s);

    s.abort_model = 0;
    s.bigendSig = LOW;

    ARMul_SelectProcessor(&s, ARM_v6_Prop | ARM_v5_Prop | ARM_v5e_Prop);
    s.lateabtSig = LOW;

    ARMul_CoProInit(&s);

    ARMul_Reset (&s);
    s.NextInstr = RESUME;
    s.Emulate = 3;
}
bool arm11_Step()
{
    s.NumInstrsToExecute = 1;
    ARMul_Emulate32(&s);

    return true;
}

bool arm11_Run(int numInstructions)
{
    s.NumInstrsToExecute = numInstructions;
    ARMul_Emulate32(&s);

    return true;
}

u32 arm11_R(u32 n)
{
    if(n >= 16) {
        DEBUG("Invalid n.\n");
        return 0;
    }

    return s.Reg[n];
}
void arm11_SetR(u32 n, u32 val)
{
    if(n >= 16) {
        DEBUG("Invalid n.\n");
        return;
    }

    s.Reg[n] = val;
}
bool aufloeser(char* a,u32 addr)
{
    if (mem_test(addr) && maxdmupaddr > addr)
    {
#ifdef not
        static const char filename[] = "map.idc";
        FILE *file = fopen(filename, "r");
        if (file != NULL)
        {
            char line[256]; /* or other suitable maximum line size */
            char scanned[256]; /* or other suitable maximum line size */
            strcpy(scanned, "");
            while (fgets(line, sizeof line, file) != NULL) /* read a line */
            {
                u32 addr2 = 0;
                sscanf(line, "MakeName(0x%08x,\"%s\");", &addr2, scanned);
                if (addr2 < addr && addr2 != 0)
                    strcpy(a, scanned);
                else
                {
                    int i = 0;
                }
            }
            fclose(file);
        }
#endif
        static const char filename[] = "map";
        FILE *file = fopen(filename, "r");
        if (file != NULL)
        {
            char line[256]; /* or other suitable maximum line size */
            char scanned[256]; /* or other suitable maximum line size */
            strcpy(scanned, "");
            while (fgets(line, sizeof line, file) != NULL) /* read a line */
            {
                u32 addr2 = 0;
                u32 size = 0;
                sscanf(line, "0x%08x %08u %s", &addr2, &size, scanned);
                if (addr2 <= addr && addr2 + size >= addr)
                    strcpy(a, scanned);
                else
                {
                    int i = 0;
                }
            }
            fclose(file);
        }
    }
    else
    {
        sprintf(a, "");
    }

    return true;
}
void arm11_Dump()
{
    DEBUG("Reg dump:\n");
    char a[256];
    u32 i;
    for (i = 0; i < 4; i++) {
        DEBUG("r%02d: %08x r%02d: %08x r%02d: %08x r%02d: %08x\n",
              4 * i, s.Reg[4 * i], 4 * i + 1, s.Reg[4 * i + 1], 4 * i + 2, s.Reg[4 * i + 2], 4 * i + 3, s.Reg[4 * i + 3]);
    }
    memset(a, 0, 256);
    aufloeser(a, s.Reg[15]);
    DEBUG("current pc %s\n",a);
    for (int i = 0; i < dumpstacksize; i++)
    {

        if (mem_test(s.Reg[13] + i * 4))
        {
            aufloeser(a, mem_Read32(s.Reg[13] + i * 4));
            DEBUG("%08X %08x %s\n", s.Reg[13] + i * 4, mem_Read32(s.Reg[13] + i * 4), a);
        }
    }
    DEBUG("\n");
}
void arm11_SetPCSP(u32 pc, u32 sp)
{
    s.Reg[13] = sp;
    s.Reg[15] = pc;
    s.pc = pc;
}
void arm11_SaveContext(thread *t)
{
    for (int i = 0; i < 13; i++) t->r[i] = s.Reg[i];
    t->sp = s.Reg[13];
    t->lr = s.Reg[14];
    t->pc = s.pc;
    t->cpsr = s.Cpsr;
    t->mode = s.NextInstr;
    t->r15 = s.Reg[15];

    for (int i = 0; i < 32; i++) t->fpu_r[i] = s.ExtReg[i];
    t->fpscr = s.VFP[1];
    t->fpexc = s.VFP[2];
}
void arm11_LoadContext(thread *t)
{
    for (int i = 0; i < 13; i++) s.Reg[i] = t->r[i];
    s.Reg[13] = t->sp;
    s.Reg[14] = t->lr;
    s.pc = t->pc;
    s.Cpsr = t->cpsr;
    s.NextInstr = t->mode;
    s.Reg[15] = t->r15;

    for (int i = 0; i < 32; i++) s.ExtReg[i] = t->fpu_r[i];
    s.VFP[1] = t->fpscr;
    s.VFP[2] = t->fpexc;
}