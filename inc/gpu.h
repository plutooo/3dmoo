/*
* Copyright (C) 2014 - plutoo
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

#ifndef _GPU_H_
#define _GPU_H_

#include "vec4.h"

/* Commands for gsp shared-mem. */
#define GSP_ID_REQUEST_DMA          0
#define GSP_ID_SET_CMDLIST          1
#define GSP_ID_SET_MEMFILL          2
#define GSP_ID_SET_DISPLAY_TRANSFER 3
#define GSP_ID_SET_TEXTURE_COPY     4
#define GSP_ID_FLUSH_CMDLIST        5

/* Interrupt ids. */
#define GSP_INT_PSC0 0 // Memory fill A
#define GSP_INT_PSC1 1 // Memory fill B
#define GSP_INT_VBLANK_TOP 2
#define GSP_INT_VBLANK_BOT 3
#define GSP_INT_PPF 4
#define GSP_INT_P3D 5 // Triggered when GPU-cmds have finished executing?
#define GSP_INT_DMA 6 // Triggered when DMA has finished.



#define LCDCOLORFILLMAIN 0x202204
#define LCDCOLORFILLSUB 0x202A04

#define framestridetop 0x400490
#define frameformattop 0x400470
#define frameselecttop 0x400478
#define framebuffer_top_size 0x40045C
#define RGBuponeleft 0x400468
#define RGBuptwoleft 0x40046C
#define RGBuponeright 0x400494
#define RGBuptworight 0x400498

#define framestridebot 0x400590
#define frameformatbot 0x400570
#define frameselectbot 0x400578
#define RGBdownoneleft 0x400568
#define RGBdowntwoleft 0x40056C


u8* LINEAR_MemoryBuff;
u8* VRAM_MemoryBuff;
u8* GSP_SharedBuff;
extern u32 GPU_Regs[0xFFFF];

#define STACK_MAX 64

#define Format_BYTE 0
#define Format_UBYTE 1
#define Format_SHORT 2
#define Format_FLOAT 3

#define VS_State_INVALID_ADDRESS 0xFFFFFFFF

#define GSP_Shared_Buff_Size 0x1000 //dumped from GSP module in Firm 4.4

#define TRIGGER_IRQ 0x10

#define CULL_MODE  0x40
#define VIEWPORT_WIDTH 0x41
#define VIEWPORT_WIDTH2 0x42
#define VIEWPORT_HEIGHT 0x43
#define VIEWPORT_HEIGHT2 0x44

#define GLViewport 0x68
#define Viewport_depth_range 0x4D
#define Viewport_depth_far_plane 0x4E

#define VS_VertexAttributeOutputMap 0x50
// untill 0x56

#define TEXTURING_SETINGS    0x80
#define TEXTURE_CONFIG0_SIZE 0x82
#define TEXTURE_CONFIG0_WRAP 0x83
#define TEXTURE_CONFIG0_ADDR 0x85
#define TEXTURE_CONFIG0_TYPE 0x8E

#define TEXTURE_CONFIG1_SIZE 0x92
#define TEXTURE_CONFIG1_WRAP 0x93
#define TEXTURE_CONFIG1_ADDR 0x95
#define TEXTURE_CONFIG1_TYPE 0x96

#define TEXTURE_CONFIG2_SIZE 0x9A
#define TEXTURE_CONFIG2_WRAP 0x9B
#define TEXTURE_CONFIG2_ADDR 0x9D
#define TEXTURE_CONFIG2_TYPE 0x9E

#define GLTEXENV 0xC0
// untill 0x100 with a jump at 0xE0- 0xF0
#define COLOR_OUTPUT_CONFIG 0x100
#define BLEND_CONFIG 0x101
#define COLOR_LOGICOP_CONFIG 0x102
#define BLEND_COLOR 0x103
#define ALPHATEST_CONFIG 0x104

#define DEPTHTEST_CONFIG 0x107
#define DEPTH_FORMAT 0x116
#define BUFFER_FORMAT 0x117

#define DEPTHBUFFER_ADDRESS 0x11C
#define COLORBUFFER_ADDRESS 0x11D

#define Framebuffer_FORMAT11E  0x11E

#define VertexAttributeConfig 0x200
// untill 0x226
#define IndexArrayConfig 0x227
#define NumVertices 0x228
#define TriggerDraw 0x22E
#define TriggerDrawIndexed 0x22F

#define TriangleTopology 0x25e

#define VS_resttriangel 0x25f

#define VS_INTUNIFORM_I0 0x2B1 //untill I3 in 0x284

#define VS_MainOffset 0x2BA
#define VS_InputRegisterMap 0x2BB
// untill 0x2BC
#define VS_FloatUniformSetup 0x2C0
// untill 0x2C8
#define VS_BeginLoadProgramData 0x2CB
#define VS_LoadProgramData 0x2CC
//untill 0x2D3
#define VS_BeginLoadSwizzleData 0x2D5
#define VS_LoadSwizzleData 0x2D6
// untill 0x2DD

#define SHDR_ADD     0x0
#define SHDR_DP3     0x1
#define SHDR_DP4     0x2
#define SHDR_DPH     0x3
#define SHDR_DST     0x4
#define SHDR_EXP     0x5
#define SHDR_LOG     0x6
#define SHDR_LITP    0x7
#define SHDR_MUL     0x8
#define SHDR_SGE     0x9
#define SHDR_SLT     0xA
#define SHDR_FLR     0xB
#define SHDR_MAX     0xC
#define SHDR_MIN     0xD
#define SHDR_RCP     0xE
#define SHDR_RSQ     0xF

#define SHDR_MOVA    0x12
#define SHDR_MOV     0x13

#define SHDR_NOP     0x21
#define SHDR_END     0x22
#define SHDR_BREAKC  0x23
#define SHDR_CALL    0x24
#define SHDR_CALLC   0x25
#define SHDR_CALLB   0x26
#define SHDR_IFB     0x27
#define SHDR_IFC     0x28
#define SHDR_LOOP    0x29
#define SHDR_JPC     0x2C
#define SHDR_JPB     0x2D
#define SHDR_CMP     0x2E
#define SHDR_CMP2    0x2F
#define SHDR_MAD1    0x38
#define SHDR_MAD2    0x39
#define SHDR_MAD3    0x3A
#define SHDR_MAD4    0x3B
#define SHDR_MAD5    0x3C
#define SHDR_MAD6    0x3D
#define SHDR_MAD7    0x3E
#define SHDR_MAD8    0x3F

struct OutputVertex {

    // VS output attributes
    struct vec4 position;
    struct vec4 dummy; // quaternions (not implemented, yet)
    struct vec4 color;
    struct vec2 texcoord0;
    struct vec2 texcoord1;
    float       texcoord0_w;
    struct vec3 View;
    struct vec2 texcoord2;

    // Padding for optimal alignment
    float pad[10];

    // Attributes used to store intermediate results

    // position after perspective divide
    struct vec3 screenpos;
};

struct clov4 {
    u8 v[5];
};

struct clov3 {
    u8 v[5];
};

void gpu_Init();
void gpu_WriteReg32(u32 addr, u32 data);
u32  gpu_ReadReg32(u32 addr);
void gpu_TriggerCmdReqQueue();
u32  gpu_RegisterInterruptRelayQueue(u32 Flags, u32 Kevent, u32*threadID, u32*outMemHandle);
u8*  gpu_GetPhysicalMemoryBuff(u32 addr);
u32  gpu_GetPhysicalMemoryRestSize(u32 addr);
void gpu_SendInterruptToAll(u32 ID);
void gpu_ExecuteCommands(u8* buffer, u32 size);
u32  gpu_GetSizeOfWidth(u16 val);
u32  gpu_ConvertVirtualToPhysical(u32 addr);
void gpu_UpdateFramebuffer();
void gpu_UpdateFramebufferAddr(u32 addr, bool bottom);
void gpu_WriteID(u16 ID, u8 mask, u32 size, u32* buffer);

//clipper.c
void Clipper_ProcessTriangle(struct OutputVertex *v0, struct OutputVertex *v1, struct OutputVertex *v2);

//rasterizer.c
void rasterizer_ProcessTriangle(struct OutputVertex *v0,
                                struct OutputVertex * v1,
                                struct OutputVertex * v2);

#endif
