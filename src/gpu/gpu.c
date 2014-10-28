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

#define logGSPparser

u32 GPUregs[0xFFFF]; //do they all exist don't know but well

u32 GPUshadercodebuffer[0xFFFF]; //how big is the buffer?

u32 swizzle_data[0xFFFF]; //how big is the buffer?

struct vec4 vectors[96];

/*u8* IObuffer;
u8* LINEmembuffer;
u8* VRAMbuff;
u8* GSPsharedbuff;*/

extern int noscreen;


void initGPU()
{
    IObuffer = malloc(0x420000);
    LINEmembuffer = malloc(0x8000000);
    VRAMbuff = malloc(0x800000);//malloc(0x600000);
    GSPsharedbuff = malloc(GSPsharebuffsize);

    memset(IObuffer, 0, 0x420000);
    memset(LINEmembuffer, 0, 0x8000000);
    memset(VRAMbuff, 0, 0x600000);
    memset(GSPsharedbuff, 0, GSPsharebuffsize);
    memset(GPUshadercodebuffer, 0, 0xFFFF*4);

    GPUwritereg32(frameselectoben, 0);
    GPUwritereg32(RGBuponeleft, 0x18000000);
    GPUwritereg32(RGBuptwoleft, 0x18000000 + 0x46500 * 1);
    GPUwritereg32(RGBuponeright, 0x18000000 + 0x46500 * 2);
    GPUwritereg32(RGBuptworight, 0x18000000 + 0x46500 * 3);
    GPUwritereg32(frameselectbot, 0);
    GPUwritereg32(RGBdownoneleft, 0x18000000 + 0x46500 * 4);
    GPUwritereg32(RGBdowntwoleft, 0x18000000 + 0x46500 * 5);
}

u32 convertvirtualtopys(u32 addr) //todo
{
    if (addr >= 0x14000000 && addr < 0x1C000000)return addr + 0xC000000; //FCRAM
    if (addr >= 0x1F000000 && addr < 0x1F600000)return addr - 0x7000000; //VRAM
    GPUDEBUG("can't convert vitual to py %08x\n",addr);
    return 0;
}
void GPUwritereg32(u32 addr, u32 data) //1eb00000 + addr
{
    GPUDEBUG("GPU write %08x to %08x\n",data,addr);
    if (addr >= 0x420000) {
        GPUDEBUG("write out of range write\r\n");
        return;
    }
    *(uint32_t*)(&IObuffer[addr]) = data;
    switch (addr) {
    default:
        break;
    }
}
u32 GPUreadreg32(u32 addr)
{
    //GPUDEBUG("GPU read %08x\n", addr);
    if (addr >= 0x420000) {
        GPUDEBUG("read out of range write\r\n");
        return 0;
    }
    return *(uint32_t*)(&IObuffer[addr]);
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
u32 getsizeofframebuffer(u32 val)
{
    switch (val) {
    case 0x0201:
        return 3;
    default:
        GPUDEBUG("unknown len %08x\n", val);
        return 3;
    }
}

u32 renderaddr = 0;
u32 unknownaddr = 0;





void updateGPUintreg(u32 data,u32 ID,u8 mask)
{
    int i;
    for (i = 0; i < 4; i++)
    {
        if (mask&(1 << i))
        {
            GPUregs[ID] = (GPUregs[ID] & ~(0xFF << (8 * i))) | (data & (0xFF << (8 * i)));
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

struct VertexShaderState {
    u32* program_counter;
    
    float* input_register_table[16];
    float* output_register_table[7 * 4];
    
    struct vec4  temporary_registers[16];
    bool status_registers[2];
    
    u32 call_stack[8]; // TODO: What is the maximal call stack depth?
    u32* call_stack_pointer;
};

u32 getattribute_register_map(u32 reg,u32 data1,u32 data2)
{
    if (reg < 8)
    {
        return (data1 >> (reg*4))&0xF;
    }
    else
    {
        return (data2 >> (reg * 4)) & 0xF;
    }
}






static struct OutputVertex buffer[2];
static int buffer_index = 0; // TODO: reset this on emulation restart

void PrimitiveAssembly_SubmitVertex(struct OutputVertex* vtx)
{
    u32 topology = (GPUregs[TriangleTopology] >> 8) & 0x3;
    switch (topology) {
    case 0://List:
    case 3://ListIndexed:
        if (buffer_index < 2) {
            memcpy(&buffer[buffer_index++], vtx, sizeof(struct OutputVertex));
            //buffer[buffer_index++] = vtx;
        }
        else {
            buffer_index = 0;

            Clipper_ProcessTriangle(&buffer[0], &buffer[1], vtx);
        }
        break;

    case 2://Fan:
        if (buffer_index == 2) {
            buffer_index = 0;

           Clipper_ProcessTriangle(&buffer[0], &buffer[1], vtx);

            //buffer[1] = vtx;
           memcpy(&buffer[1], vtx, sizeof(struct OutputVertex));
        }
        else {
            //buffer[buffer_index++] = vtx;
            memcpy(&buffer[buffer_index++], vtx, sizeof(struct OutputVertex));
        }
        break;

      case 1: //Strip
             if (buffer_index < 2) {
                //buffer[buffer_index++] = vtx;
                 memcpy(&buffer[buffer_index++], vtx, sizeof(struct OutputVertex));
            }
            else {
                Clipper_ProcessTriangle(&buffer[0], &buffer[1], vtx);
                // TODO: This could be done a lot more efficiently...
                /*buffer[buffer_index] = vtx;
                std::swap(buffer[0], buffer[1]);
                std::swap(buffer[1], buffer[2]);*/
                /*buffer[0] = buffer[1];
                buffer[1] = vtx;*/
                memcpy(&buffer[0], &buffer[1], sizeof(struct OutputVertex));
                memcpy(&buffer[1], vtx, sizeof(struct OutputVertex));
            }
            break;
    default:
        GPUDEBUG("Unknown triangle mode %x\n", (int)topology);
        break;
    }
}








#define SHDR_ADD 0x0
#define SHDR_DP3 0x1
#define SHDR_DP4 0x2

#define SHDR_MUL 0x8

#define SHDR_MAX  0xC
#define SHDR_MIN  0xD
#define SHDR_RCP  0xE
#define SHDR_RSQ  0xF

#define SHDR_MOV  0x13

#define SHDR_RET  0x21
#define SHDR_FLS  0x22
#define SHDR_CALL  0x24

u32 instr_common_src1(u32 hex)
{
    return (hex >> 0xC) & 0x7F;
}
u32 instr_common_dest(u32 hex)
{
    return (hex >> 0x15) & 0x1F;
}
u32 instr_common_src2(u32 hex)
{
    return (hex >> 0x7) & 0x1F;
}
u32 instr_common_operand_desc_id(u32 hex)
{
    return hex & 0x1F;
}
u32 instr_opcode(u32 hex)
{
    return (hex >> 0x1A);
}
u32 instr_flow_control_offset_words(u32 hex)
{
    return (hex>>0xa)&0xFFF;
}
bool swizzle_DestComponentEnabled(int i, u32 swizzle)
{
    return (swizzle & (0x8 >> i));
}

#define printfunc

void ProcessShaderCode(struct VertexShaderState* state) {
    while (true) {
        bool increment_pc = true;
        bool exit_loop = false;
        u32 instr = *(u32*)state->program_counter;

        u32 instr_common_src1v = instr_common_src1(instr);
        const float* src1_ = (instr_common_src1v < 0x10) ? state->input_register_table[instr_common_src1v]
            : (instr_common_src1v < 0x20) ? &state->temporary_registers[instr_common_src1v - 0x10].v[0]
            : (instr_common_src1v < 0x80) ? &vectors[instr_common_src1v - 0x20].v[0]
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
            src1_[(int)((swizzle>>11)&0x3)],
            src1_[(int)((swizzle >> 9) & 0x3)],
            src1_[(int)((swizzle >> 7) & 0x3)],
            src1_[(int)((swizzle >> 5) & 0x3)],
        };
        const float src2[4] = {
            src2_[(int)((swizzle >> 20) & 0x3)],
            src2_[(int)((swizzle >> 18) & 0x3)],
            src2_[(int)((swizzle >> 16) & 0x3)],
            src2_[(int)((swizzle >> 14) & 0x3)],
        };

        if (swizzle&0x10) {
            src1[0] = src1[0] * (-1.f);
            src1[1] = src1[1] * (-1.f);
            src1[2] = src1[2] * (-1.f);
            src1[3] = src1[3] * (-1.f);
        }

        switch (instr_opcode(instr)) {
        case SHDR_ADD:
        {
#ifdef printfunc
                         GPUDEBUG("ADD %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
                                         for (int i = 0; i < 4; ++i) {
                                             if (!swizzle_DestComponentEnabled(i, swizzle))
                                                 continue;

                                             dest[i] = src1[i] + src2[i];
                                         }

                                         break;
        }

        case SHDR_MUL:
        {
#ifdef printfunc
                         GPUDEBUG("MUL %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
#endif
                                         for (int i = 0; i < 4; ++i) {
                                             if (!swizzle_DestComponentEnabled(i, swizzle))
                                                 continue;

                                             dest[i] = src1[i] * src2[i];
                                         }

                                         break;
        }

        case SHDR_DP3:
        case SHDR_DP4:
        {
#ifdef printfunc
                         GPUDEBUG("DP3/4 %02X %02X %02X %08x\n", instr_common_destv, instr_common_src1v, instr_common_src2v, swizzle);
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

            // Reciprocal
        case SHDR_RCP:
        {
#ifdef printfunc
                         GPUDEBUG("RCP %02X %02X %08x\n", instr_common_destv, instr_common_src1v, swizzle);
#endif
                                         for (int i = 0; i < 4; ++i) {
                                             if (!swizzle_DestComponentEnabled(i, swizzle))
                                                 continue;

                                             // TODO: Be stable against division by zero!
                                             // TODO: I think this might be wrong... we should only use one component here
                                             dest[i] = (1.0 / src1[i]);
                                         }

                                         break;
        }

            // Reciprocal Square Root
        case SHDR_RSQ:
        {
#ifdef printfunc
                         GPUDEBUG("RSQ %02X %02X %08x\n", instr_common_destv, instr_common_src1v, swizzle);
#endif     
                                         for (int i = 0; i < 4; ++i) {
                                             if (!swizzle_DestComponentEnabled(i, swizzle))
                                                 continue;

                                             // TODO: Be stable against division by zero!
                                             // TODO: I think this might be wrong... we should only use one component here
                                             dest[i] = (1.0 / sqrt(src1[i]));
                                         }

                                         break;
        }

        case SHDR_MOV:
        {
#ifdef printfunc
                         GPUDEBUG("MOV %02X %02X %08x\n", instr_common_destv, instr_common_src1v, swizzle);
#endif   
                                         for (int i = 0; i < 4; ++i) {
                                             if (!swizzle_DestComponentEnabled(i, swizzle))
                                                 continue;

                                             dest[i] = src1[i];
                                         }
                                         break;
        }

        case SHDR_RET:
#ifdef printfunc
            GPUDEBUG("RET\n");
#endif
            *state->call_stack_pointer = VertexShaderState_INVALID_ADDRESS;
            state->call_stack_pointer--;
            if (*state->call_stack_pointer == VertexShaderState_INVALID_ADDRESS) {
                exit_loop = true;
            }
            else {
                state->program_counter = &GPUshadercodebuffer[*state->call_stack_pointer];
           }

            break;

        case SHDR_CALL:
#ifdef printfunc
            GPUDEBUG("CALL %08x\n",instr_flow_control_offset_words(instr));
#endif   
            increment_pc = false;

            //_dbg_assert_(GPU, state.call_stack_pointer - state.call_stack < sizeof(state.call_stack)); //todo

            *state->call_stack_pointer = ((u32)(uintptr_t)state->program_counter - (u32)(uintptr_t)(&GPUshadercodebuffer[0])) / 4;
            state->call_stack_pointer++;
            // TODO: Does this offset refer to the beginning of shader memory?
            state->program_counter = &GPUshadercodebuffer[instr_flow_control_offset_words(instr)];
            break;

        case SHDR_FLS:
#ifdef printfunc
            GPUDEBUG("FLS\n");
#endif   
            // TODO: Do whatever needs to be done here?
            break;

        default:
            GPUDEBUG("Unhandled instruction: 0x%08x\n",instr);
            break;
        }

        if (increment_pc)
            state->program_counter++;

        if (exit_loop)
            break;
    }
}






void RunShader(struct vec4 input[17], int num_attributes, struct OutputVertex *ret)
{
    struct VertexShaderState state;

    //const u32* main = &shader_memory[registers.Get<Regs::VSMainOffset>().offset_words];
    state.program_counter = (u32*)(uintptr_t)((u32)(uintptr_t)(&GPUshadercodebuffer[0]) + (u16)(uintptr_t)GPUregs[VSMainOffset]*4);

    // Setup input register table

    float dummy_register = (0.f);
    for (int i = 0; i < 16; i++)state.input_register_table[i] = &dummy_register;
    for (int i = 0; i<num_attributes; i++)
        state.input_register_table[getattribute_register_map(i, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[i].v[0];
    /*if (num_attributes > 0) state.input_register_table[getattribute_register_map(0, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[0].v[0];
    if (num_attributes > 1) state.input_register_table[getattribute_register_map(1, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[1].v[0];
    if (num_attributes > 2) state.input_register_table[getattribute_register_map(2, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[2].v[0];
    if (num_attributes > 3) state.input_register_table[getattribute_register_map(3, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[3].v[0];
    if (num_attributes > 4) state.input_register_table[getattribute_register_map(4, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[4].v[0];
    if (num_attributes > 5) state.input_register_table[getattribute_register_map(5, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[5].v[0];
    if (num_attributes > 6) state.input_register_table[getattribute_register_map(6, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[6].v[0];
    if (num_attributes > 7) state.input_register_table[getattribute_register_map(7, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[7].v[0];
    if (num_attributes > 8) state.input_register_table[getattribute_register_map(8, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[8].v[0];
    if (num_attributes > 9) state.input_register_table[getattribute_register_map(9, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[9].v[0];
    if (num_attributes > 10) state.input_register_table[getattribute_register_map(10, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[10].v[0];
    if (num_attributes > 11) state.input_register_table[getattribute_register_map(11, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[11].v[0];
    if (num_attributes > 12) state.input_register_table[getattribute_register_map(12, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[12].v[0];
    if (num_attributes > 13) state.input_register_table[getattribute_register_map(13, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[13].v[0];
    if (num_attributes > 14) state.input_register_table[getattribute_register_map(14, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[14].v[0];
    if (num_attributes > 15) state.input_register_table[getattribute_register_map(15, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[15].v[0];*/
    
    // Setup output register table
    //struct OutputVertex ret;
    for (int i = 0; i < 7; ++i) {
        u32 output_register_map = GPUregs[VSVertexAttributeOutputMap + i];

        u32 semantics[4] = {
            (output_register_map >> 0) & 0x1F, (output_register_map >> 8) & 0x1F,
            (output_register_map >> 16) & 0x1F, (output_register_map >> 24) & 0x1F
        };

        for (int comp = 0; comp < 4; ++comp)
            state.output_register_table[4 * i + comp] = ((float*)ret) + semantics[comp];
    }


    state.status_registers[0] = false;
    state.status_registers[1] = false;
    int k;
    for (k = 0; k < 8; k++)state.call_stack[k] = VertexShaderState_INVALID_ADDRESS;
    state.call_stack_pointer = &state.call_stack[1];

    for (int i = 0; i < 16; i++)
    {
        for (int j = 0; j < 4; j++)state.temporary_registers[i].v[j] = (0.f);
    }

    ProcessShaderCode(&state);

    GPUDEBUG("Output vertex: pos (%.2f, %.2f, %.2f, %.2f), col(%.2f, %.2f, %.2f, %.2f), tc0(%.2f, %.2f)\n",
        ret->pos.v[0], ret->pos.v[1], ret->pos.v[2], ret->pos.v[3],
        ret->color.v[0], ret->color.v[1], ret->color.v[2], ret->color.v[3],
        ret->tc0.v[0], ret->tc0.v[1]);

    //return ret;
}












u32 GetComponent(u32 n,u32* data)
{
    if (n < 8)
    {
        return ((*(data + 1)) >> n * 4) & 0xF;
    }
    else
    {
        return ((*(data + 2)) >> (n - 8) * 4) & 0xF;
    }
}

u32 GetNumElements(u32 n, u32* data)
{
    if (n < 8)
    {
        u32 temp = *(data + 1);
        return ((temp >> ((n * 4) + 2)) & 0x3) + 1;
    }
    else
    {
        return ((*(data + 2)) >> (((n - 8) * 4) + 2 )) & 0x3;
    }
}

u32 GetFormat(u32 n, u32* data)
{
    if (n < 8)
    {
        return ((*(data + 1)) >> n * 4) & 0x3;
    }
    else
    {
        return ((*(data + 2)) >> (n - 8) * 4) & 0x3;
    }
}
u32 GetElementSizeInBytes(int n, u32* data) 
{
    return (GetFormat(n,data) == Format_FLOAT) ? 4 : (GetFormat(n,data) == Format_SHORT) ? 2 : 1;
}
u32 GetStride(int n, u32* data)
{
    return GetNumElements(n,data) * GetElementSizeInBytes(n,data);
}
void writeGPUID(u16 ID, u8 mask, u32 size, u32* buffer)
{
    u32 i;
    switch (ID)
    {
    // It seems like these trigger vertex rendering
    case TriggerDraw:
    case TriggerDrawIndexed:
    {
                               u32* attribute_config = &GPUregs[VertexAttributeConfig];
                               u32 base_address = (*attribute_config) << 3;
                               // Information about internal vertex attributes
                               u8* vertex_attribute_sources[16];
                               u32 vertex_attribute_strides[16];
                               u32 vertex_attribute_formats[16];
                               u32 vertex_attribute_elements[16];
                               u32 vertex_attribute_element_size[16];

                               mem_Dbugdump();
                               // Setup attribute data from loaders
                               for (int loader = 0; loader < 12; loader++) {
                                   u32* loader_config = (attribute_config + (loader + 1) * 3);

                                   u32 load_address = base_address + *loader_config;

                                   // TODO: What happens if a loader overwrites a previous one's data?
                                   u8 component_count = loader_config[2] >> 28;
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
                               u32 index_info_offset = GPUregs[IndexArrayConfig] & 0x7FFFFFFF;
                               const u8* index_address_8 = (u8*)get_pymembuffer(base_address + index_info_offset);
                               const u16* index_address_16 = (u16*)index_address_8;
                               bool index_u16 = (GPUregs[IndexArrayConfig] >> 31);
                               for (u32 index = 0; index < GPUregs[NumVertices]; index++)
                               {
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
        if (mask != 0xF)
        {
            GPUDEBUG("abnormal VSLoadProgramData %0x1 %0x3\n", mask, size);
        }
        for (i = 0; i < size; i++)
            GPUshadercodebuffer[GPUregs[VSBeginLoadProgramData]++] = *(buffer + i);
        break;

    case VSLoadSwizzleData:
    case VSLoadSwizzleData + 1:
    case VSLoadSwizzleData + 2:
    case VSLoadSwizzleData + 3:
    case VSLoadSwizzleData + 4:
    case VSLoadSwizzleData + 5:
    case VSLoadSwizzleData + 6:
    case VSLoadSwizzleData + 7:
        if (mask != 0xF)
        {
            GPUDEBUG("abnormal VSLoadSwizzleData %0x1 %0x3\n", mask, size);
        }
        for (i = 0; i < size; i++)
            swizzle_data[GPUregs[VSBeginLoadSwizzleData]++] = *(buffer + i);
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
        for (i = 0; i < size; i++)
        {
            VSFloatUniformSetuptembuffer[VSFloatUniformSetuptembuffercurrent++] = *(buffer + i);
            bool isfloat32 = (GPUregs[VSFloatUniformSetup] >> 31) == 1;

            if (VSFloatUniformSetuptembuffercurrent == (isfloat32 ? 4 : 3))
            {
                VSFloatUniformSetuptembuffercurrent = 0;
                u8 index = GPUregs[VSFloatUniformSetup] & 0x7F;
                if (index > 95) {
                    GPUDEBUG("Invalid VS uniform index %02x\n", index);
                    break;
                }
                // NOTE: The destination component order indeed is "backwards"
                if (isfloat32) {
                    vectors[index].v[3] = *(float*)(&VSFloatUniformSetuptembuffer[0]);
                    vectors[index].v[2] = *(float*)(&VSFloatUniformSetuptembuffer[1]);
                    vectors[index].v[1] = *(float*)(&VSFloatUniformSetuptembuffer[2]);
                    vectors[index].v[0] = *(float*)(&VSFloatUniformSetuptembuffer[3]);
                }
                else {
                    f24to32(VSFloatUniformSetuptembuffer[0] >> 8, &vectors[index].v[3]);
                    f24to32(((VSFloatUniformSetuptembuffer[0] & 0xFF) << 16) | ((VSFloatUniformSetuptembuffer[1] >> 16) & 0xFFFF), &vectors[index].v[2]);
                    f24to32(((VSFloatUniformSetuptembuffer[1] & 0xFFFF) << 8) | ((VSFloatUniformSetuptembuffer[2] >> 24) & 0xFF), &vectors[index].v[1]);
                    f24to32(VSFloatUniformSetuptembuffer[2] & 0xFFFFFF, &vectors[index].v[0]);

                }

                GPUDEBUG("Set uniform %02x to (%f %f %f %f)\n", index,
                    vectors[index].v[0], vectors[index].v[1], vectors[index].v[2],
                    vectors[index].v[3]);
                // TODO: Verify that this actually modifies the register!
                GPUregs[VSFloatUniformSetup]++;
            }
        }
        break;
    case TRIGGER_IRQ:
        if (mask != 0xF && size != 0 && *buffer == 0x12345678)
        {
            GPUDEBUG("abnormal TRIGGER_IRQ %0x1 %0x3 %08x\n", mask, size, *buffer);
        }
        sendGPUinterall(5);//P3D

        break;
    default:
        updateGPUintreg(*buffer, ID, mask);
    }
}


void runGPU_Commands(u8* buffer, u32 sizea)
{
    u32 i;
    for (i = 0; i < sizea; i += 8)
    {
        u32 cmd = *(u32*)(buffer + 4 + i);
        u32 dataone = *(u32*)(buffer + i);
        u16 ID = cmd & 0xFFFF;
        u16 mask = (cmd >> 16) & 0xF;
        u16 size = (cmd >> 20) & 0x7FF;
        u8 grouping = (cmd >> 31);
        u32 datafild[0x800]; //maximal size
        datafild[0] = dataone;
#ifdef logGSPparser
        GPUDEBUG("cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "")
#endif
        int j;
        for (j = 0; j < size; j++)
        {
            datafild[1 + j] = *(u32*)(buffer + 8 + i);
#ifdef logGSPparser
            GPUDEBUG("data %08x\n", datafild[1 + j]);
#endif
            i += 4;
        }
        if (size & 0x1)
        {
            u32 data = *(u32*)(buffer + 8 + i);
#ifdef logGSPparser
            GPUDEBUG("padding data %08x\n", data);
#endif
            i += 4;
        }
        if (mask != 0)
        {
            if (size > 0 && mask != 0xF)
                GPUDEBUG("masked data? cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
            if (grouping)
            {
                for (j = 0; j <= size; j++)writeGPUID(ID + j, mask, 1, &datafild[j]);
            }
            else
            {
                writeGPUID(ID, mask, size + 1, datafild);
            }
        }
        else
        {
            GPUDEBUG("masked out data is ignored cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
        }

    }
}


void updateFramebufferaddr(u32 addr,bool bot)
{
    //we use the last in buffer with flag set
    if (!bot) {
            if ((mem_Read32(addr + 4) & 0x1) == 0)
                GPUwritereg32(RGBuponeleft, convertvirtualtopys(mem_Read32(addr + 8)));
            else
                GPUwritereg32(RGBuptwoleft, convertvirtualtopys(mem_Read32(addr + 8)));
            GPUwritereg32(frameselectoben, mem_Read32(addr + 0x18));
            u32 u90 = mem_Read32(addr + 0x10);
            u32 format = mem_Read32(addr + 0x14);
            int i = 0;
            //the rest is todo
        }
        else
        {
            if ((mem_Read32(addr + 4)& 0x1) == 0)
                GPUwritereg32(RGBdownoneleft, mem_Read32(addr + 8));
            else
                GPUwritereg32(RGBdowntwoleft, mem_Read32(addr + 8));
            GPUwritereg32(frameselectbot, mem_Read32(addr + 0x18));
            //the rest is todo
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
            if ((*(u32*)(baseaddrtop)& 0x1) == 0)
                GPUwritereg32(RGBuponeleft, convertvirtualtopys(*(u32*)(baseaddrtop + 4)));
            else
                GPUwritereg32(RGBuptwoleft, convertvirtualtopys(*(u32*)(baseaddrtop + 4)));
            GPUwritereg32(frameselectoben, *(u32*)(baseaddrtop + 0x14));
            u32 u90 =*(u32*)(baseaddrtop + 0xC);
            u32 format = *(u32*)(baseaddrtop + 0x10);
            int i = 0;
            //the rest is todo
        }
        u8 *baseaddrbot = (u8*)(GSPsharedbuff + 0x240 + i * 0x80); //bot
        if (*(u8*)(baseaddrbot + 1)) {
            *(u8*)(baseaddrbot + 1) = 0;
            if (*(u8*)(baseaddrbot))
                baseaddrbot += 0x20; //get the other
            else
                baseaddrbot += 0x4;
            if ((*(u32*)(baseaddrbot) &0x1) == 0)
                GPUwritereg32(RGBdownoneleft, convertvirtualtopys(*(u32*)(baseaddrbot + 4)));
            else
                GPUwritereg32(RGBdowntwoleft, convertvirtualtopys(*(u32*)(baseaddrbot + 4)));
            GPUwritereg32(frameselectbot, *(u32*)(baseaddrbot + 0x14)); //todo
            //the rest is todo


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
