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
#include "vec4.h"

#define logGSPparser

float f24to32(u32 data, u32 *dataa);

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
    GPUwritereg32(frameselectbot, 0);
    GPUwritereg32(RGBdownoneleft, 0x18000000 + 0x46500 * 4);
    GPUwritereg32(RGBdowntwoleft, 0x18000000 + 0x46500 * 5);
}

u32 convertvirtualtopys(u32 addr) //todo
{
    if (addr >= 0x14000000 && addr < 0x1C000000)return addr + 0xC000000; //FCRAM
    if (addr >= 0x1F000000 && addr < 0x1F600000)return addr - 0x7000000; //VRAM
    DEBUG("can't convert vitual to py %08x\n",addr);
    return 0;
}
void GPUwritereg32(u32 addr, u32 data) //1eb00000 + addr
{
    DEBUG("GPU write %08x to %08x\n",data,addr);
    if (addr >= 0x420000) {
        DEBUG("write out of range write\r\n");
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
    //DEBUG("GPU read %08x\n", addr);
    if (addr >= 0x420000) {
        DEBUG("read out of range write\r\n");
        return 0;
    }
    return *(uint32_t*)(&IObuffer[addr]);
}
u32 getsizeofwight(u16 val) //this is the size of pixel
{
    switch (val) {
    case 0x0201:
        return 3;
    default:
        DEBUG("unknown len %04x\n",val);
        return 3;
    }
}
u32 getsizeofwight32(u32 val)
{
    switch (val) {
    case 0x00800080: //no idea why
        return 0x8000;
    default:
        return (val & 0xFFFF) * ((val>>16) & 0xFFFF) * 3;
    }
}

u32 getsizeofframebuffer(u32 val)
{
    switch (val) {
    case 0x0201:
        return 3;
    default:
        DEBUG("unknown len %08x\n", val);
        return 3;
    }
}

u32 renderaddr = 0;
u32 unknownaddr = 0;





updateGPUintreg(u32 data,u32 ID,u8 mask)
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


#define VSVertexAttributeOutputMap 0x50 
// untill 0x56

#define VertexAttributeConfig 0x200
// untill 0x226 
#define IndexArrayConfig 0x227
#define NumVertices 0x228
#define TriggerDraw 0x22E
#define TriggerDrawIndexed 0x22F

#define TriangleTopology 0x25e

#define VSMainOffset 0x2BA
#define VSInputRegisterMap 0x2BB
// untill 0x2BC
#define VSFloatUniformSetup 0x2C0
// untill 0x2C8
#define VSBeginLoadProgramData 0x2CB
#define VSLoadProgramData 0x2CC
//untill 0x2D3
#define VSBeginLoadSwizzleData 0x2D5
#define VSLoadSwizzleData 0x2D6
// untill 0x2DD



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


struct OutputVertex {
    
        // VS output attributes
       struct vec4 pos;
       struct vec4 dummy; // quaternions (not implemented, yet)
       struct vec4 color;
       struct vec2 tc0;
        float tc0_v;
    
        // Padding for optimal alignment
       float pad[14];
    
       // Attributes used to store intermediate results
       
        // position after perspective divide
       struct vec3 screenpos;
};








static struct OutputVertex buffer[2];
static int buffer_index = 0; // TODO: reset this on emulation restart

void PrimitiveAssembly_SubmitVertex(struct OutputVertex vtx)
{
    u32 topology = (GPUregs[TriangleTopology] >> 8) & 0x3;
    switch (topology) {
    case 0://List:
    case 3://ListIndexed:
        if (buffer_index < 2) {
            buffer[buffer_index++] = vtx;
        }
        else {
            buffer_index = 0;

            //Clipper_ProcessTriangle(buffer[0], buffer[1], vtx);
        }
        break;

    case 2://Fan:
        if (buffer_index == 2) {
            buffer_index = 0;

           //Clipper_ProcessTriangle(buffer[0], buffer[1], vtx);

            buffer[1] = vtx;
        }
        else {
            buffer[buffer_index++] = vtx;
        }
        break;

    default:
        DEBUG("Unknown triangle mode %x:", (int)topology);
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
    return (hex >> 0x13) & 0x7F;
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
instr_flow_control_offset_words(u32 hex)
{
    return (hex>>0xa)&0xFFF;
}
bool swizzle_DestComponentEnabled(int i, u32 swizzle)
{
    return (swizzle & (0x8 >> i));
}

struct vec4 shader_uniforms[96];



void ProcessShaderCode(struct VertexShaderState state) {
    while (true) {
        bool increment_pc = true;
        bool exit_loop = false;
        u32 instr = *(u32*)state.program_counter;

        const float* src1_ = (instr_common_src1(instr) < 0x10) ? state.input_register_table[instr_common_src1(instr)]
            : (instr_common_src1(instr) < 0x20) ? &state.temporary_registers[instr_common_src1(instr) - 0x10].v[0]
            : (instr_common_src1(instr) < 0x80) ? &shader_uniforms[instr_common_src1(instr) - 0x20].v[0]
            : (float*)0;
        const float* src2_ = (instr_common_src2(instr) < 0x10) ? state.input_register_table[instr_common_src2(instr)]
            : &state.temporary_registers[instr_common_src2(instr) - 0x10].v[0];
        // TODO: Unsure about the limit values
        float* dest = (instr_common_dest(instr) <= 0x1C) ? state.output_register_table[instr_common_dest(instr)]
            : (instr_common_dest(instr) <= 0x3C) ? (float*)0
            : (instr_common_dest(instr) <= 0x7C) ? &state.temporary_registers[(instr_common_dest(instr) - 0x40) / 4].v[instr_common_dest(instr) % 4] //vec4_getp(instr_common_dest(instr) % 4, state.temporary_registers[(instr_common_dest(instr) - 0x40) / 4])
            : (float*)0;

        u32 swizzle = swizzle_data[instr_common_operand_desc_id(instr)];

        const float src1[4] = {
            src1_[(int)((swizzle>>11)&0x3)],
            src1_[(int)((swizzle >> 9) & 0x3)],
            src1_[(int)((swizzle >> 7) & 0x3)],
            src1_[(int)((swizzle >> 5) & 0x3)],
        };
        const float src2[4] = {
            src2_[(int)((swizzle >> 14) & 0x3)],
            src2_[(int)((swizzle >> 16) & 0x3)],
            src2_[(int)((swizzle >> 18) & 0x3)],
            src2_[(int)((swizzle >> 20) & 0x3)],
        };

        switch (instr_opcode(instr)) {
        case SHDR_ADD:
        {
                                         for (int i = 0; i < 4; ++i) {
                                             if (!swizzle_DestComponentEnabled(i, swizzle))
                                                 continue;

                                             dest[i] = src1[i] + src2[i];
                                         }

                                         break;
        }

        case SHDR_MUL:
        {
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
                                         for (int i = 0; i < 4; ++i) {
                                             if (!swizzle_DestComponentEnabled(i, swizzle))
                                                 continue;

                                             dest[i] = src1[i];
                                         }
                                         break;
        }

        case SHDR_RET:
            if (*state.call_stack_pointer == VertexShaderState_INVALID_ADDRESS) {
                exit_loop = true;
            }
            else {
                state.program_counter = &GPUshadercodebuffer[*state.call_stack_pointer--];
                *state.call_stack_pointer = VertexShaderState_INVALID_ADDRESS;
            }

            break;

        case SHDR_CALL:
            increment_pc = false;

            //_dbg_assert_(GPU, state.call_stack_pointer - state.call_stack < sizeof(state.call_stack)); //todo

            *++state.call_stack_pointer = state.program_counter - (u32)(&GPUshadercodebuffer[0]);
            // TODO: Does this offset refer to the beginning of shader memory?
            state.program_counter = &GPUshadercodebuffer[instr_flow_control_offset_words(instr)];
            break;

        case SHDR_FLS:
            // TODO: Do whatever needs to be done here?
            break;

        default:
            DEBUG("Unhandled instruction: 0x%08x",instr);
            break;
        }

        if (increment_pc)
            state.program_counter++;

        if (exit_loop)
            break;
        if (0xFFFF>state.program_counter)
            break;
    }
}






RunShader(struct vec4 input[17], int num_attributes, struct OutputVertex ret)
{
    struct VertexShaderState state;

    //const u32* main = &shader_memory[registers.Get<Regs::VSMainOffset>().offset_words];
    state.program_counter = (u32*)((u32)(&GPUshadercodebuffer[0]) + (u16)GPUregs[VSMainOffset]);;

    // Setup input register table

    if (num_attributes > 0) state.input_register_table[getattribute_register_map(0, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[0].v[0];
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
    if (num_attributes > 15) state.input_register_table[getattribute_register_map(15, GPUregs[VSInputRegisterMap], GPUregs[VSInputRegisterMap + 1])] = &input[15].v[0];

    // Setup output register table
    //struct OutputVertex ret;
    for (int i = 0; i < 7; ++i) {
        u32 output_register_map = GPUregs[VSVertexAttributeOutputMap + i];

        u32 semantics[4] = {
            (output_register_map >> 0) & 0x1F, (output_register_map >> 8) & 0x1F,
            (output_register_map >> 16) & 0x1F, (output_register_map >> 24) & 0x1F
        };

        for (int comp = 0; comp < 4; ++comp)
            state.output_register_table[4 * i + comp] = ((float*)&ret) + semantics[comp];
    }


    state.status_registers[0] = false;
    state.status_registers[1] = false;
    int k;
    for (k = 0; k < 7; k++)state.call_stack[k] = VertexShaderState_INVALID_ADDRESS;
    state.call_stack_pointer = &state.call_stack[0];

    ProcessShaderCode(state);

    DEBUG("Output vertex: pos (%.2f, %.2f, %.2f, %.2f), col(%.2f, %.2f, %.2f, %.2f), tc0(%.2f, %.2f)\n",
        ret.pos.v[0], ret.pos.v[1], ret.pos.v[2], ret.pos.v[3],
        ret.color.v[0], ret.color.v[1], ret.color.v[2], ret.color.v[3],
        ret.tc0.v[0], ret.tc0.v[1]);

    //return ret;
}












u32 GetComponent(u32 n,u32* data)
{
    if (n < 8)
    {
        return ((*data) >> n * 4) & 0xF;
    }
    else
    {
        return ((*(data + 1)) >> (n - 8) * 4) & 0xF;
    }
}

u32 GetNumElements(u32 n, u32* data)
{
    if (n < 8)
    {
        return ((*(data + 1)) >> ((n * 4)) +2 ) & 0x3;
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
    int i;
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
                               const u8* index_address_8 = (u8*)get_pymembuffer(base_address) + index_info_offset;
                               const u16* index_address_16 = (u16*)index_address_8;
                               bool index_u16 = (GPUregs[IndexArrayConfig] >> 31);
                               for (int index = 0; index < GPUregs[NumVertices]; index++)
                               {
                                   int vertex = is_indexed ? (index_u16 ? index_address_16[index] : index_address_8[index]) : index;

                                   if (is_indexed) {
                                       // TODO: Implement some sort of vertex cache!
                                   }

                                   // Initialize data for the current vertex
                                   struct vec4 input[17];
                                   u8 NumTotalAttributes = (attribute_config[1] >> 28) + 1;
                                   for (int i = 0; i < NumTotalAttributes; i++) {
                                       for (int comp = 0; comp < vertex_attribute_elements[i]; comp++) {
                                           const u8* srcdata = vertex_attribute_sources[i] + vertex_attribute_strides[i] * vertex + comp * vertex_attribute_element_size[i];
                                           const float srcval = (vertex_attribute_formats[i] == 0) ? *(s8*)srcdata :
                                               (vertex_attribute_formats[i] == 1) ? *(u8*)srcdata :
                                               (vertex_attribute_formats[i] == 2) ? *(s16*)srcdata :
                                               *(float*)srcdata;
                                           input[i].v[comp] = srcval;
                                           DEBUG("Loaded component %x of attribute %x for vertex %x (index %x) from 0x%08x + 0x%08x + 0x%04x: %f\n",
                                               comp, i, vertex, index,
                                               base_address,
                                               vertex_attribute_sources[i] - base_address,
                                               srcdata - vertex_attribute_sources[i],
                                               input[i].v[comp]);
                                       }
                                   }
                                   struct OutputVertex output;
                                   RunShader(input, NumTotalAttributes, output);
                                   //VertexShader::OutputVertex output = VertexShader::RunShader(input, attribute_config.GetNumTotalAttributes());

                                   if (is_indexed) {
                                       // TODO: Add processed vertex to vertex cache!

                                   }

                                   PrimitiveAssembly_SubmitVertex(output);

                               }

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
            DEBUG("abnormal VSLoadProgramData %0x1 %0x3\n", mask, size);
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
            DEBUG("abnormal VSLoadSwizzleData %0x1 %0x3\n", mask, size);
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
                u8 index = GPUregs[VSFloatUniformSetup] & 0xFF;
                if (index > 95) {
                    DEBUG("Invalid VS uniform index %02x\n", index);
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

                DEBUG("Set uniform %02x to (%f %f %f %f)\n", index,
                    vectors[index].v[0], vectors[index].v[1], vectors[index].v[2],
                    vectors[index].v[3]);
                // TODO: Verify that this actually modifies the register!
                GPUregs[VSFloatUniformSetup]++;
            }
        }
        break;
    default:
        updateGPUintreg(*buffer, ID, mask);
    }
}


void runGPU_Commands(u8* buffer, u32 size)
{
    u32 i;
    for (i = 0; i < size; i += 8)
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
        DEBUG("cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "")
#endif
        int j;
        for (j = 0; j < size; j++)
        {
            datafild[1 + j] = *(u32*)(buffer + 8 + i);
#ifdef logGSPparser
            DEBUG("data %08x\n", datafild[1 + j]);
#endif
            i += 4;
        }
        if (size & 0x1)
        {
            u32 data = *(u32*)(buffer + 8 + i);
#ifdef logGSPparser
            DEBUG("padding data %08x\n", data);
#endif
            i += 4;
        }
        if (mask != 0)
        {
            if (size > 0 && mask != 0xF)
                DEBUG("masked data? cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
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
            DEBUG("masked out data is ignored cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "");
        }

    }
}







#ifdef oldga2
char* GLenumname[] = { "GL_BYTE", "GL_UNSIGNED_BYTE", "GL_UNSIGNED_SHORT", "GL_FLOAT" };

u32 GPUshaderbuffer[0xFFFF];


void runshadr(u32 entry)
{
    bool exit = false;
    do
    {
        entry++;
        u32 cmd = GPUshaderbuffer[entry];
        switch (cmd >> 26)
        {
        case 0x0:
            DEBUG("add\n");
            break;
        case 0x1:
            DEBUG("dp3\n");
            break;
        case 0x2:
            DEBUG("dp4\n");
            break;
        case 0x8:
            DEBUG("mul\n");
            break;
        case 0x9:
            DEBUG("max\n");
            break;
        case 0xA:
            DEBUG("min\n");
            break;
        case 0x13:
            DEBUG("mov d%02x, d%02x (%02x)\n", (cmd >> 21) & 0x1F, (cmd >> 12) & 0x7F, cmd & 0x7F);
            break;
        case 0x21:
            DEBUG("end\n");
            exit = true;
            break;
        case 0x22:
            DEBUG("flush\n");
            break;
        case 0x28:
            DEBUG("if\n");
            break;
        default:
            DEBUG("unknown gpu cmd %08x\n",cmd);
            break;
        }
    } while (!exit && entry < 0xFF);
}

typedef struct {
    float w;
    float z;
    float y;
    float x;
} uniformtype;

uniformtype uniforms[96];

bool IsFloat2c0 = 0;
u8 index2c0 = 0;

void runGPU_Commands(u8* buffer, u32 size)
{
    u32 i;
    s32 shdaddr = -1;
    u32 shdrentr = 0;
    for (i = 0; i < size; i += 8)
    {
        u32 cmd = *(u32*)(buffer + 4 + i);
        u32 dataone = *(u32*)(buffer + i);
        u16 ID = cmd & 0xFFFF;
        u16 mask = (cmd >> 16) & 0xF;
        u16 size = (cmd >> 20) & 0x7FF;
        u8 grouping = (cmd >> 31);
        switch (ID)
        {

        case 0x22e:
        case 0x22f:
        {
               runshadr(shdrentr);
               DEBUG("starter\n");
               break;
        }

        case 0x2c1:
        {
                      int k;
                      u32 temp = *(u32*)(buffer + 0x4);
                      *(u32*)(buffer + 0x4) = dataone;
                      for (k = 0; k < ((size + 1) / (IsFloat2c0 ? 4 : 3)); k++)
                      {
                          if (IsFloat2c0)
                          {
                              uniforms[index2c0].w = *(float*)(buffer + 0x4 + i + k * 4 * 4);
                              uniforms[index2c0].z = *(float*)(buffer + 0x8 + i + k * 4 * 4);
                              uniforms[index2c0].y = *(float*)(buffer + 0xC + i + k * 4 * 4);
                              uniforms[index2c0].x = *(float*)(buffer + 0x10 + i + k * 4 * 4);
                          }
                          else
                          {
                              uniforms[index2c0].w = *(u32*)(buffer + 0x4 + i + k * 4 * 3) >> 8;
                              uniforms[index2c0].z = ((*(u32*)(buffer + 0x8 + i + k * 4 * 3) >> 16) & 0xFFFF) | (*(u32*)(buffer + 0x4 + i + k * 4 * 3) & 0xFF) << 16;
                              uniforms[index2c0].y = (((*(u32*)(buffer + 0x8 + i + k * 4 * 3) >> 16) & 0xFFFF) << 8) | *(u32*)(buffer + 0xC + i + k * 4 * 3) >> 24;
                              uniforms[index2c0].x = *(u32*)(buffer + 0xC + i + k * 4 * 3) & 0xFFFFFF;

                              f24to32(uniforms[index2c0].w, &uniforms[index2c0].w);
                              f24to32(uniforms[index2c0].z, &uniforms[index2c0].z);
                              f24to32(uniforms[index2c0].y, &uniforms[index2c0].y);
                              f24to32(uniforms[index2c0].x, &uniforms[index2c0].x);
                          }
                          DEBUG("VSFloatUniform %02x (%f %f %f %f) %s\n", index2c0, uniforms[index2c0].w, uniforms[index2c0].z, uniforms[index2c0].y, uniforms[index2c0].x, IsFloat2c0 ? "float32" : "float24");
                          index2c0++;
                      }
                      *(u32*)(buffer + 0x4) = temp;
                      break;
        }
        case 0x2c0:
        {
                      if (size != 0)
                      {
                          bool IsFloat = (dataone >> 31);
                          u8 index = (dataone & 0xFF);
                          if (IsFloat)
                          {
                              uniforms[index].w = *(float*)(buffer + 0x8 + i);
                              uniforms[index].z = *(float*)(buffer + 0xC + i);
                              uniforms[index].y = *(float*)(buffer + 0x10 + i);
                              uniforms[index].x = *(float*)(buffer + 0x14 + i);
                          }
                          else
                          {
                              uniforms[index].w = *(u32*)(buffer + 0x8 + i) >> 8;
                              uniforms[index].z = ((*(u32*)(buffer + 0xC + i) >> 16) & 0xFFFF) | (*(u32*)(buffer + 0x8 + i) & 0xFF) << 16;
                              uniforms[index].y = (((*(u32*)(buffer + 0xC + i) >> 16) & 0xFFFF) << 8) | *(u32*)(buffer + 0x10 + i) >> 24;
                              uniforms[index].x = *(u32*)(buffer + 0x10 + i) & 0xFFFFFF;

                              f24to32(uniforms[index].w, &uniforms[index].w);
                              f24to32(uniforms[index].z, &uniforms[index].z);
                              f24to32(uniforms[index].y, &uniforms[index].y);
                              f24to32(uniforms[index].x, &uniforms[index].x);
                          }
                          DEBUG("VSFloatUniform %02x (%f %f %f %f) %s\n", index, uniforms[index].w, uniforms[index].z, uniforms[index].y, uniforms[index].x, IsFloat ? "float32" : "float24");
                      }
                      else
                      {
                          IsFloat2c0 = (dataone >> 31);
                          index2c0 = (dataone & 0xFF);
                          DEBUG("VSFloatUniform %02x (?) %s\n", index2c0, IsFloat2c0 ? "float32" : "float24");
                      }

                      break;
        }
        case 0x2BA:
            if (cmd == 0x800f02ba && ((dataone & 0xFFFF0000) == 0x7FFF0000))
            {
                shdrentr = dataone & 0xFFFF;
                DEBUG("shader program entry %04x\n", shdrentr);
                continue;
            }
        case 0x2cb:
            if (cmd == 0x000f02cb && dataone == 0x00000000)
            {
                DEBUG("shader program data start\n", size);
                shdaddr = 0;
                continue;
            }
        case 0x2cc:
            if ((cmd & 0x800FFFFF) == 0x000f02cc)
            {
                DEBUG("shader program data %04x\n", size);
                if (shdaddr < 0) DEBUG("error uninted shader program data");
                GPUshaderbuffer[shdaddr++] = dataone;
                u32 j;
                for (j = 0; j < size; j++)
                {
                    GPUshaderbuffer[shdaddr++] = *(u32*)(buffer + 0x8 + i);
                    i += 4;
                }
                if (size & 0x1)
                {
                    u32 data = *(u32*)(buffer + 8 + i);
                    DEBUG("padding data %08x\n", data);
                    i += 4;
                }

                //runshadr(0);
                continue;
            }
        case 0x2bf:
            if (cmd == 0x000f02bf && dataone == 0x00000001)
            {
                DEBUG("shader program end\n", size);
                shdaddr = -1;
                continue;
            }
        case 0x200:
            if (cmd == 0x826f0200)
            {
                DEBUG("SetAttributeBuffers %08x\n", dataone << 3);
                u32 data1 = *(u32*)(buffer + 8 + i);
                u32 data2 = *(u32*)(buffer + 0xC + i);

                u32 count = ((data2 >> 28) & 0xF) + 1;
                u32 mask = ((data2 >> 16) & 0xFFF);
                printf("count %02x mask %03x\n", count, mask);

                int k;
                for (k = 0; k < count; k++)
                {
                    u8 format;
                    if (k < 8)
                    {
                        format = (data1 >> (k * 4)) & 0xF;
                    }
                    else
                    {
                        format = (data2 >> ((k - 8) * 4)) & 0xF;
                    }
                    printf("Format: %s %01x (%01x)\n", GLenumname[format & 0x3], ((format >> 2) & 0x3) + 1, format);
                    u32 enddata1 = *(u32*)(buffer + 0x10 + i + k * 0xC);
                    u32 enddata2 = *(u32*)(buffer + 0x14 + i + k * 0xC);
                    u32 enddata3 = *(u32*)(buffer + 0x18 + i + k * 0xC);
                    printf("addr: %08x\n", (dataone << 3) + enddata1);
                    printf("Permutations: %04x%08x\n", enddata3 & 0xFFFF, enddata2);

                    printf("stride: %03x\n", (enddata3 >> 16) & 0xFFF);
                    printf("NumAttributes: %01x\n", (enddata3 >> 28) & 0xF);
                }


                i += 4 * 0x26;

                if (size & 0x1)
                {
                    u32 data = *(u32*)(buffer + 8 + i);
                    DEBUG("padding data %08x\n", data);
                    i += 4;
                }

                continue;
            }
        case 0x10:
            if (cmd == 0x000F0010 && dataone == 0x12345678)
            {
                DEBUG("END\n");
                continue;
            }
        case 0x111:
            if (cmd == 0x000F0111)
            {
                DEBUG("cmd 0x000F0111 %x\n", dataone);
                continue;
            }
        case 0x110:
            if (cmd == 0x000F0110)
            {
                DEBUG("cmd 0x000F0110 %x\n", dataone);
                continue;
            }
        case 0x117:
            if (cmd == 0x000F0117)
            {
                DEBUG("cmd 0x000F0117 %04x %04x\n", dataone & 0xFFFF, (dataone >> 16) & 0xFFFF);
                continue;
            }
        case 0x11D:
            if (cmd == 0x000F011D)
            {
                renderaddr = dataone << 3;
                DEBUG("setrederaddr %08x\n", renderaddr);
                continue;
            }
        case 0x116:
            if (cmd == 0x000F0116)
            {
                DEBUG("cmd 0x000F0116 %08x\n", dataone);
                continue;
            }
        case 0x11C:
            if (cmd == 0x000F011C)
            {
                unknownaddr = dataone << 3;
                DEBUG("set unknownaddr %08x\n", unknownaddr);
                continue;
            }
        case 0x011E:
            if (cmd == 0x000F011E)
            {
                DEBUG("configframebuffer --todo-- width=%04x height= %04x\n", dataone & 0xFFF, ((dataone >> 12) & 0xFFF) + 1);
                continue;
            }
        case 0x006E:
            if (cmd == 0x000F006E)
            {
                DEBUG("configframebuffer2 --todo-- width=%04x height= %04x\n", dataone & 0xFFF, ((dataone >> 12) & 0xFFF) + 1);
                continue;
            }
        case 0x41:
            if (cmd == 0x000F0041)
            {
                float temp23;
                f24to32(dataone, &temp23);
                DEBUG("VIEWPORT_WIDTH %f\n", temp23);
                continue;
            }
        case 0x42:
            if (cmd == 0x000F0042)
            {
                DEBUG("VIEWPORT_WIDTH_INV %08x\n", dataone);
                continue;
            }
        case 0x43:
            if (cmd == 0x000F0043)
            {
                float temp23;
                f24to32(dataone, &temp23);
                DEBUG("VIEWPORT_HEIGHT %f\n", temp23);
                continue;
            }
        case 0x44:
            if (cmd == 0x000F0044)
            {
                DEBUG("VIEWPORT_HEIGHT_INV %08x\n", dataone);
                continue;
            }

        case 0x68:
            if (cmd == 0x000F0068)
            {
                DEBUG("glViewport %08x\n", dataone);
                continue;
            }

        default:
            break;
        }
        DEBUG("cmd %04x mask %01x size %03x (%08x) %s \n", ID, mask, size, dataone, grouping ? "grouping" : "")
            int j;
        for (j = 0; j < size; j++)
        {
            u32 data = *(u32*)(buffer + 8 + i);
            DEBUG("data %08x\n", data);
            i += 4;
        }
        if (size & 0x1)
        {
            u32 data = *(u32*)(buffer + 8 + i);
            DEBUG("padding data %08x\n", data);
            i += 4;
        }
    }


}
#endif


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