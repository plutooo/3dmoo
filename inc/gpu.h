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
#define RGBuponeleft 0x400468
#define RGBuptwoleft 0x40046C
#define RGBuponeright 0x400494
#define RGBuptworight 0x400498

#define framestridebot 0x400590
#define frameformatbot 0x400570
#define frameselectbot 0x400578
#define RGBdownoneleft 0x400568
#define RGBdowntwoleft 0x40056C


u8* LINEmembuffer;
u8* VRAMbuff;
u8* GSPsharedbuff;
extern u32 GPU_Regs[0xFFFF];

#define GSPsharebuffsize 0x1000 //dumped from GSP module in Firm 4.4

#define TRIGGER_IRQ 0x10

#define VIEWPORT_WIDTH 0x41
#define VIEWPORT_HEIGHT 0x43

#define GLViewport 0x68
#define Viewport_depth_range 0x4D
#define Viewport_depth_far_plane 0x4E

#define VSVertexAttributeOutputMap 0x50
// untill 0x56

#define TEXTURINGSETINGS80 0x80
#define TEXTURCONFIG0SIZE 0x82
#define TEXTURCONFIG0ADDR 0x85
#define TEXTURCONFIG0TYPE 0x8E

#define TEXTURCONFIG1SIZE 0x92
#define TEXTURCONFIG1ADDR 0x95
#define TEXTURCONFIG1TYPE 0x96

#define TEXTURCONFIG2SIZE 0x9A
#define TEXTURCONFIG2ADDR 0x9D
#define TEXTURCONFIG2TYPE 0x9E

#define GLTEXENV 0xC0
// untill 0x100 with a jump at 0xE0- 0xF0

#define DEPTHTEST_CONFIG 0x107
#define BUFFERFORMAT 0x117

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

#define VSresttriangel 0x25f

#define VS_INTUNIFORM_I0 0x2B1 //untill I3 in 0x284

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


struct OutputVertex {

    // VS output attributes
    struct vec4 pos;
    struct vec4 dummy; // quaternions (not implemented, yet)
    struct vec4 color;
    struct vec2 tc0;
    struct vec2 tc1;
    float tc0_w;
    struct vec3 View;
    struct vec2 tc2;

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
u32 gpu_ReadReg32(u32 addr);
void GPUTriggerCmdReqQueue();
u32 GPURegisterInterruptRelayQueue(u32 Flags, u32 Kevent, u32*threadID, u32*outMemHandle);
u8* get_pymembuffer(u32 addr);
u32 get_py_memrestsize(u32 addr);
void gpu_SendInterruptToAll(u32 ID);
void runGPU_Commands(u8* buffer, u32 size);
u32 getsizeofwight(u16 val);
u32 convertvirtualtopys(u32 addr);
void updateFramebuffer();
void updateFramebufferaddr(u32 addr, bool bot);

//clipper.c
void Clipper_ProcessTriangle(struct OutputVertex *v0, struct OutputVertex *v1, struct OutputVertex *v2);

//rasterizer.c
void rasterizer_ProcessTriangle(const struct OutputVertex *v0,
                                const struct OutputVertex * v1,
                                const struct OutputVertex * v2);

#endif
