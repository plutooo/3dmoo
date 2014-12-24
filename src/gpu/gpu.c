/*
* Copyright (C) 2014 - plutoo
* Copyright (C) 2014 - ichfly
* Copyright (C) 2014 - Normmatt
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

//Parts of this file are based on code taken from Citra, copyright (C) 2014 Tony Wasserka.

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "gpu.h"
#include <math.h>

//#define GSP_ENABLE_LOG

u32 GPU_Regs[0xFFFF]; //do they all exist don't know but well

u32 GPUshadercodebuffer[0xFFFF]; //how big is the buffer?
u32 swizzle_data[0xFFFF]; //how big is the buffer?

extern int noscreen;


void gpu_Init()
{
    LINEmembuffer = malloc(0x8000000);
    VRAMbuff = malloc(0x800000);//malloc(0x600000);
    GSPsharedbuff = malloc(GSPsharebuffsize);

    memset(LINEmembuffer, 0, 0x8000000);
    memset(VRAMbuff, 0, 0x600000);
    memset(GSPsharedbuff, 0, GSPsharebuffsize);
    memset(GPUshadercodebuffer, 0, 0xFFFF*4);

    gpu_WriteReg32(frameselecttop, 0);
    gpu_WriteReg32(RGBuponeleft, 0x18000000);
    gpu_WriteReg32(RGBuptwoleft, 0x18000000 + 0x5DC00 * 1);
    gpu_WriteReg32(RGBuponeright, 0x18000000 + 0x5DC00 * 2);
    gpu_WriteReg32(RGBuptworight, 0x18000000 + 0x5DC00 * 3);
    gpu_WriteReg32(frameselectbot, 0);
    gpu_WriteReg32(RGBdownoneleft, 0x18000000 + 0x5DC00 * 4);
    gpu_WriteReg32(RGBdowntwoleft, 0x18000000 + 0x5DC00 * 5);

    //mem_Write32(0x1FF81080, (u32)0.0f);
}

u32 convertvirtualtopys(u32 addr) //todo
{
    if (addr >= 0x14000000 && addr < 0x1C000000)return addr + 0xC000000; //FCRAM
    if (addr >= 0x1F000000 && addr < 0x1F600000)return addr - 0x7000000; //VRAM
    GPUDEBUG("can't convert vitual to py %08x\n",addr);
    return 0;
}

u32 getsizeofwight(u16 val) //this is the size of pixel
{
    switch (val&0x7000) { //check this
    case 0x0000: //RGBA8
        return 4;
    case 0x1000: //RGB8
        return 3;
    case 0x2000: //RGB565
        return 2;
    case 0x3000://RGB5A1
        return 2;
    case 0x4000://RGBA4
        return 2;
    default:
        GPUDEBUG("unknown len %04x\n",val);
        return 2;
    }
}

u32 renderaddr = 0;
u32 unknownaddr = 0;

static void updateGPUintreg(u32 data, u32 ID, u8 mask)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (mask&(1 << i)) {
            GPU_Regs[ID] = (GPU_Regs[ID] & ~(0xFF << (8 * i))) | (data & (0xFF << (8 * i)));
        }
    }
}



u8 VSFloatUniformSetuptembuffercurrent = 0;
u32 VSFloatUniformSetuptembuffer[4];


#define Format_BYTE 0
#define Format_UBYTE 1
#define Format_SHORT 2
#define Format_FLOAT 3




#define VertexShaderState_INVALID_ADDRESS 0xFFFFFFFF

#define STACK_MAX 64
typedef struct Stack {
    u32     data[STACK_MAX];
    int     size;
} aStack;

void Stack_Init(struct Stack *S)
{
    S->size = 0;
}

bool Stack_Empty(struct Stack *S)
{
    return S->size <= 0;
}

int Stack_Top(struct Stack *S)
{
    if(S->size == 0) {
        fprintf(stderr, "Error: stack empty\n");
        return -1;
    }

    return S->data[S->size - 1];
}

void Stack_Push(struct Stack *S, u32 d)
{
    if(S->size < STACK_MAX)
        S->data[S->size++] = d;
    else
        fprintf(stderr, "Error: stack full\n");
}

void Stack_Pop(struct Stack *S)
{
    if(S->size == 0)
        fprintf(stderr, "Error: stack empty\n");
    else
        S->size--;
}

struct VertexShaderState {
    u32* program_counter;
    u32* program_counter_end;

    //Registers are like this:
    //name			nr component	count	R/W		Nr Bits
    //-----------------------------------------------------
    //input			4				16		R		24
    //temp			4				16		RW		24
    //float const	4				96		R		24
    //address		2				1		RW		8
    //boolean		1				16		R		1
    //integer		1				4		R		24
    //loop counter	1				1		R		8
    //output		4				16		W		24
    //status		1				2		RW		1

    float* input_register_table[16];
    struct vec4  temporary_registers[16];
    u8  address_registers[2];
    bool boolean_registers[16];
    u8 integer_registers[4][3];

    u8 loop_count;

    float* output_register_table[16 * 4];

    bool status_registers[2];

    /*u32* call_stack_pointer; // TODO: What is the maximal call stack depth?
    u32 call_stack[16];*/

    struct Stack call_stack;
    struct Stack call_type_stack;
    struct Stack call_end_stack;
};

struct vec4 const_vectors[96];

static u32 getattribute_register_map(u32 reg, u32 data1, u32 data2)
{
    if (reg < 8) {
        return (data1 >> (reg*4))&0xF;
    } else {
        return (data2 >> (reg * 4)) & 0xF;
    }
}






static struct OutputVertex buffer[2];
static int buffer_index = 0; // TODO: reset this on emulation restart
static int strip_ready = 0;
static void PrimitiveAssembly_SubmitVertex(struct OutputVertex* vtx)
{
    u32 topology = (GPU_Regs[TriangleTopology] >> 8) & 0x3;
    switch (topology) {
    case 0://List:
    case 3://ListIndexed:
        if (buffer_index < 2) {
            memcpy(&buffer[buffer_index++], vtx, sizeof(struct OutputVertex));
        } else {
            buffer_index = 0;

            Clipper_ProcessTriangle(&buffer[0], &buffer[1], vtx);
        }
        break;

    case 1://Strip
    case 2://Fan:
        if(strip_ready) {
            // TODO: Should be "buffer[0], buffer[1], vtx" instead!
            // Not quite sure why we need this order for things to show up properly.
            // Maybe a bug in the rasterizer?
            Clipper_ProcessTriangle(&buffer[0], &buffer[1], vtx);
        }
        memcpy(&buffer[buffer_index], vtx, sizeof(struct OutputVertex));
        if(topology == 1) { //Strip
            strip_ready |= (buffer_index == 1);
            buffer_index = !buffer_index;
        }
        else if(topology == 2) { //Fan
            buffer_index = 1;
            strip_ready = 1;
        }
        break;
    default:
        GPUDEBUG("Unknown triangle mode %x\n", (int)topology);
        break;
    }
}




#define STACKTYPE_TYPE_ID_MASK 0x80000000

#define STACKTYPE_CALL 0x00000000
#define STACKTYPE_LOOP 0x80000000
#define STACKTYPE_LOOP_int_MASK 0xF

#define SHDR_ADD 0x0
#define SHDR_DP3 0x1
#define SHDR_DP4 0x2
#define SHDR_DPH 0x3
#define SHDR_DST 0x4
#define SHDR_EXP 0x5
#define SHDR_LOG 0x6
#define SHDR_LITP 0x7
#define SHDR_MUL 0x8
#define SHDR_SGE 0x9
#define SHDR_SLT 0xA
#define SHDR_FLR 0xB
#define SHDR_MAX  0xC
#define SHDR_MIN  0xD
#define SHDR_RCP  0xE
#define SHDR_RSQ  0xF

#define SHDR_MOVA 0x12
#define SHDR_MOV  0x13

#define SHDR_RET  0x21	//This is actually NOP
#define SHDR_FLS  0x22	//This is actually END
#define SHDR_BREAKC  0x23
#define SHDR_CALL  0x24
#define SHDR_CALLC 0x25
#define SHDR_CALLB 0x26
#define SHDR_IFB 0x27
#define SHDR_IFC 0x28
#define SHDR_LOOP 0x29
#define SHDR_JPC 0x2C
#define SHDR_JPB 0x2D
#define SHDR_CMP 0x2E
#define SHDR_CMP2 0x2F
#define SHDR_MAD1 0x38
#define SHDR_MAD2 0x39
#define SHDR_MAD3 0x3A
#define SHDR_MAD4 0x3B
#define SHDR_MAD5 0x3C
#define SHDR_MAD6 0x3D
#define SHDR_MAD7 0x3E
#define SHDR_MAD8 0x3F

static u32 instr_mad_src1(u32 hex)
{
    return (hex >> 0x11) & 0x7F;
}
static u32 instr_mad_src2(u32 hex)
{
    return (hex >> 0xA) & 0x7F;
}
static u32 instr_mad_src3(u32 hex)
{
    return (hex >> 0x5) & 0x1F;
}
static u32 instr_mad_dest(u32 hex)
{
    return hex& 0x1F;
}

static u32 instr_common_src1(u32 hex)
{
    return (hex >> 0xC) & 0x7F;
}
static u32 instr_common_idx(u32 hex)
{
    return (hex >> 0x13) & 0x3;
}
static u32 instr_common_dest(u32 hex)
{
    return (hex >> 0x15) & 0x1F;
}
static u32 instr_common_src2(u32 hex)
{
    return (hex >> 0x7) & 0x1F;
}
static u32 instr_common_operand_desc_id(u32 hex)
{
    return hex & 0x1F;
}
static u32 instr_opcode(u32 hex)
{
    return (hex >> 0x1A);
}
static u32 instr_flow_control_offset_words(u32 hex)
{
    return (hex>>0xa)&0xFFF;
}
static bool swizzle_DestComponentEnabled(int i, u32 swizzle)
{
    return (swizzle & (0x8 >> i));
}

static bool ShaderCMP(float a, float b, u32 mode)
{
    //Not 100% sure:
    switch (mode) {
    case 0:
        return a == b;
    case 1:
        return a != b;
    case 2:
        return a < b;
    case 3:
        return a <= b;
    case 4:
        return a > b;
    case 5:
        return a >= b;
    }
    //This is not possible!
    return false;
}

//#define printfunc

void loop(struct VertexShaderState* state, u32 offset, u32 num_instruction, u32 return_offset,u32 int_reg)
{
    Stack_Push(&state->call_stack, offset + num_instruction);
    Stack_Push(&state->call_type_stack, STACKTYPE_LOOP | (int_reg&STACKTYPE_LOOP));
    Stack_Push(&state->call_end_stack, return_offset);
}

void call(struct VertexShaderState* state, u32 offset, u32 num_instruction, u32 return_offset)
{
    state->program_counter = &GPUshadercodebuffer[offset] - 1; // -1 to make sure when incrementing the PC we end up at the correct offset
    Stack_Push(&state->call_stack, offset + num_instruction);
    Stack_Push(&state->call_type_stack, STACKTYPE_CALL);
    Stack_Push(&state->call_end_stack, return_offset);
}

void ProcessShaderCode(struct VertexShaderState* state)
{
    while (true) {
        if(!Stack_Empty(&state->call_stack)) {
            if((state->program_counter - &GPUshadercodebuffer[0]) == Stack_Top(&state->call_stack)) {

                switch (Stack_Top(&state->call_type_stack) & STACKTYPE_TYPE_ID_MASK)
                {
                    case STACKTYPE_CALL:
                        state->program_counter = &GPUshadercodebuffer[Stack_Top(&state->call_end_stack)];
                        Stack_Pop(&state->call_type_stack);
                        Stack_Pop(&state->call_stack);
                        Stack_Pop(&state->call_end_stack);
                        break;
                    case STACKTYPE_LOOP:
                    {
                        u8 ID = Stack_Top(&state->call_type_stack) & STACKTYPE_LOOP_int_MASK;
                        if (state->loop_count <= state->integer_registers[ID][1] + state->integer_registers[ID][0])
                        {
                            state->program_counter = &GPUshadercodebuffer[Stack_Top(&state->call_end_stack)];
                        }
                        else //remove loop
                        {
                            Stack_Pop(&state->call_type_stack);
                            Stack_Pop(&state->call_stack);
                            Stack_Pop(&state->call_end_stack);
                        }
                        state->loop_count += state->integer_registers[ID][2];
                        break;
                    }
                }

                // TODO: Is "trying again" accurate to hardware?
                continue;
            }
        }

        bool increment_pc = true;
        u32 instr = *(u32*)state->program_counter;

        s8 idx = state->address_registers[instr_common_idx(instr)];
        u32 instr_common_src1v = instr_common_src1(instr) + idx;
        const float* src1_ = (instr_common_src1v < 0x10) ? state->input_register_table[instr_common_src1v]
                             : (instr_common_src1v < 0x20) ? &state->temporary_registers[instr_common_src1v - 0x10].v[0]
                             : (instr_common_src1v < 0x80) ? &const_vectors[instr_common_src1v - 0x20].v[0]
                             : (float*)0;

        u32 instr_common_src2v = instr_common_src2(instr);
        const float* src2_ = (instr_common_src2v < 0x10) ? state->input_register_table[instr_common_src2v]
                             : &state->temporary_registers[instr_common_src2v - 0x10].v[0];
        u32 instr_common_destv = instr_common_dest(instr);
        float* dest = (instr_common_destv < 8) ? state->output_register_table[4 * instr_common_destv]
                      : (instr_common_destv < 0x10) ? (float*)0
                      : (instr_common_destv < 0x20) ? &state->temporary_registers[instr_common_destv - 0x10].v[0]
                      : (float*)0;

        u32 swizzle = swizzle_data[instr_common_operand_desc_id(instr)];

        float src1[4] = {
            src1_[(int)((swizzle >> 11)& 0x3)],
            src1_[(int)((swizzle >> 9) & 0x3)],
            src1_[(int)((swizzle >> 7) & 0x3)],
            src1_[(int)((swizzle >> 5) & 0x3)],
        };
        float src2[4] = {
            src2_[(int)((swizzle >> 20) & 0x3)],
            src2_[(int)((swizzle >> 18) & 0x3)],
            src2_[(int)((swizzle >> 16) & 0x3)],
            src2_[(int)((swizzle >> 14) & 0x3)],
        };

        if (swizzle&(1 << 0xD)) {
            src2[0] = src2[0] * (-1.f);
            src2[1] = src2[1] * (-1.f);
            src2[2] = src2[2] * (-1.f);
            src2[3] = src2[3] * (-1.f);
        }
        if (swizzle & 0x10) {
            src1[0] = src1[0] * (-1.f);
            src1[1] = src1[1] * (-1.f);
            src1[2] = src1[2] * (-1.f);
            src1[3] = src1[3] * (-1.f);
        }

#ifdef printfunc
        DEBUG("opcode: %08x\n", instr);
#endif
        switch (instr_opcode(instr)) {
        case SHDR_ADD: {
#ifdef printfunc
            DEBUG("ADD %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
            for (int i = 0; i < 4; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                dest[i] = src1[i] + src2[i];
            }

            break;
        }
        case SHDR_DP3:
        case SHDR_DP4: {
#ifdef printfunc
            DEBUG("DP3/4 %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
            float dot = (0.f);
            int num_components = (instr_opcode(instr) == SHDR_DP3) ? 3 : 4;
            for (int i = 0; i < num_components; ++i)
                dot = dot + src1[i] * src2[i];

            for (int i = 0; i < num_components; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                dest[i] = dot;
            }
            break;
        }
        case SHDR_DST: {
#ifdef printfunc
            DEBUG("DST %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
            if (swizzle_DestComponentEnabled(0, swizzle)) dest[0] = 1;
            if (swizzle_DestComponentEnabled(1, swizzle)) dest[1] = src1[1] * src2[1];
            if (swizzle_DestComponentEnabled(2, swizzle)) dest[2] = src1[2];
            if (swizzle_DestComponentEnabled(3, swizzle)) dest[3] = src2[3];
            break;
        }
        case SHDR_EXP: {
#ifdef printfunc
            DEBUG("EXP %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
            for (int i = 0; i < 4; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                dest[i] = powf(2.f, src1[0]);
            }
            break;
        }
        case SHDR_LOG: {
#ifdef printfunc
            DEBUG("LOG %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
            for (int i = 0; i < 4; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                dest[i] = log2f(src1[0]);
            }
            break;
        }
        case SHDR_MUL: {
#ifdef printfunc
            DEBUG("MUL %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
            for (int i = 0; i < 4; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                dest[i] = src1[i] * src2[i];
            }

            break;
        }
        case SHDR_MAX: {
#ifdef printfunc
            DEBUG("MAX %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
            for (int i = 0; i < 4; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                dest[i] = ((src1[i] > src2[i]) ? src1[i] : src2[i]);
            }

            break;
        }
        case SHDR_MIN: {
#ifdef printfunc
            DEBUG("MIN %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
            for (int i = 0; i < 4; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                dest[i] = ((src1[i] < src2[i]) ? src1[i] : src2[i]);
            }

            break;
        }

        // Reciprocal
        case SHDR_RCP: {
#ifdef printfunc
            DEBUG("RCP %02X %02X %08x\n", instr_common_destv, instr_common_src1v, swizzle);
#endif
            for (int i = 0; i < 4; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                // TODO: Be stable against division by zero!
                // TODO: I think this might be wrong... we should only use one component here
                dest[i] = (1.0f / src1[i]);
            }

            break;
        }

        // Reciprocal Square Root
        case SHDR_RSQ: {
#ifdef printfunc
            DEBUG("RSQ %02X %02X %08x\n", instr_common_destv, instr_common_src1v, swizzle);
#endif
            for (int i = 0; i < 4; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                // TODO: Be stable against division by zero!
                // TODO: I think this might be wrong... we should only use one component here
                dest[i] = (1.0f / sqrtf(src1[i]));
            }

            break;
        }

        case SHDR_MOV: {
#ifdef printfunc
            DEBUG("MOV %02X %02X %08x\n", instr_common_destv, instr_common_src1v, swizzle);
#endif
            for (int i = 0; i < 4; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                dest[i] = src1[i];
            }
            break;
        }

        case SHDR_RET: //Really just a NOP
#ifdef printfunc
            DEBUG("RET\n");
#endif
            /*state->call_stack_pointer--;
            if (*state->call_stack_pointer == VertexShaderState_INVALID_ADDRESS) {
                return;
            } else {
                state->program_counter = &GPUshadercodebuffer[*state->call_stack_pointer];
            }
            */
            break;

        case SHDR_CALL:
        {
#ifdef printfunc
            DEBUG("CALL %08x\n",instr_flow_control_offset_words(instr));
#endif
            u32 addrv = (instr >> 8) & 0x3FFC;
            u32 boolv = (instr >> 22) & 0xF;
            u32 retv = instr & 0x3FF;

            increment_pc = false;

            //_dbg_assert_(GPU, state.call_stack_pointer - state.call_stack < sizeof(state.call_stack)); //todo

            //*state->call_stack_pointer = ((u32)(uintptr_t)state->program_counter - (u32)(uintptr_t)(&GPUshadercodebuffer[0])) / 4;
            //state->call_stack_pointer++;

            call(state, addrv / 4, retv, ((u32)(uintptr_t)(state->program_counter + 1) - (u32)(uintptr_t)(&GPUshadercodebuffer[0])) / 4);
            break;
        }

        case SHDR_CALLC:
        {
            u32 addrv = (instr >> 8) & 0x3FFC;
            u32 retv = instr & 0x3FF;
            u32 flagsv = (instr >> 22) & 0xF;


            u32 mode = flagsv & 0x3;
            bool status0 = flagsv & 0x4;
            bool status1 = flagsv & 0x8;

            bool condition = false;
            switch (mode) {
            case 0: //OR
                if ((status0 == state->status_registers[0]) || (status1 == state->status_registers[1]))
                    condition = true;
                break;
            case 1: //AND
                if ((status0 == state->status_registers[0]) && (status1 == state->status_registers[1]))
                    condition = true;
                break;
            case 2: //Y
                if (status0 == state->status_registers[0])
                    condition = true;
                break;
            case 3: //X
                if (status1 == state->status_registers[1])
                    condition = true;
                break;
            }

#ifdef printfunc
            DEBUG("CALLC %02X (%s)\n", flagsv, condition ? "true" : "false");
#endif

            if (condition)
            {
                increment_pc = false;
                call(state, addrv / 4, retv, ((u32)(uintptr_t)(state->program_counter + 1) - (u32)(uintptr_t)(&GPUshadercodebuffer[0])) / 4);
            }
            break;
        }
        case SHDR_CALLB:
        {

            u32 addrv = (instr >> 8) & 0x3FFC;
            u32 boolv = (instr >> 22) & 0xF;
            u32 retv = instr & 0x3FF;


            //_dbg_assert_(GPU, state.call_stack_pointer - state.call_stack < sizeof(state.call_stack)); //todo

            //*state->call_stack_pointer = ((u32)(uintptr_t)state->program_counter - (u32)(uintptr_t)(&GPUshadercodebuffer[0])) / 4;
            //state->call_stack_pointer++;

#ifdef printfunc
            DEBUG("CALLB %02X (%s)\n", boolv, state->boolean_registers[boolv] ? "true" : "false");
#endif

            if (state->boolean_registers[boolv])
            {
                increment_pc = false;
                call(state, addrv / 4, retv, ((u32)(uintptr_t)(state->program_counter + 1) - (u32)(uintptr_t)(&GPUshadercodebuffer[0])) / 4);
            }
            break;
        }

        case SHDR_FLS:
#ifdef printfunc
            DEBUG("FLS\n");
#endif
            // TODO: Do whatever needs to be done here?
            return;
        case SHDR_CMP:
        case SHDR_CMP2: {
#ifdef printfunc
            DEBUG("CMP\n");
#endif
            //Not 100% sure:
            u32 mode1 = (instr >> 19) & 0x7;
            u32 mode2 = (instr >> 22) & 0x7;
            //This is correct
            state->status_registers[0] = ShaderCMP(src1[0], src2[0], mode1);
            state->status_registers[1] = ShaderCMP(src1[1], src2[1], mode2);
            break;
        }
        case SHDR_IFB: {
            u32 addrv = (instr >> 8) & 0x3FFC;
            u32 boolv = (instr >> 22) & 0xF;
            u32 retv = instr & 0x3FF;

            bool condition = state->boolean_registers[boolv];

#ifdef printfunc
            DEBUG("IFB %02X (%s)\n", boolv, condition?"true":"false");
#endif
            //If condition is false skip to else case
            /*if (!condition) {
                increment_pc = false;
                state->program_counter = &GPUshadercodebuffer[addrv/4];
                break;
            }*/

            if(condition)
            {
                u32 binary_offset = ((u32)(uintptr_t)(state->program_counter) - (u32)(uintptr_t)(&GPUshadercodebuffer[0])) / 4;
                call(state, binary_offset + 1, (addrv / 4) - (binary_offset - 1), (addrv / 4)+retv);
            }
            else
            {
                call(state, addrv / 4, retv, (addrv / 4) + retv);
            }
            //Store return address on stack... This is generally used
            /**state->call_stack_pointer = (addrv / 4) + retv;
            state->call_stack_pointer++;

            //u32 *next_pc = &GPUshadercodebuffer[(addrv / 4) + retv];
            state->program_counter_end = &GPUshadercodebuffer[addrv / 4];
            state->program_counter++;
            ProcessShaderCode(state);*/

            //increment_pc = false;
            //state->program_counter = next_pc;
            break;
        }
        case SHDR_IFC: {
            u32 addrv = (instr >> 8) & 0x3FFC;
            u32 flagsv = (instr >> 22) & 0xF;
            u32 retv = instr & 0x3FF;

            u32 mode = flagsv & 0x3;
            bool status0 = flagsv & 0x4;
            bool status1 = flagsv & 0x8;

            bool condition = false;
            switch (mode) {
            case 0: //OR
                if ((status0 == state->status_registers[0]) || (status1 == state->status_registers[1]))
                    condition = true;
                break;
            case 1: //AND
                if ((status0 == state->status_registers[0]) && (status1 == state->status_registers[1]))
                    condition = true;
                break;
            case 2: //Y
                if (status0 == state->status_registers[0])
                    condition = true;
                break;
            case 3: //X
                if (status1 == state->status_registers[1])
                    condition = true;
                break;
            }

#ifdef printfunc
            DEBUG("IFC %s\n", condition ? "true" : "false");
#endif

            //If condition is false skip to else case
            /*if (!condition) {
                increment_pc = false;
                state->program_counter = &GPUshadercodebuffer[addrv / 4];
                break;
            }*/


            if(condition)
            {
                u32 binary_offset = ((u32)(uintptr_t)(state->program_counter) - (u32)(uintptr_t)(&GPUshadercodebuffer[0])) / 4;
                call(state, binary_offset + 1, (addrv / 4) - (binary_offset - 1), (addrv / 4) + retv);
            }
            else
            {
                call(state, addrv / 4, retv, (addrv / 4) + retv);
            }

            //Store return address on stack... This is generally used
            /**state->call_stack_pointer = (addrv / 4) + retv;
            state->call_stack_pointer++;

            //u32 *next_pc = &GPUshadercodebuffer[(addrv / 4) + retv];
            state->program_counter_end = &GPUshadercodebuffer[addrv / 4];
            state->program_counter++;
            ProcessShaderCode(state);*/

            //increment_pc = false;
            //state->program_counter = next_pc;
            break;
        }

        case SHDR_JPB: {
            u32 addrv = (instr >> 8) & 0x3FFC;
            u32 boolv = (instr >> 22) & 0xF;
            u32 retv = instr & 0x3FF;

            bool invert = retv == 1;
            bool condition = state->boolean_registers[boolv];

            if (invert)
                condition = !condition;

#ifdef printfunc
            DEBUG("JPB %08X, %s\n", addrv, condition?"true":"false");
#endif
            if (condition) {
                state->program_counter = &GPUshadercodebuffer[addrv / 4];
                increment_pc = false;
            }

            break;
        }

        case SHDR_JPC: {
            u32 addrv = (instr >> 8) & 0x3FFC;
            u32 flagsv = (instr >> 22) & 0xF;
            u32 retv = instr & 0x3FF;

            u32 mode = flagsv & 0x3;
            bool status0 = flagsv & 0x4;
            bool status1 = flagsv & 0x8;

            bool condition = false;
            switch (mode) {
            case 0: //OR
                if ((status0 == state->status_registers[0]) || (status1 == state->status_registers[1]))
                    condition = true;
                break;
            case 1: //AND
                if ((status0 == state->status_registers[0]) && (status1 == state->status_registers[1]))
                    condition = true;
                break;
            case 2: //Y
                if (status0 == state->status_registers[0])
                    condition = true;
                break;
            case 3: //X
                if (status1 == state->status_registers[1])
                    condition = true;
                break;
            }

#ifdef printfunc
            DEBUG("JPC %08X, %s\n", addrv, condition ? "true" : "false");
#endif
            if (condition) {
                state->program_counter = &GPUshadercodebuffer[addrv / 4];
                increment_pc = false;
            }

            break;
        }

        case SHDR_MOVA: { //TODO: FIX
#ifdef printfunc
            DEBUG("MOVA %02X %02X %08x\n", instr_common_destv, instr_common_src1v, swizzle);
#endif

            for (int i = 0; i < 2; ++i) {
                //Is it just high 8bits or low 8bits? can't be more than 8 bits at the value looks wrong otherwise
                state->address_registers[i+1] = ((u32)src1[i] >> 24);
            }

            break;
        }
        case SHDR_LOOP:
        {
            u32 NUM = (instr & 0xFF);
            u32 DST = ((instr >> 0xA) & 0xFFF);
            u32 ID = ((instr >> 0x16) & 0xF);
#ifdef printfunc
            DEBUG("LOOP %02X %03X %01x\n", NUM, DST, ID);
#endif
            state->loop_count = state->integer_registers[ID][1];
            if (state->loop_count <= state->integer_registers[ID][1] + state->integer_registers[ID][0])
            {
                increment_pc = false;
                state->program_counter = &GPUshadercodebuffer[DST];
            }
            else
                loop(state, DST, NUM, ((u32)(uintptr_t)(state->program_counter + 1) - (u32)(uintptr_t)(&GPUshadercodebuffer[0])) / 4, ID);
            break;
        }
        case SHDR_MAD1: //todo add swizzle for the other src
        case SHDR_MAD2:
        case SHDR_MAD3:
        case SHDR_MAD4:
        case SHDR_MAD5:
        case SHDR_MAD6:
        case SHDR_MAD7:
        case SHDR_MAD8:
        {
#ifdef printfunc
            DEBUG("MAD %02X %02X %02X %02X\n", instr_mad_dest(instr), instr_mad_src1(instr), instr_mad_src2(instr), instr_mad_src3(instr));
#endif
            //mad
            u32 instr_common_src1v = instr_mad_src1(instr) + idx;
            const float* src1_ = (instr_common_src1v < 0x10) ? state->input_register_table[instr_common_src1v]
                : (instr_common_src1v < 0x20) ? &state->temporary_registers[instr_common_src1v - 0x10].v[0]
                : (instr_common_src1v < 0x80) ? &const_vectors[instr_common_src1v - 0x20].v[0]
                : (float*)0;

            u32 instr_common_src2v = instr_mad_src2(instr);
            const float* src2_ = (instr_common_src2v < 0x10) ? state->input_register_table[instr_common_src2v]
                : &state->temporary_registers[instr_common_src2v - 0x10].v[0];
            u32 instr_common_src3v = instr_mad_src3(instr);
            const float* src3_ = (instr_common_src3v < 0x10) ? state->input_register_table[instr_common_src3v]
                : &state->temporary_registers[instr_common_src3v - 0x10].v[0];

            u32 instr_common_destv = instr_mad_dest(instr);
            float* dest = (instr_common_destv < 8) ? state->output_register_table[4 * instr_common_destv]
                : (instr_common_destv < 0x10) ? (float*)0
                : (instr_common_destv < 0x20) ? &state->temporary_registers[instr_common_destv - 0x10].v[0]
                : (float*)0;

            float src1[4] = {
                src1_[(int)((swizzle >> 11) & 0x3)],
                src1_[(int)((swizzle >> 9) & 0x3)],
                src1_[(int)((swizzle >> 7) & 0x3)],
                src1_[(int)((swizzle >> 5) & 0x3)],
            };
            float src2[4] = {
                src2_[(int)((swizzle >> 20) & 0x3)],
                src2_[(int)((swizzle >> 18) & 0x3)],
                src2_[(int)((swizzle >> 16) & 0x3)],
                src2_[(int)((swizzle >> 14) & 0x3)],
            };

            float src3[4] = {
                src3_[(int)((swizzle >> 29) & 0x3)],
                src3_[(int)((swizzle >> 27) & 0x3)],
                src3_[(int)((swizzle >> 25) & 0x3)],
                src3_[(int)((swizzle >> 23) & 0x3)],
            };

            if (swizzle & 0x10) {
                src1[0] = src1[0] * (-1.f);
                src1[1] = src1[1] * (-1.f);
                src1[2] = src1[2] * (-1.f);
                src1[3] = src1[3] * (-1.f);
            }
            if (swizzle&(1 << 0xD)) {
                src2[0] = src2[0] * (-1.f);
                src2[1] = src2[1] * (-1.f);
                src2[2] = src2[2] * (-1.f);
                src2[3] = src2[3] * (-1.f);
            }
            if (swizzle&(1 << 0x17)) {
                src3[0] = src3[0] * (-1.f);
                src3[1] = src3[1] * (-1.f);
                src3[2] = src3[2] * (-1.f);
                src3[3] = src3[3] * (-1.f);
            }


            //DST[i] = SRC3[i] + SRC2[i]*SRC1[i]
            for (int i = 0; i < 3; ++i) {
                if (!swizzle_DestComponentEnabled(i, swizzle))
                    continue;

                dest[i] = src3[i] + src1[i] * src2[i];
            }
            break;
        }
        default:
            DEBUG("Unhandled instruction: 0x%08x\n",instr);
            break;
        }

        if (increment_pc)
            state->program_counter++;

        /*if (state->program_counter == state->program_counter_end) {
            state->program_counter_end = 0;
            break;
        }*/
    }

}

void RunShader(struct vec4 input[17], int num_attributes, struct OutputVertex *ret)
{
    struct VertexShaderState state;

    //const u32* main = &shader_memory[registers.Get<Regs::VSMainOffset>().offset_words];
    state.program_counter = &GPUshadercodebuffer[GPU_Regs[VSMainOffset] & 0xFFFF];

    // Setup input register table

    float dummy_register = (0.f);
    for (int i = 0; i < 16; i++)
        state.input_register_table[i] = &dummy_register;

    for (int i = 0; i<num_attributes; i++)
        state.input_register_table[getattribute_register_map(i, GPU_Regs[VSInputRegisterMap], GPU_Regs[VSInputRegisterMap + 1])] = &input[i].v[0];

    for (int i = 0; i < 16; i++)
        state.boolean_registers[i] = GPU_Regs[0x2B0]&(1<<i);

    for (int i = 0; i < 4; i++)
        state.address_registers[i] = 0;

    //set up integer
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++)
            state.integer_registers[i][j] = ((GPU_Regs[VS_INTUNIFORM_I0 + i]>>(j*8))&0xFF);
    // Setup output register table
    //struct OutputVertex ret;
    for (int i = 0; i < 7; ++i) {
        u32 output_register_map = GPU_Regs[VSVertexAttributeOutputMap + i];

        u32 semantics[4] = {
            (output_register_map >> 0) & 0x1F, (output_register_map >> 8) & 0x1F,
            (output_register_map >> 16) & 0x1F, (output_register_map >> 24) & 0x1F
        };

        for (int comp = 0; comp < 4; ++comp)
            state.output_register_table[4 * i + comp] = ((float*)ret) + semantics[comp];
    }


    state.status_registers[0] = false;
    state.status_registers[1] = false;
    /*int k;
    for (k = 0; k < 16; k++)state.call_stack[k] = VertexShaderState_INVALID_ADDRESS;
    state.call_stack_pointer = &state.call_stack[1];*/
    Stack_Init(&state.call_stack);
    Stack_Init(&state.call_end_stack);
    Stack_Init(&state.call_type_stack);
    Stack_Push(&state.call_stack, VertexShaderState_INVALID_ADDRESS);
    Stack_Push(&state.call_end_stack, VertexShaderState_INVALID_ADDRESS);

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 4; j++)state.temporary_registers[i].v[j] = (0.f);
    }
    ret->tc0.v[0] = 0.f;
    ret->tc0.v[1] = 0.f;

    ProcessShaderCode(&state);

    GPUDEBUG("Output vertex: pos (%.2f, %.2f, %.2f, %.2f), col(%.2f, %.2f, %.2f, %.2f), tc0(%.2f, %.2f)\n",
             ret->pos.v[0], ret->pos.v[1], ret->pos.v[2], ret->pos.v[3],
             ret->color.v[0], ret->color.v[1], ret->color.v[2], ret->color.v[3],
             ret->tc0.v[0], ret->tc0.v[1]);

    //return ret;
}












static u32 GetComponent(u32 n,u32* data)
{
    if (n < 8) {
        return ((*(data + 1)) >> n * 4) & 0xF;
    } else {
        return ((*(data + 2)) >> (n - 8) * 4) & 0xF;
    }
}

static u32 GetNumElements(u32 n, u32* data)
{
    if (n < 8) {
        u32 temp = *(data + 1);
        return ((temp >> ((n * 4) + 2)) & 0x3) + 1;
    } else {
        return ((*(data + 2)) >> (((n - 8) * 4) + 2 )) & 0x3;
    }
}

static u32 GetFormat(u32 n, u32* data)
{
    if (n < 8) {
        return ((*(data + 1)) >> n * 4) & 0x3;
    } else {
        return ((*(data + 2)) >> (n - 8) * 4) & 0x3;
    }
}
static u32 GetElementSizeInBytes(int n, u32* data)
{
    return (GetFormat(n,data) == Format_FLOAT) ? 4 : (GetFormat(n,data) == Format_SHORT) ? 2 : 1;
}
static u32 GetStride(int n, u32* data)
{
    return GetNumElements(n,data) * GetElementSizeInBytes(n,data);
}
void writeGPUID(u16 ID, u8 mask, u32 size, u32* buffer)
{
    u32 i;
    switch (ID) {
    // It seems like these trigger vertex rendering
    case TriggerDraw:
    case TriggerDrawIndexed: {
        u32* attribute_config = &GPU_Regs[VertexAttributeConfig];
        u32 base_address = (*attribute_config) << 3;
        // Information about internal vertex attributes
        u8* vertex_attribute_sources[16];
        u32 vertex_attribute_strides[16];
        u32 vertex_attribute_formats[16];
        u32 vertex_attribute_elements[16];
        u32 vertex_attribute_element_size[16];

        memset(&vertex_attribute_sources[0], 0, 16);
        memset(&vertex_attribute_strides[0], 0, 16 * 4);
        memset(&vertex_attribute_formats[0], 0, 16 * 4);
        memset(&vertex_attribute_elements[0], 0, 16 * 4);
        memset(&vertex_attribute_element_size[0], 0, 16 * 4);

        //mem_Dbugdump();
        // Setup attribute data from loaders
        //u8 NumTotalAttributes = 0;
        for (int loader = 0; loader < 12; loader++) {
            u32* loader_config = (attribute_config + (loader + 1) * 3);

            u32 load_address = base_address + *loader_config;

            // TODO: What happens if a loader overwrites a previous one's data?
            u8 component_count = loader_config[2] >> 28;
            //NumTotalAttributes = component_count;
            for (int component = 0; component < component_count; component++) {
                u32 attribute_index = GetComponent(component, loader_config);//loader_config.GetComponent(component);
                vertex_attribute_sources[attribute_index] = (u8*)get_pymembuffer(load_address);
                vertex_attribute_strides[attribute_index] = (loader_config[2] >> 16) & 0xFFF;
                vertex_attribute_formats[attribute_index] = GetFormat(attribute_index, attribute_config);
                vertex_attribute_elements[attribute_index] = GetNumElements(attribute_index, attribute_config);
                vertex_attribute_element_size[attribute_index] = GetElementSizeInBytes(attribute_index, attribute_config);
                load_address += GetStride(attribute_index, attribute_config);

            }
        }
        // Load vertices
        bool is_indexed = (ID == TriggerDrawIndexed);
        //const auto& index_info = registers.Get<Regs::IndexArrayConfig>();
        u32 index_info_offset = GPU_Regs[IndexArrayConfig] & 0x7FFFFFFF;
        const u8* index_address_8 = (u8*)get_pymembuffer(base_address + index_info_offset);
        const u16* index_address_16 = (u16*)index_address_8;
        bool index_u16 = (GPU_Regs[IndexArrayConfig] >> 31);
        for (u32 index = 0; index < GPU_Regs[NumVertices]; index++) {
            int vertex = is_indexed ? (index_u16 ? index_address_16[index] : index_address_8[index]) : index;

            if (is_indexed) {
                // TODO: Implement some sort of vertex cache!
            }

            // Initialize data for the current vertex
            struct vec4 input[17];
            u8 NumTotalAttributes = (attribute_config[2] >> 28) + 1;
            for (int i = 0; i < NumTotalAttributes; i++) {
                for (u32 comp = 0; comp < vertex_attribute_elements[i]; comp++) {
                    const u8* srcdata = vertex_attribute_sources[i] + vertex_attribute_strides[i] * vertex + comp * vertex_attribute_element_size[i];
                    const float srcval = (vertex_attribute_formats[i] == 0) ? *(s8*)srcdata :
                                         (vertex_attribute_formats[i] == 1) ? *(u8*)srcdata :
                                         (vertex_attribute_formats[i] == 2) ? *(s16*)srcdata :
                                         *(float*)srcdata;
                    input[i].v[comp] = srcval;
                    GPUDEBUG("Loaded component %x of attribute %x for vertex %x (index %x) from 0x%08x + 0x%08lx + 0x%04lx: %f\n",
                             comp, i, vertex, index,
                             base_address,
                             vertex_attribute_sources[i] - (u8*)get_pymembuffer(base_address),
                             srcdata - vertex_attribute_sources[i],
                             input[i].v[comp]);
                }
            }
            struct OutputVertex output;
            RunShader(input, NumTotalAttributes, &output);
            //VertexShader::OutputVertex output = VertexShader::RunShader(input, attribute_config.GetNumTotalAttributes());

            if (is_indexed) {
                // TODO: Add processed vertex to vertex cache!

            }

            // Send to triangle clipper
            PrimitiveAssembly_SubmitVertex(&output);

            //screen_RenderGPUaddr(GPU_Regs[COLORBUFFER_ADDRESS] << 3);

        }
        break;

    }
    // Load shader program code
    case VSLoadProgramData:
    case VSLoadProgramData + 1:
    case VSLoadProgramData + 2:
    case VSLoadProgramData + 3:
    case VSLoadProgramData + 4:
    case VSLoadProgramData + 5:
    case VSLoadProgramData + 6:
    case VSLoadProgramData + 7:
        if (mask != 0xF) {
            GPUDEBUG("abnormal VSLoadProgramData %0x1 %0x3\n", mask, size);
        }
        for (i = 0; i < size; i++)
            GPUshadercodebuffer[GPU_Regs[VSBeginLoadProgramData]++] = *(buffer + i);
        break;

    case VSLoadSwizzleData:

    case VSLoadSwizzleData + 1:
    case VSLoadSwizzleData + 2:
    case VSLoadSwizzleData + 3:
    case VSLoadSwizzleData + 4:
    case VSLoadSwizzleData + 5:
    case VSLoadSwizzleData + 6:
    case VSLoadSwizzleData + 7:
        if (mask != 0xF) {
            GPUDEBUG("abnormal VSLoadSwizzleData %0x1 %0x3\n", mask, size);
        }
        for (i = 0; i < size; i++)
            swizzle_data[GPU_Regs[VSBeginLoadSwizzleData]++] = *(buffer + i);
        break;

    case VSresttriangel:
        if (*buffer & 0x1) //todo more checks
            buffer_index = 0;
        updateGPUintreg(*buffer, ID, mask);
        break;
    case VSFloatUniformSetup:
        updateGPUintreg(*buffer, ID, mask);
        buffer++;
        size--;
    case VSFloatUniformSetup + 1:
    case VSFloatUniformSetup + 2:
    case VSFloatUniformSetup + 3:
    case VSFloatUniformSetup + 4:
    case VSFloatUniformSetup + 5:
    case VSFloatUniformSetup + 6:
    case VSFloatUniformSetup + 7:
    case VSFloatUniformSetup + 8:
        for (i = 0; i < size; i++) {
            VSFloatUniformSetuptembuffer[VSFloatUniformSetuptembuffercurrent++] = *(buffer + i);
            bool isfloat32 = (GPU_Regs[VSFloatUniformSetup] >> 31) == 1;

            if (VSFloatUniformSetuptembuffercurrent == (isfloat32 ? 4 : 3)) {
                VSFloatUniformSetuptembuffercurrent = 0;
                u8 index = GPU_Regs[VSFloatUniformSetup] & 0x7F;
                if (index > 95) {
                    GPUDEBUG("Invalid VS uniform index %02x\n", index);
                    break;
                }
                // NOTE: The destination component order indeed is "backwards"
                if (isfloat32) {
                    const_vectors[index].v[3] = *(float*)(&VSFloatUniformSetuptembuffer[0]);
                    const_vectors[index].v[2] = *(float*)(&VSFloatUniformSetuptembuffer[1]);
                    const_vectors[index].v[1] = *(float*)(&VSFloatUniformSetuptembuffer[2]);
                    const_vectors[index].v[0] = *(float*)(&VSFloatUniformSetuptembuffer[3]);
                } else {
                    f24to32(VSFloatUniformSetuptembuffer[0] >> 8, &const_vectors[index].v[3]);
                    f24to32(((VSFloatUniformSetuptembuffer[0] & 0xFF) << 16) | ((VSFloatUniformSetuptembuffer[1] >> 16) & 0xFFFF), &const_vectors[index].v[2]);
                    f24to32(((VSFloatUniformSetuptembuffer[1] & 0xFFFF) << 8) | ((VSFloatUniformSetuptembuffer[2] >> 24) & 0xFF), &const_vectors[index].v[1]);
                    f24to32(VSFloatUniformSetuptembuffer[2] & 0xFFFFFF, &const_vectors[index].v[0]);

                }

                GPUDEBUG("Set uniform %02x to (%f %f %f %f)\n", index,
                         const_vectors[index].v[0], const_vectors[index].v[1], const_vectors[index].v[2],
                         const_vectors[index].v[3]);
                // TODO: Verify that this actually modifies the register!
                GPU_Regs[VSFloatUniformSetup]++;
            }
        }
        break;
    case TRIGGER_IRQ:
        if (mask != 0xF && size != 0 && *buffer == 0x12345678) {
            GPUDEBUG("abnormal TRIGGER_IRQ %0x1 %0x3 %08x\n", mask, size, *buffer);
        }
        gpu_SendInterruptToAll(5);//P3D

        break;
    default:
        updateGPUintreg(*buffer, ID, mask);
    }
}


void runGPU_Commands(u8* buffer, u32 sizea)
{
    u32 i;
    for (i = 0; i < sizea; i += 8) {
        u32 cmd = *(u32*)(buffer + 4 + i);
        u32 dataone = *(u32*)(buffer + i);
        u16 ID = cmd & 0xFFFF;
        u8 mask = (cmd >> 16) & 0xF;
        u16 size = (cmd >> 20) & 0x7FF;
        u8 grouping = (cmd >> 31);
        u32 datafild[0x800]; //maximal size
        datafild[0] = dataone;
#ifdef GSP_ENABLE_LOG
		GPUDEBUG("cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
#endif
        int j;
        for (j = 0; j < size; j++) {
            datafild[1 + j] = *(u32*)(buffer + 8 + i);
#ifdef GSP_ENABLE_LOG
            GPUDEBUG("data %08x\n", datafild[1 + j]);
#endif
            i += 4;
        }
        if (size & 0x1) {
            u32 data = *(u32*)(buffer + 8 + i);
#ifdef GSP_ENABLE_LOG
            GPUDEBUG("padding data %08x\n", data);
#endif
            i += 4;
        }
        if (mask != 0) {
#ifdef GSP_ENABLE_LOG
            if (size > 0 && mask != 0xF)
                GPUDEBUG("masked data? cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
#endif
            if (grouping) {
                for (j = 0; j <= size; j++)writeGPUID(ID + j, mask, 1, &datafild[j]);
            } else {
                writeGPUID(ID, mask, size + 1, datafild);
            }
        } else {
#ifdef GSP_ENABLE_LOG
            GPUDEBUG("masked out data is ignored cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
#endif
        }

    }
}

void updateFramebufferaddr(u32 addr,bool bot)
{
    u32 active_framebuf = mem_Read32(addr); //"0=first, 1=second"
    u32 framebuf0_vaddr = mem_Read32(addr + 4); //"Framebuffer virtual address, for the main screen this is the 3D left framebuffer"
    u32 framebuf1_vaddr = mem_Read32(addr + 8); //"For the main screen: 3D right framebuffer address"
    u32 framebuf_widthbytesize = mem_Read32(addr + 12); //"Value for 0x1EF00X90, controls framebuffer width"
    u32 format = mem_Read32(addr + 16); //"Framebuffer format, this u16 is written to the low u16 for LCD register 0x1EF00X70."
    u32 framebuf_dispselect = mem_Read32(addr + 20); //"Value for 0x1EF00X78, controls which framebuffer is displayed"
    u32 unk = mem_Read32(addr + 24); //"?"

    if (!bot) {
        if(active_framebuf == 0)
            gpu_WriteReg32(RGBuponeleft, convertvirtualtopys(framebuf0_vaddr));
        else
            gpu_WriteReg32(RGBuptwoleft, convertvirtualtopys(framebuf0_vaddr));

        if(framebuf1_vaddr == 0)
        {
            if(active_framebuf == 0)
                gpu_WriteReg32(RGBuponeright, convertvirtualtopys(framebuf0_vaddr));
            else
                gpu_WriteReg32(RGBuptworight, convertvirtualtopys(framebuf0_vaddr));
        }
        else
        {
            if(active_framebuf == 0)
                gpu_WriteReg32(RGBuponeright, convertvirtualtopys(framebuf1_vaddr));
            else
                gpu_WriteReg32(RGBuptworight, convertvirtualtopys(framebuf1_vaddr));
        }

        gpu_WriteReg32(framestridetop, framebuf_widthbytesize);

        gpu_WriteReg32(frameformattop, format);
        gpu_WriteReg32(frameselecttop, framebuf_dispselect);
    } else {
        if(active_framebuf == 0)
            gpu_WriteReg32(RGBdownoneleft, convertvirtualtopys(framebuf0_vaddr));
        else
            gpu_WriteReg32(RGBdowntwoleft, convertvirtualtopys(framebuf0_vaddr));

        gpu_WriteReg32(framestridebot, framebuf_widthbytesize);

        gpu_WriteReg32(frameformatbot, format);
        gpu_WriteReg32(frameselectbot, framebuf_dispselect);
    }
    return;
}

void updateFramebuffer()
{
    //we use the last in buffer with flag set
    int i;
    for (i = 0; i < 4; i++) {
        u8 *baseaddrtop = (u8*)(GSPsharedbuff + 0x200 + i * 0x80); //top
        if (*(u8*)(baseaddrtop + 1)) {
            *(u8*)(baseaddrtop + 1) = 0;
            if (*(u8*)(baseaddrtop))
                baseaddrtop += 0x20; //get the other
            else
                baseaddrtop += 0x4;

            u32 active_framebuf = *(u32*)(baseaddrtop); //"0=first, 1=second"
            u32 framebuf0_vaddr = *(u32*)(baseaddrtop + 4); //"Framebuffer virtual address, for the main screen this is the 3D left framebuffer"
            u32 framebuf1_vaddr = *(u32*)(baseaddrtop + 8); //"For the main screen: 3D right framebuffer address"
            u32 framebuf_widthbytesize = *(u32*)(baseaddrtop + 12); //"Value for 0x1EF00X90, controls framebuffer width"
            u32 format = *(u32*)(baseaddrtop + 16); //"Framebuffer format, this u16 is written to the low u16 for LCD register 0x1EF00X70."
            u32 framebuf_dispselect = *(u32*)(baseaddrtop + 20); //"Value for 0x1EF00X78, controls which framebuffer is displayed"
            u32 unk = *(u32*)(baseaddrtop + 24); //"?"

            if(active_framebuf == 0)
                gpu_WriteReg32(RGBuponeleft, convertvirtualtopys(framebuf0_vaddr));
            else
                gpu_WriteReg32(RGBuptwoleft, convertvirtualtopys(framebuf0_vaddr));

            if(framebuf1_vaddr == 0)
            {
                if(active_framebuf == 0)
                    gpu_WriteReg32(RGBuponeright, convertvirtualtopys(framebuf0_vaddr));
                else
                    gpu_WriteReg32(RGBuptworight, convertvirtualtopys(framebuf0_vaddr));
            }
            else
            {
                if(active_framebuf == 0)
                    gpu_WriteReg32(RGBuponeright, convertvirtualtopys(framebuf1_vaddr));
                else
                    gpu_WriteReg32(RGBuptworight, convertvirtualtopys(framebuf1_vaddr));
            }

            gpu_WriteReg32(framestridetop, framebuf_widthbytesize);

            gpu_WriteReg32(frameformattop, format);
            gpu_WriteReg32(frameselecttop, framebuf_dispselect);
        }
        u8 *baseaddrbot = (u8*)(GSPsharedbuff + 0x240 + i * 0x80); //bot
        if (*(u8*)(baseaddrbot + 1)) {
            *(u8*)(baseaddrbot + 1) = 0;
            if (*(u8*)(baseaddrbot))
                baseaddrbot += 0x20; //get the other
            else
                baseaddrbot += 0x4;

            u32 active_framebuf = *(u32*)(baseaddrbot); //"0=first, 1=second"
            u32 framebuf0_vaddr = *(u32*)(baseaddrbot + 4); //"Framebuffer virtual address, for the main screen this is the 3D left framebuffer"
            u32 framebuf1_vaddr = *(u32*)(baseaddrbot + 8); //"For the main screen: 3D right framebuffer address"
            u32 framebuf_widthbytesize = *(u32*)(baseaddrbot + 12); //"Value for 0x1EF00X90, controls framebuffer width"
            u32 format = *(u32*)(baseaddrbot + 16); //"Framebuffer format, this u16 is written to the low u16 for LCD register 0x1EF00X70."
            u32 framebuf_dispselect = *(u32*)(baseaddrbot + 20); //"Value for 0x1EF00X78, controls which framebuffer is displayed"
            u32 unk = *(u32*)(baseaddrbot + 24); //"?"

            if(active_framebuf == 0)
                gpu_WriteReg32(RGBdownoneleft, convertvirtualtopys(framebuf0_vaddr));
            else
                gpu_WriteReg32(RGBdowntwoleft, convertvirtualtopys(framebuf0_vaddr));

            gpu_WriteReg32(framestridebot, framebuf_widthbytesize);

            gpu_WriteReg32(frameformatbot, format);
            gpu_WriteReg32(frameselectbot, framebuf_dispselect);
        }
    }

    return;
}

u8* get_pymembuffer(u32 addr)
{
    if (addr >= 0x18000000 && addr < 0x18600000)return VRAMbuff + (addr - 0x18000000);
    if (addr >= 0x20000000 && addr < 0x28000000)return LINEmembuffer + (addr - 0x20000000);
    return NULL;
}
u32 get_py_memrestsize(u32 addr)
{
    if (addr >= 0x18000000 && addr < 0x18600000)return addr - 0x18000000;
    if (addr >= 0x20000000 && addr < 0x28000000)return addr - 0x20000000;
    return 0;
}
