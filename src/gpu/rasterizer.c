/*
* Copyright (C) 2014 - Tony Wasserka.
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
//this is based on citra, copyright (C) 2014 Tony Wasserka.

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "gpu.h"
#include "color.h"

//#define testtriang

#ifdef testtriang
u32 numb = 0x111;
#endif

struct vec3_12P4 {
    s16 v[3];//x, y,z;
};

static u16 min3(s16 v1,s16 v2,s16 v3)
{
    if (v1 < v2 && v1 < v3) return v1;
    if (v2 < v3) return v2;
    return v3;
}
static u16 max3(s16 v1, s16 v2, s16 v3)
{
    if (v1 > v2 && v1 > v3) return v1;
    if (v2 > v3) return v2;
    return v3;
}

#define IntMask 0xFFF0
#define FracMask 0xF

static bool IsRightSideOrFlatBottomEdge(struct vec3_12P4 * vtx, struct vec3_12P4 *line1, struct vec3_12P4 *line2)
{
    if (line1->v[1] == line2->v[1]) {
        // just check if vertex is above us => bottom line parallel to x-axis
        return vtx->v[1] < line1->v[1];
    } else {
        // check if vertex is on our left => right side
        // TODO: Not sure how likely this is to overflow
        return (int)vtx->v[0] < (int)line1->v[0] + ((int)line2->v[0] - (int)line1->v[0]) * ((int)vtx->v[1] - (int)line1->v[1]) / ((int)line2->v[1] - (int)line1->v[1]);
    }
}

static s32 orient2d(u16 vtx1x, u16  vtx1y, u16  vtx2x, u16  vtx2y, u16  vtx3x, u16  vtx3y)
{
    s32 vec1x = vtx2x - vtx1x;
    s32 vec1y = vtx2y - vtx1y;
    s32 vec2x = vtx3x - vtx1x;
    s32 vec2y = vtx3y - vtx1y;
    // TODO: There is a very small chance this will overflow for sizeof(int) == 4
    return vec1x*vec2y - vec1y*vec2x;
}

static u16 GetDepth(int x, int y) 
{
    u16* depth_buffer = (u16*)get_pymembuffer(GPU_Regs[DEPTHBUFFER_ADDRESS] << 3);

    return *(depth_buffer + x + y * (GPU_Regs[Framebuffer_FORMAT11E] & 0xFFF) / 2);
}

static void SetDepth(int x, int y, u16 value)
{
    u16* depth_buffer = (u16*)get_pymembuffer(GPU_Regs[DEPTHBUFFER_ADDRESS] << 3);

    // Assuming 16-bit depth buffer format until actual format handling is implemented
    if (depth_buffer) //there is no depth_buffer
        *(depth_buffer + x + y * (GPU_Regs[Framebuffer_FORMAT11E] & 0xFFF) / 2) = value;
}

#ifdef testtriang
static void DrawPixel(int x, int y, struct clov4* color)
{
#else
static void DrawPixel(int x, int y, const struct clov4* color)
{
#endif
    u8* color_buffer = (u8*)get_pymembuffer(GPU_Regs[COLORBUFFER_ADDRESS] << 3);

#ifdef testtriang
    color->v[0] = (numb&0xF) << 0x4;
    color->v[1] = (numb & 0xF0);
    color->v[2] = (numb & 0xF00) >> 0x4;
#endif

    u32 inputdim = GPU_Regs[Framebuffer_FORMAT11E];
    u32 outy = (inputdim & 0xFFF);
    u32 outx = ((inputdim >> 0x10) & 0xFFF);

    //TODO: workout why this seems required for ctrulib gpu demo (outy=480)
    if(outy > 240) outy = 240;

    //DEBUG("x=%d,y=%d,outx=%d,outy=%d,format=%d,inputdim=%08X,bufferformat=%08X\n", x, y, outx, outy, (GPU_Regs[BUFFERFORMAT] & 0x7000) >> 12, inputdim, GPU_Regs[BUFFERFORMAT]);

    Color ncolor;
    ncolor.r = color->v[0];
    ncolor.g = color->v[1];
    ncolor.b = color->v[2];
    ncolor.a = color->v[3];

    u8* outaddr;
    // Assuming RGB8 format until actual framebuffer format handling is implemented
    switch (GPU_Regs[BUFFERFORMAT] & 0x7000) { //input format

    case 0: //RGBA8
        outaddr = color_buffer + x * 4 + y * (outy)* 4; //check if that is correct
        color_encode(&ncolor, RGBA8, outaddr);
        break;
    case 0x1000: //RGB8
        outaddr = color_buffer + x * 3 + y * (outy)* 3; //check if that is correct
        color_encode(&ncolor, RGB8, outaddr);
        break;
    case 0x2000: //RGB565
        outaddr = color_buffer + x * 2 + y * (outy)* 2; //check if that is correct
        color_encode(&ncolor, RGB565, outaddr);
        break;
    case 0x3000: //RGB5A1
        outaddr = color_buffer + x * 2 + y * (outy)* 2; //check if that is correct
        color_encode(&ncolor, RGBA5551, outaddr);
        break;
    case 0x4000: //RGBA4
        outaddr = color_buffer + x * 2 + y * (outy)* 2; //check if that is correct
        color_encode(&ncolor, RGBA4, outaddr);
        break;
    default:
        DEBUG("error unknown output format %04X\n", GPU_Regs[BUFFERFORMAT] & 0x7000);
        break;
    }

}

#define GetPixel RetrievePixel
static void RetrievePixel(int x, int y, struct clov4 *output)
{
    u8* color_buffer = (u8*)get_pymembuffer(GPU_Regs[COLORBUFFER_ADDRESS] << 3);

    u32 inputdim = GPU_Regs[Framebuffer_FORMAT11E];
    u32 outy = (inputdim & 0xFFF);
    u32 outx = ((inputdim >> 0x10) & 0xFFF);

    //TODO: workout why this seems required for ctrulib gpu demo (outy=480)
    if(outy > 240) outy = 240;

    //DEBUG("x=%d,y=%d,outx=%d,outy=%d,format=%d,inputdim=%08X,bufferformat=%08X\n", x, y, outx, outy, (GPU_Regs[BUFFERFORMAT] & 0x7000) >> 12, inputdim, GPU_Regs[BUFFERFORMAT]);

    Color ncolor;
    memset(&ncolor, 0, sizeof(Color));

    u8* addr;
    // Assuming RGB8 format until actual framebuffer format handling is implemented
    switch(GPU_Regs[BUFFERFORMAT] & 0x7000) { //input format

        case 0: //RGBA8
            addr = color_buffer + x * 4 + y * (outy)* 4; //check if that is correct
            color_decode(addr, RGBA8, &ncolor);
            break;
        case 0x1000: //RGB8
            addr = color_buffer + x * 3 + y * (outy)* 3; //check if that is correct
            color_decode(addr, RGB8, &ncolor);
            break;
        case 0x2000: //RGB565
            addr = color_buffer + x * 2 + y * (outy)* 2; //check if that is correct
            color_decode(addr, RGB565, &ncolor);
            break;
        case 0x3000: //RGB5A1
            addr = color_buffer + x * 2 + y * (outy)* 2; //check if that is correct
            color_decode(addr, RGBA5551, &ncolor);
            break;
        case 0x4000: //RGBA4
            addr = color_buffer + x * 2 + y * (outy)* 2; //check if that is correct
            color_decode(addr, RGBA4, &ncolor);
            break;
        default:
            DEBUG("error unknown output format %04X\n", GPU_Regs[BUFFERFORMAT] & 0x7000);
            break;
    }

    output->v[0] = ncolor.r;
    output->v[1] = ncolor.g;
    output->v[2] = ncolor.b;
    output->v[3] = ncolor.a;
}

static float GetInterpolatedAttribute(float attr0, float attr1, float attr2, const struct OutputVertex *v0, const struct OutputVertex * v1, const struct OutputVertex * v2,float w0,float w1, float w2)
{
    float interpolated_attr_over_w = (attr0 / v0->pos.v[3])*w0 + (attr1 / v1->pos.v[3])*w1 + (attr2 / v2->pos.v[3])*w2;
    float interpolated_w_inverse = ((1.f) / v0->pos.v[3])*w0 + ((1.f) / v1->pos.v[3])*w1 + ((1.f) / v2->pos.v[3])*w2;
    return interpolated_attr_over_w / interpolated_w_inverse;
}
static void GetColorModifier(u32 factor, struct clov4/*3*/ * values)
{
    switch (factor) {
    case 0://SourceColor:
        return;
    case 1://OneMinusSourceColor
        values->v[0] = 255 - values->v[0];
        values->v[1] = 255 - values->v[1];
        values->v[2] = 255 - values->v[2];
        return;
    case 2://SourceAlpha:
        values->v[0] = values->v[3];
        values->v[1] = values->v[3];
        values->v[2] = values->v[3];
        return;
    case 3://OneMinusSourceAlpha:
        values->v[0] = 255 - values->v[3];
        values->v[1] = 255 - values->v[3];
        values->v[2] = 255 - values->v[3];
        return;
    case 4://SourceRed:
        //values->v[0] = values->v[0];
        values->v[1] = values->v[0];
        values->v[2] = values->v[0];
        return;
    case 5://OneMinusSourceRed:
        values->v[1] = 255 - values->v[0];
        values->v[2] = 255 - values->v[0];
        //Important!
        values->v[0] = 255 - values->v[0];
        return;
    case 8://SourceGreen:
        values->v[0] = values->v[1];
        //values->v[1] = values->v[1];
        values->v[2] = values->v[1];
        return;
    case 9://OneMinusSourceGreen:
        values->v[0] = 255 - values->v[1];
        values->v[2] = 255 - values->v[1];
        //Important!
        values->v[1] = 255 - values->v[1];
        return;
    case 0xC://SourceBlue:
        values->v[0] = values->v[2];
        values->v[1] = values->v[2];
        //values->v[2] = values->v[2];
        return;
    case 0xD://OneMinusSourceBlue:
        values->v[0] = 255 - values->v[2];
        values->v[1] = 255 - values->v[2];
        values->v[2] = 255 - values->v[2];
        return;
    default:
        GPUDEBUG("Unknown color factor %d\n", (int)factor);
        return;
    }
}
static u8 AlphaCombine(u32 op, struct clov3* input)
{
    switch (op) {
    case 0://Replace:
        return input->v[0];
    case 1://Modulate:
        return input->v[0] * input->v[1] / 255;
    case 2://Add:
        return input->v[0] + input->v[1];
    case 3://Add Signed:
        return input->v[0] + input->v[1] - 128;
    case 4://Lerp:
        return (input->v[0] * input->v[2] + input->v[1] * (255 - input->v[2])) / 255;
    case 5://Subtract:
        return input->v[0] - input->v[1];
    case 8://Multiply Addition:
        return (input->v[0] * input->v[1] / 255) + input->v[2];
    case 9://Addition Multiply:
        return (input->v[0] + input->v[1]) * input->v[2] / 255;
    default:
        GPUDEBUG("Unknown alpha combiner operation %d\n", (int)op);
        return 0;
    }
};

static void ColorCombine(u32 op, struct clov4 input[3])
{
    switch (op) {
    case 0://Replace:
        //return input[0];
        return;
    case 1://Modulate:
        (input)[0].v[0] = (input)[0].v[0] * (input)[1].v[0] / 255;
        (input)[0].v[1] = (input)[0].v[1] * (input)[1].v[1] / 255;
        (input)[0].v[2] = (input)[0].v[2] * (input)[1].v[2] / 255;
        return;  //((input[0] * input[1]) / 255);
    case 2://Add:
        (input)[0].v[0] = (input)[0].v[0] + (input)[1].v[0];
        (input)[0].v[1] = (input)[0].v[1] + (input)[1].v[1];
        (input)[0].v[2] = (input)[0].v[2] + (input)[1].v[2];
        return; //input->v[0] + input->v[1];
    case 3://Add Signed:
        (input)[0].v[0] = (input)[0].v[0] + (input)[1].v[0] - 128;
        (input)[0].v[1] = (input)[0].v[1] + (input)[1].v[1] - 128;
        (input)[0].v[2] = (input)[0].v[2] + (input)[1].v[2] - 128;
        return;
    case 4://Lerp:
        (input)[0].v[0] = (input)[0].v[0] * (input)[2].v[0] + (input)[1].v[0] * (255 - (input)[2].v[0]) / 255;
        (input)[0].v[1] = (input)[0].v[1] * (input)[2].v[1] + (input)[1].v[1] * (255 - (input)[2].v[1]) / 255;
        (input)[0].v[2] = (input)[0].v[2] * (input)[2].v[2] + (input)[1].v[2] * (255 - (input)[2].v[2]) / 255;
        return; //(input->v[0] * input->v[2] + input->v[1] * (255 - input->v[2])) / 255;
    case 5://Subtract:
        (input)[0].v[0] = (input)[0].v[0] - (input)[1].v[0];
        (input)[0].v[1] = (input)[0].v[1] - (input)[1].v[1];
        (input)[0].v[2] = (input)[0].v[2] - (input)[1].v[2];
        return;
    case 8://Multiply Addition:
        (input)[0].v[0] = ((input)[0].v[0] * (input)[1].v[0] / 255) + (input)[2].v[0];
        (input)[0].v[1] = ((input)[0].v[1] * (input)[1].v[1] / 255) + (input)[2].v[1];
        (input)[0].v[2] = ((input)[0].v[2] * (input)[1].v[2] / 255) + (input)[2].v[2];
        return;
    case 9://Addition Multiply:
        (input)[0].v[0] = ((input)[0].v[0] + (input)[1].v[0]) * (input)[2].v[0] / 255;
        (input)[0].v[1] = ((input)[0].v[1] + (input)[1].v[1]) * (input)[2].v[1] / 255;
        (input)[0].v[2] = ((input)[0].v[2] + (input)[1].v[2]) * (input)[2].v[2] / 255;
        return;
    default:
        GPUDEBUG("Unknown color combiner operation %d\n", (int)op);
    }
}
static u8 GetAlphaModifier(u32 factor, u8 value)
{
    switch (factor) {
    case 0://SourceAlpha:
        return value;
    case 1://OneMinusSourceAlpha:
        return 255 - value;
    //case 2://Red:
    //case 3://OneMinusRed:
    //case 4://Green:
    //case 5://OneMinusGreen:
    //case 6://Blue:
    //case 7://OneMinusBlue:
    default:
        //GPUDEBUG("Unknown alpha factor %d\n", (int)factor);
        return 0;
    }
}

typedef enum{
    Zero = 0,
    One = 1,

    SourceAlpha = 6,
    OneMinusSourceAlpha = 7,
    ConstantAlpha = 12,
    OneMinusConstantAlpha = 13,
} BlendFactor;
static void LookupFactorRGB(BlendFactor factor, struct clov4 *source, struct clov4 *output)
{
    switch(factor)
    {
        case Zero:
            output->v[0] = 0;
            output->v[1] = 0;
            output->v[2] = 0;
            break;
        case One:
            output->v[0] = 255;
            output->v[1] = 255;
            output->v[2] = 255;
            break;
        case SourceAlpha:
            output->v[0] = source->v[3];
            output->v[1] = source->v[3];
            output->v[2] = source->v[3];
            break;
        case OneMinusSourceAlpha:
            output->v[0] = 255 - source->v[3];
            output->v[1] = 255 - source->v[3];
            output->v[2] = 255 - source->v[3];
            break;
        case ConstantAlpha:
            output->v[0] = output->v[1] = output->v[2] = GPU_Regs[BLEND_COLOR] >> 24;
            break;
        case OneMinusConstantAlpha:
            output->v[0] = output->v[1] = output->v[2] = 255 - (GPU_Regs[BLEND_COLOR] >> 24);
            break;
        default:
            DEBUG("Unknown color blend factor %x\n", factor);
            break;
    }
}

static void LookupFactorA(BlendFactor factor, struct clov4 *source, struct clov4 *output)
{
    switch(factor)
    {
        case Zero:
            output->v[3] = 0;
            break;
        case One:
            output->v[3] = 255;
            break;
        case SourceAlpha:
            output->v[3] = source->v[3];
            break;
        case OneMinusSourceAlpha:
            output->v[3] = 255 - source->v[3];
            break;
        case ConstantAlpha:
            output->v[3] = GPU_Regs[BLEND_COLOR] >> 24;
            break;
        case OneMinusConstantAlpha:
            output->v[3] = 255-(GPU_Regs[BLEND_COLOR] >> 24);
            break;
        default:
            DEBUG("Unknown alpha blend factor %x\n", factor);
            break;
    }
}

typedef enum{
    ClampToEdge = 0,
    ClampToBorder = 1,
    Repeat = 2,
    MirrorRepeat = 3
} WrapMode;
static int GetWrappedTexCoord(WrapMode wrap, int val, int size)
{
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
    int ret = 0;
    switch(wrap)
    {
        case ClampToEdge:
            ret = MAX(val, 0);
            ret = MIN(ret, size - 1);
            break;
        case Repeat:
            ret = (int)((unsigned)val % size);
            break;
        //case MirrorRepeat:
            //val %= size;
            //val = ~val;
        //    break;
        default:
            DEBUG("Unknown wrap format %08X\n", wrap);
            ret = 0;
            break;
    }

    return ret;
#undef MAX
#undef MIN
}


static unsigned NibblesPerPixel(TextureFormat format) {
    switch(format) {
        case RGBA8:
            return 8;

        case RGB8:
            return 6;

        case RGBA5551:
        case RGB565:
        case RGBA4:
        case IA8:
            return 4;

        case A4:
            return 1;

        case I8:
        case A8:
        case IA4:
        default:  // placeholder for yet unknown formats
            return 2;
    }
}
const struct clov4 LookupTexture(const u8* source, int x, int y, const TextureFormat format, int stride, int width, int height, bool disable_alpha)
{
    //DEBUG("Format=%d\n", format);
    // Images are split into 8x8 tiles. Each tile is composed of four 4x4 subtiles each
    // of which is composed of four 2x2 subtiles each of which is composed of four texels.
    // Each structure is embedded into the next-bigger one in a diagonal pattern, e.g.
    // texels are laid out in a 2x2 subtile like this:
    // 2 3
    // 0 1
    //
    // The full 8x8 tile has the texels arranged like this:
    //
    // 42 43 46 47 58 59 62 63
    // 40 41 44 45 56 57 60 61
    // 34 35 38 39 50 51 54 55
    // 32 33 36 37 48 49 52 53
    // 10 11 14 15 26 27 30 31
    // 08 09 12 13 24 25 28 29
    // 02 03 06 07 18 19 22 23
    // 00 01 04 05 16 17 20 21

    // TODO(neobrain): Not sure if this swizzling pattern is used for all textures.
    // To be flexible in case different but similar patterns are used, we keep this
    // somewhat inefficient code around for now.
    int texel_index_within_tile = 0;
    for(int block_size_index = 0; block_size_index < 3; ++block_size_index) {
        int sub_tile_width = 1 << block_size_index;
        int sub_tile_height = 1 << block_size_index;

        int sub_tile_index = (x & sub_tile_width) << block_size_index;
        sub_tile_index += 2 * ((y & sub_tile_height) << block_size_index);
        texel_index_within_tile += sub_tile_index;
    }

    const int block_width = 8;
    const int block_height = 8;

    int coarse_x = (x / block_width) * block_width;
    int coarse_y = (y / block_height) * block_height;

    struct clov4 ret;

    switch(format) {
        case RGBA8:
        {
            const u8* source_ptr = source + coarse_x * block_height * 4 + coarse_y * stride + texel_index_within_tile * 4;
            ret.v[0] = source_ptr[3];
            ret.v[1] = source_ptr[2];
            ret.v[2] = source_ptr[1];
            ret.v[3] = disable_alpha ? 255 : source_ptr[0];
            break;
        }

        case RGB8:
        {
            const u8* source_ptr = source + coarse_x * block_height * 3 + coarse_y * stride + texel_index_within_tile * 3;
            ret.v[0] = source_ptr[0];
            ret.v[1] = source_ptr[1];
            ret.v[2] = source_ptr[2];
            ret.v[3] = 255;
            break;
        }

        case RGBA5551:
        {
            const u16 source_ptr = *(const u16*)(source + coarse_x * block_height * 2 + coarse_y * stride + texel_index_within_tile * 2);
            u8 r = (source_ptr >> 11) & 0x1F;
            u8 g = ((source_ptr) >> 6) & 0x1F;
            u8 b = (source_ptr >> 1) & 0x1F;
            u8 a = source_ptr & 1;

            ret.v[0] = (r << 3) | (r >> 2);
            ret.v[1] = (g << 3) | (g >> 2);
            ret.v[2] = (b << 3) | (b >> 2);
            ret.v[3] = disable_alpha ? 255 : (a * 255);
            break;
        }

        case RGB565:
        {
            const u16 source_ptr = *(const u16*)(source + coarse_x * block_height * 2 + coarse_y * stride + texel_index_within_tile * 2);
            u8 r = (source_ptr >> 11) & 0x1F;
            u8 g = ((source_ptr) >> 5) & 0x3F;
            u8 b = (source_ptr)& 0x1F;
            ret.v[0] = (r << 3) | (r >> 2);
            ret.v[1] = (g << 2) | (g >> 4);
            ret.v[2] = (b << 3) | (b >> 2);
            ret.v[3] = 255;
            break;
        }

        case RGBA4:
        {
            const u8* source_ptr = source + coarse_x * block_height * 2 + coarse_y * stride + texel_index_within_tile * 2;
            u8 r = source_ptr[1] >> 4;
            u8 g = source_ptr[1] & 0xFF;
            u8 b = source_ptr[0] >> 4;
            u8 a = source_ptr[0] & 0xFF;
            r = (r << 4) | r;
            g = (g << 4) | g;
            b = (b << 4) | b;
            a = (a << 4) | a;

            ret.v[0] = r;
            ret.v[1] = g;
            ret.v[2] = b;
            ret.v[3] = disable_alpha ? 255 : a;
            break;
        }

        case IA8:
        {
            const u8* source_ptr = source + coarse_x * block_height * 2 + coarse_y * stride + texel_index_within_tile * 2;

            // TODO: Better control this...
            if(disable_alpha) {
                ret.v[0] = *(source_ptr + 1);
                ret.v[1] = *source_ptr;
                ret.v[2] = 0;
                ret.v[3] = 255;
            }
            else {
                ret.v[0] = *source_ptr;
                ret.v[1] = *source_ptr;
                ret.v[2] = *source_ptr;
                ret.v[3] = *(source_ptr + 1);
            }
            break;
        }

        case I8:
        {
            const u8* source_ptr = source + coarse_x * block_height + coarse_y * stride + texel_index_within_tile;

            // TODO: Better control this...
            ret.v[0] = *source_ptr;
            ret.v[1] = *source_ptr;
            ret.v[2] = *source_ptr;
            ret.v[3] = 255;
            break;
        }

        case A8:
        {
            const u8* source_ptr = source + coarse_x * block_height + coarse_y * stride + texel_index_within_tile;

            // TODO: Better control this...
            if(disable_alpha) {
                ret.v[0] = *source_ptr;
                ret.v[1] = *source_ptr;
                ret.v[2] = *source_ptr;
                ret.v[3] = 255;
            }
            else {
                ret.v[0] = 0;
                ret.v[1] = 0;
                ret.v[2] = 0;
                ret.v[3] = *source_ptr;
            }
            break;
        }

        case IA4:
        {
            const u8* source_ptr = source + coarse_x * block_height + coarse_y * stride + texel_index_within_tile;

            // TODO: Order?
            u8 i = ((*source_ptr) & 0xF0) >> 4;
            u8 a = (*source_ptr) & 0xF;
            a |= a << 4;
            i |= i << 4;

            // TODO: Better control this...
            if(disable_alpha) {
                ret.v[0] = i;
                ret.v[1] = a;
                ret.v[2] = 0;
                ret.v[3] = 255;
            }
            else {
                ret.v[0] = i;
                ret.v[1] = i;
                ret.v[2] = i;
                ret.v[3] = a;
            }
            break;
        }

        case I4:
        {
            const u8* source_ptr = source + coarse_x * block_height / 2 + coarse_y * stride + texel_index_within_tile / 2;

            // TODO: Order?
            u8 i = (coarse_x % 2) ? ((*source_ptr) & 0xF) : (((*source_ptr) & 0xF0) >> 4);
            i |= i << 4;

            ret.v[0] = i;
            ret.v[1] = i;
            ret.v[2] = i;
            ret.v[3] = 255;
            break;
        }

        case A4:
        {
            const u8* source_ptr = source + coarse_x * block_height / 2 + coarse_y * stride + texel_index_within_tile / 2;

            // TODO: Order?
            u8 a = (coarse_x % 2) ? ((*source_ptr) & 0xF) : (((*source_ptr) & 0xF0) >> 4);
            a |= a << 4;

            // TODO: Better control this...
            if(disable_alpha) {
                ret.v[0] = a;
                ret.v[1] = a;
                ret.v[2] = a;
                ret.v[3] = 255;
            }
            else {
                ret.v[0] = 0;
                ret.v[1] = 0;
                ret.v[2] = 0;
                ret.v[3] = a;
            }
            break;
        }

        default:
            DEBUG("Unknown texture format: 0x%08X\n", (u32)format);
            break;
    }

    return ret;
}

void rasterizer_ProcessTriangle(const struct OutputVertex *v0,
                                const struct OutputVertex * v1,
                                const struct OutputVertex * v2)
{
#ifdef testtriang
    numb++;
#endif
    // NOTE: Assuming that rasterizer coordinates are 12.4 fixed-point values

    struct vec3_12P4 vtxpos[3];
    for (int i = 0; i < 3; i++)vtxpos[0].v[i] = (s16)(v0->screenpos.v[i] * 16.0f);
    for (int i = 0; i < 3; i++)vtxpos[1].v[i] = (s16)(v1->screenpos.v[i] * 16.0f);
    for (int i = 0; i < 3; i++)vtxpos[2].v[i] = (s16)(v2->screenpos.v[i] * 16.0f);
    // TODO: Proper scissor rect test!
    u16 min_x = min3(vtxpos[0].v[0], vtxpos[1].v[0], vtxpos[2].v[0]);
    u16 min_y = min3(vtxpos[0].v[1], vtxpos[1].v[1], vtxpos[2].v[1]);
    u16 max_x = max3(vtxpos[0].v[0], vtxpos[1].v[0], vtxpos[2].v[0]);
    u16 max_y = max3(vtxpos[0].v[1], vtxpos[1].v[1], vtxpos[2].v[1]);
    min_x &= IntMask;
    min_y &= IntMask;
    max_x = ((max_x + FracMask) & IntMask);
    max_y = ((max_y + FracMask) & IntMask);
    // Triangle filling rules: Pixels on the right-sided edge or on flat bottom edges are not
    // drawn. Pixels on any other triangle border are drawn. This is implemented with three bias
    // values which are added to the barycentric coordinates w0, w1 and w2, respectively.
    // NOTE: These are the PSP filling rules. Not sure if the 3DS uses the same ones...

    int bias0 = IsRightSideOrFlatBottomEdge(&vtxpos[0], &vtxpos[1], &vtxpos[2]) ? -1 : 0;
    int bias1 = IsRightSideOrFlatBottomEdge(&vtxpos[1], &vtxpos[2], &vtxpos[0]) ? -1 : 0;
    int bias2 = IsRightSideOrFlatBottomEdge(&vtxpos[2], &vtxpos[0], &vtxpos[1]) ? -1 : 0;
    // TODO: Not sure if looping through x first might be faster
    for (u16 y = min_y; y < max_y; y += 0x10) {
        for (u16 x = min_x; x < max_x; x += 0x10) {
            // Calculate the barycentric coordinates w0, w1 and w2
            /*auto orient2d = [](const Math::Vec2<Fix12P4>& vtx1,
                const Math::Vec2<Fix12P4>& vtx2,
                const Math::Vec2<Fix12P4>& vtx3) {
                const auto vec1 = Math::MakeVec(vtx2 - vtx1, 0);
                const auto vec2 = Math::MakeVec(vtx3 - vtx1, 0);
                // TODO: There is a very small chance this will overflow for sizeof(int) == 4
                return Math::Cross(vec1, vec2).z;
            };*/
            int w0 = bias0 + orient2d(vtxpos[1].v[0], vtxpos[1].v[1], vtxpos[2].v[0], vtxpos[2].v[1], x, y );
            int w1 = bias1 + orient2d(vtxpos[2].v[0], vtxpos[2].v[1], vtxpos[0].v[0], vtxpos[0].v[1], x, y );
            int w2 = bias2 + orient2d(vtxpos[0].v[0], vtxpos[0].v[1], vtxpos[1].v[0], vtxpos[1].v[1], x, y );
            int wsum = w0 + w1 + w2;
            // If current pixel is not covered by the current primitive
            if (w0 < 0 || w1 < 0 || w2 < 0)
                continue;
            // Perspective correct attribute interpolation:
            // Attribute values cannot be calculated by simple linear interpolation since
            // they are not linear in screen space. For example, when interpolating a
            // texture coordinate across two vertices, something simple like
            // u = (u0*w0 + u1*w1)/(w0+w1)
            // will not work. However, the attribute value divided by the
            // clipspace w-coordinate (u/w) and and the inverse w-coordinate (1/w) are linear
            // in screenspace. Hence, we can linearly interpolate these two independently and
            // calculate the interpolated attribute by dividing the results.
            // I.e.
            // u_over_w = ((u0/v0.pos.w)*w0 + (u1/v1.pos.w)*w1)/(w0+w1)
            // one_over_w = (( 1/v0.pos.w)*w0 + ( 1/v1.pos.w)*w1)/(w0+w1)
            // u = u_over_w / one_over_w
            //
            // The generalization to three vertices is straightforward in baricentric coordinates.

            struct clov4 primary_color;
            primary_color.v[0] = (u8)(GetInterpolatedAttribute(v0->color.v[0], v1->color.v[0], v2->color.v[0], v0, v1, v2, (float)w0, (float)w1, (float)w2) * 255.f);
            primary_color.v[1] = (u8)(GetInterpolatedAttribute(v0->color.v[1], v1->color.v[1], v2->color.v[1], v0, v1, v2, (float)w0, (float)w1, (float)w2) * 255.f);
            primary_color.v[2] = (u8)(GetInterpolatedAttribute(v0->color.v[2], v1->color.v[2], v2->color.v[2], v0, v1, v2, (float)w0, (float)w1, (float)w2) * 255.f);
            primary_color.v[3] = (u8)(GetInterpolatedAttribute(v0->color.v[3], v1->color.v[3], v2->color.v[3], v0, v1, v2, (float)w0, (float)w1, (float)w2) * 255.f);
            /*Math::Vec4<u8> primary_color{
                (u8)(GetInterpolatedAttribute(v0.color.r(), v1.color.r(), v2.color.r()).ToFloat32() * 255),
                (u8)(GetInterpolatedAttribute(v0.color.g(), v1.color.g(), v2.color.g()).ToFloat32() * 255),
                (u8)(GetInterpolatedAttribute(v0.color.b(), v1.color.b(), v2.color.b()).ToFloat32() * 255),
                (u8)(GetInterpolatedAttribute(v0.color.a(), v1.color.a(), v2.color.a()).ToFloat32() * 255)
            };*/
            struct clov4 texture_color[4];

            float u[3],v[3];
            u[0] = GetInterpolatedAttribute(v0->tc0.v[0], v1->tc0.v[0], v2->tc0.v[0], v0, v1, v2, (float)w0, (float)w1, (float)w2);
            v[0] = GetInterpolatedAttribute(v0->tc0.v[1], v1->tc0.v[1], v2->tc0.v[1], v0, v1, v2, (float)w0, (float)w1, (float)w2);
            u[1] = GetInterpolatedAttribute(v0->tc1.v[0], v1->tc1.v[0], v2->tc1.v[0], v0, v1, v2, (float)w0, (float)w1, (float)w2);
            v[1] = GetInterpolatedAttribute(v0->tc1.v[1], v1->tc1.v[1], v2->tc1.v[1], v0, v1, v2, (float)w0, (float)w1, (float)w2);
            u[2] = GetInterpolatedAttribute(v0->tc2.v[0], v1->tc2.v[0], v2->tc2.v[0], v0, v1, v2, (float)w0, (float)w1, (float)w2);
            v[2] = GetInterpolatedAttribute(v0->tc2.v[1], v1->tc2.v[1], v2->tc2.v[1], v0, v1, v2, (float)w0, (float)w1, (float)w2);

            for (int i = 0; i < 3; ++i) {
                if (GPU_Regs[TEXTURINGSETINGS80] & (0x1<<i)) {
                    u8* texture_data = NULL;
                    switch (i) {
                    case 0:
                        texture_data = (u8*)(get_pymembuffer(GPU_Regs[TEXTURCONFIG0ADDR] << 3));
                        break;
                    case 1:
                        texture_data = (u8*)(get_pymembuffer(GPU_Regs[TEXTURCONFIG1ADDR] << 3));
                        break;
                    case 2:
                        texture_data = (u8*)(get_pymembuffer(GPU_Regs[TEXTURCONFIG2ADDR] << 3));
                        break;
                    }

                    int s=0;
                    int t=0;
                    int row_stride=0;
                    int format=0;
                    int height=0;
                    int width=0;
                    int wrap_s=0;
                    int wrap_t=0;
                    switch (i) {
                    case 0:
                        height = (GPU_Regs[TEXTURCONFIG0SIZE] & 0xFFFF);
                        width = (GPU_Regs[TEXTURCONFIG0SIZE] >> 16) & 0xFFFF;
                        wrap_s = (GPU_Regs[TEXTURCONFIG0WRAP] >> 8) & 3;
                        wrap_t = (GPU_Regs[TEXTURCONFIG0WRAP] >> 12) & 3;
                        format = GPU_Regs[TEXTURCONFIG0TYPE] & 0xF;
                        break;
                    case 1:
                        height = (GPU_Regs[TEXTURCONFIG1SIZE] & 0xFFFF);
                        width = (GPU_Regs[TEXTURCONFIG1SIZE] >> 16) & 0xFFFF;
                        wrap_s = (GPU_Regs[TEXTURCONFIG1WRAP] >> 8) & 3;
                        wrap_t = (GPU_Regs[TEXTURCONFIG1WRAP] >> 12) & 3;
                        format = GPU_Regs[TEXTURCONFIG1TYPE] & 0xF;
                        break;
                    case 2:
                        height = (GPU_Regs[TEXTURCONFIG2SIZE] & 0xFFFF);
                        width = (GPU_Regs[TEXTURCONFIG2SIZE] >> 16) & 0xFFFF;
                        wrap_s = (GPU_Regs[TEXTURCONFIG2WRAP] >> 8) & 3;
                        wrap_t = (GPU_Regs[TEXTURCONFIG2WRAP] >> 12) & 3;
                        format = GPU_Regs[TEXTURCONFIG2TYPE] & 0xF;
                        break;
                    }
                    
                    s = (int)(u[i] * (width*1.0f));
                    s = GetWrappedTexCoord((WrapMode)wrap_s, s, width);
                    t = (int)(v[i] * (height*1.0f));
                    t = GetWrappedTexCoord((WrapMode)wrap_t, t, height);
                    row_stride = NibblesPerPixel(format) * width / 2;

                    //Flip it vertically
                    t = height - 1 - t;
                    //TODO: Fix this up so it works correctly.

                    /*int texel_index_within_tile = 0;
                    for (int block_size_index = 0; block_size_index < 3; ++block_size_index) {
                        int sub_tile_width = 1 << block_size_index;
                        int sub_tile_height = 1 << block_size_index;
                        int sub_tile_index = (s & sub_tile_width) << block_size_index;
                        sub_tile_index += 2 * ((t & sub_tile_height) << block_size_index);
                        texel_index_within_tile += sub_tile_index;
                    }
                    const int block_width = 8;
                    const int block_height = 8;
                    int coarse_s = (s / block_width) * block_width;
                    int coarse_t = (t / block_height) * block_height;

                    u8* source_ptr = (u8*)texture_data + coarse_s * block_height * 3 + coarse_t * row_stride + texel_index_within_tile * 3;
                    texture_color[i].v[0] = source_ptr[2];
                    texture_color[i].v[1] = source_ptr[1];
                    texture_color[i].v[2] = source_ptr[0];
                    texture_color[i].v[3] = 0xFF;*/

                    struct clov4 temp = LookupTexture(texture_data, s, t, format, row_stride, width, height, false);
                    texture_color[i].v[0] = temp.v[0];
                    texture_color[i].v[1] = temp.v[1];
                    texture_color[i].v[2] = temp.v[2];
                    texture_color[i].v[3] = temp.v[3];

                    /*FILE *f = fopen("test.bin", "wb");
                    fwrite(texture_data, 1, 0x400000, f);
                    fclose(f);*/
                }
            }
            struct clov4 combiner_output;
            combiner_output.v[3] = 0xFF;

            struct clov4 comb_buf[5];
            comb_buf[0].v[0] = GPU_Regs[0xFD] & 0xFF;
            comb_buf[0].v[1] = (GPU_Regs[0xFD] >> 8) & 0xFF;
            comb_buf[0].v[2] = (GPU_Regs[0xFD] >> 16) & 0xFF;
            comb_buf[0].v[3] = (GPU_Regs[0xFD] >> 24) & 0xFF;

            for (int i = 0; i < 6; i++) {
                u32 regnumaddr = GLTEXENV + i * 8;
                if (i > 3) regnumaddr += 0x10;

                if (i > 0 && i < 5) {
                    if (((GPU_Regs[0xE0] >> (i + 7)) & 1) == 1) {
                        comb_buf[i].v[0] = combiner_output.v[0];
                        comb_buf[i].v[1] = combiner_output.v[1];
                        comb_buf[i].v[2] = combiner_output.v[2];
                    } else {
                        comb_buf[i].v[0] = comb_buf[i - 1].v[0];
                        comb_buf[i].v[1] = comb_buf[i - 1].v[1];
                        comb_buf[i].v[2] = comb_buf[i - 1].v[2];
                    }
                    if (((GPU_Regs[0xE0] >> (i + 11)) & 1) == 1) {
                        comb_buf[i].v[3] = combiner_output.v[3];
                    } else {
                        comb_buf[i].v[3] = comb_buf[i - 1].v[3];
                    }
                }

                // struct clov3 color_result[3]; = {
                //    GetColorSource(tev_stage.color_source1)),
                //     GetColorSource(tev_stage.color_source2)),
                //    GetColorSource(tev_stage.color_source3))
                //     };
                struct clov4 color_result[3];

                for (int j = 0; j < 3; j++) {
                    switch ((GPU_Regs[regnumaddr] >> (j * 4))&0xF) {
                    case 0://PrimaryColor
                        memcpy(&color_result[j], &primary_color, sizeof(struct clov4));
                        break;
                    //case 1://PrimaryFragmentColor:
                    //case 2://SecondaryFragmentColor:
                    case 3: //Texture0
                        memcpy(&color_result[j], &texture_color[0], sizeof(struct clov4));
                        break;
                    case 4: //Texture1
                        memcpy(&color_result[j], &texture_color[1], sizeof(struct clov4));
                        break;
                    case 5: //Texture2
                        memcpy(&color_result[j], &texture_color[2], sizeof(struct clov4));
                        break;
                    //case 6://Texture 3 (proctex):
                    case 0xD://PreviousBuffer:
                        //prevent errors if the tevstages are bad
                        if(i > 0) {
                            color_result[j].v[0] = comb_buf[i - 1].v[0];
                            color_result[j].v[1] = comb_buf[i - 1].v[1];
                            color_result[j].v[2] = comb_buf[i - 1].v[2];
                            color_result[j].v[3] = comb_buf[i - 1].v[3];
                        }
                    case 0xE: //Constant
                        color_result[j].v[0] = GPU_Regs[regnumaddr + 3] & 0xFF;
                        color_result[j].v[1] = (GPU_Regs[regnumaddr + 3] >> 8) & 0xFF;
                        color_result[j].v[2] = (GPU_Regs[regnumaddr + 3] >> 0x10) & 0xFF;
                        color_result[j].v[3] = (GPU_Regs[regnumaddr + 3] >> 24) & 0xFF;
                        break;
                    case 0xF://Previous
                        memcpy(&color_result[j], &combiner_output, sizeof(struct clov4));
                        break;
                    default:
                        GPUDEBUG("Unknown color combiner source %d\n", (int)(GPU_Regs[regnumaddr] >> (j * 4)) & 0xF);
                        break;
                    }
                }
                GetColorModifier((GPU_Regs[regnumaddr + 1] >> 0) & 0xF, &color_result[0]);
                GetColorModifier((GPU_Regs[regnumaddr + 1] >> 4) & 0xF, &color_result[1]);
                GetColorModifier((GPU_Regs[regnumaddr + 1] >> 8) & 0xF, &color_result[2]);
                // color combiner
                // NOTE: Not sure if the alpha combiner might use the color output of the previous
                // stage as input. Hence, we currently don't directly write the result to
                // combiner_output.rgb(), but instead store it in a temporary variable until
                // alpha combining has been done.
                ColorCombine(GPU_Regs[regnumaddr + 2] & 0xF, &color_result[0]);
                struct clov3 alpha_result;
                for (int j = 0; j < 3; j++) {
                    u8 alpha = 0;
                    switch ((GPU_Regs[regnumaddr] >> (16 + j * 4)) & 0xF) {
                    case 0://PrimaryColor:
                        alpha = primary_color.v[3];
                        break;
                    //case 1://PrimaryFragmentColor:
                    //case 2://SecondaryFragmentColor:
                    case 3://Texture0:
                        alpha = texture_color[0].v[3];
                        break;
                    case 4://Texture1:
                        alpha = texture_color[1].v[3];
                        break;
                    case 5://Texture2:
                        alpha = texture_color[2].v[3];
                        break;
                    //case 6://Texture 3 (proctex):
                    case 0xD://PreviousBuffer:
                        //prevent errors if the tevstages are bad
                        if(i > 0) alpha = comb_buf[i - 1].v[3];
                    case 0xE://Constant:
                        alpha = (GPU_Regs[regnumaddr + 3] >> 0x18) & 0xFF;
                        break;
                    case 0xF://Previous:
                        alpha = combiner_output.v[3];
                        break;
                    default:
                        GPUDEBUG("Unknown alpha combiner source %d\n", (int)((GPU_Regs[regnumaddr] >> (16 + j * 4))) & 0xF);
                        break;
                    }
                    alpha_result.v[j] = GetAlphaModifier((GPU_Regs[regnumaddr + 1] >> (12 + 3 * j)) & 0x7, alpha);
                }
                color_result[0].v[3] = AlphaCombine((GPU_Regs[regnumaddr + 2] >> 16) & 0xF, &alpha_result);
                memcpy(&combiner_output, &color_result[0], sizeof(struct clov4));
            }

            // TODO: Does depth indeed only get written even if depth testing is enabled?
            if(GPU_Regs[DEPTHTEST_CONFIG] & 1)
            {
                u16 z = (u16)(-((float)v0->screenpos.v[2] * w0 +
                    (float)v1->screenpos.v[2] * w1 +
                    (float)v2->screenpos.v[2] * w2) * 65535.f / wsum); // TODO: Shouldn't need to multiply by 65536?

                u16 ref_z = GetDepth(x >> 4, y >> 4);

                bool pass = false;

                switch((GPU_Regs[DEPTHTEST_CONFIG]>>4)&7) {
                    case 1: //Always
                        pass = true;
                        break;

                    case 6: //GreaterThan
                        pass = z > ref_z;
                        break;

                    default:
                        DEBUG("Unknown depth test function %x\n", (GPU_Regs[DEPTHTEST_CONFIG] >> 4) & 7);
                        break;
                }

                if(!pass)
                    continue;

                if((GPU_Regs[DEPTHTEST_CONFIG]>>12) & 1)
                    SetDepth(x >> 4, y >> 4, z);
            }

            //Alpha blending
            if((GPU_Regs[COLOROUTPUT_CONFIG] >> 8) & 1) //Alpha blending enabled?
            {
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
                struct clov4 dest, srcfactor, dstfactor, result;
                GetPixel(x >> 4, y >> 4, &dest);

                u32 factor_source_rgb = (GPU_Regs[BLEND_CONFIG] >> 16) & 0xF;
                u32 factor_source_a = (GPU_Regs[BLEND_CONFIG] >> 24) & 0xF;
                LookupFactorRGB(factor_source_rgb, &combiner_output, &srcfactor);
                LookupFactorA(factor_source_a, &combiner_output, &srcfactor);

                u32 factor_dest_rgb = (GPU_Regs[BLEND_CONFIG] >> 20) & 0xF;
                u32 factor_dest_a = (GPU_Regs[BLEND_CONFIG] >> 28) & 0xF;
                LookupFactorRGB(factor_dest_rgb, &combiner_output, &dstfactor);
                LookupFactorA(factor_dest_a, &combiner_output, &dstfactor);

                u32 blend_equation_rgb = GPU_Regs[BLEND_CONFIG] & 0xFF;
                u32 blend_equation_a = (GPU_Regs[BLEND_CONFIG]>>8) & 0xFF;
                switch(blend_equation_rgb)
                {
                    case 0: //Add
                        result.v[0] = CLAMP((((int)combiner_output.v[0] * (int)srcfactor.v[0] + (int)dest.v[0] * (int)dstfactor.v[0]) / 255), 0, 255);
                        result.v[1] = CLAMP((((int)combiner_output.v[1] * (int)srcfactor.v[1] + (int)dest.v[1] * (int)dstfactor.v[1]) / 255), 0, 255);
                        result.v[2] = CLAMP((((int)combiner_output.v[2] * (int)srcfactor.v[2] + (int)dest.v[2] * (int)dstfactor.v[2]) / 255), 0, 255);
                        break;
                    case 1: //Subtract
                        result.v[0] = CLAMP((((int)combiner_output.v[0] * (int)srcfactor.v[0] - (int)dest.v[0] * (int)dstfactor.v[0]) / 255), 0, 255);
                        result.v[1] = CLAMP((((int)combiner_output.v[1] * (int)srcfactor.v[1] - (int)dest.v[1] * (int)dstfactor.v[1]) / 255), 0, 255);
                        result.v[2] = CLAMP((((int)combiner_output.v[2] * (int)srcfactor.v[2] - (int)dest.v[2] * (int)dstfactor.v[2]) / 255), 0, 255);
                        break;
                    case 2: //Reverse Subtract
                        result.v[0] = CLAMP((((int)dest.v[0] * (int)dstfactor.v[0] - (int)combiner_output.v[0] * (int)srcfactor.v[0]) / 255), 0, 255);
                        result.v[1] = CLAMP((((int)dest.v[1] * (int)dstfactor.v[1] - (int)combiner_output.v[1] * (int)srcfactor.v[1]) / 255), 0, 255);
                        result.v[2] = CLAMP((((int)dest.v[2] * (int)dstfactor.v[2] - (int)combiner_output.v[2] * (int)srcfactor.v[2]) / 255), 0, 255);
                        break;
                    default:
                        DEBUG("Unknown RGB blend equation %x", blend_equation_rgb);
                        break;
                }

                switch(blend_equation_a)
                {
                    case 0: //Add
                        result.v[3] = CLAMP((((int)combiner_output.v[3] * (int)srcfactor.v[3] + (int)dest.v[3] * (int)dstfactor.v[3]) / 255), 0, 255);
                        break;
                    case 1: //Subtract
                        result.v[3] = CLAMP((((int)combiner_output.v[3] * (int)srcfactor.v[3] - (int)dest.v[3] * (int)dstfactor.v[3]) / 255), 0, 255);
                        break;
                    case 2: //Reverse Subtract
                        result.v[3] = CLAMP((((int)dest.v[3] * (int)dstfactor.v[3] - (int)combiner_output.v[3] * (int)srcfactor.v[3]) / 255), 0, 255);
                        break;
                    default:
                        DEBUG("Unknown A blend equation %x", blend_equation_a);
                        break;
                }

                memcpy(&combiner_output, &result, sizeof(struct clov4));
#undef MIN
            }
            else
            {
                DEBUG("logic op: %x", GPU_Regs[COLORLOGICOP_CONFIG] & 0xF);
            }

            /*struct clov4 combiner_output;
            combiner_output.v[0] = 0x0;
            combiner_output.v[1] = 0x0;
            combiner_output.v[2] = 0x0;
            combiner_output.v[3] = 0x0;*/

            DrawPixel((x >> 4), (y >> 4), &combiner_output);
        }
    }
}
