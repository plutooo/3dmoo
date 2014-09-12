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
//this is based on citra, copyright (C) 2014 Tony Wasserka.

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "gpu.h"

struct vec3_12P4 {
    s16 v[3];//x, y,z;
};

u16 min3(s16 v1,s16 v2,s16 v3)
{
    if (v1 < v2 && v1 < v3) return v1;
    if (v2 < v3) return v2;
    return v3;
}
u16 max3(s16 v1, s16 v2, s16 v3)
{
    if (v1 > v2 && v1 > v3) return v1;
    if (v2 > v3) return v2;
    return v3;
}

#define IntMask 0xFFF0
#define FracMask 0xF

bool IsRightSideOrFlatBottomEdge(struct vec3_12P4 * vtx, struct vec3_12P4 *line1, struct vec3_12P4 *line2)
{
    if (line1->v[1] == line2->v[1]) {
        // just check if vertex is above us => bottom line parallel to x-axis
        return vtx->v[1] < line1->v[1];
    }
    else {
        // check if vertex is on our left => right side
        // TODO: Not sure how likely this is to overflow
        return (int)vtx->v[0] < (int)line1->v[0] + ((int)line2->v[0] - (int)line1->v[0]) * ((int)vtx->v[1] - (int)line1->v[1]) / ((int)line2->v[1] - (int)line1->v[1]);
    }
}

s32 orient2d(u16 vtx1x, u16  vtx1y, u16  vtx2x, u16  vtx2y, u16  vtx3x, u16  vtx3y)
{
    s32 vec1x = vtx2x - vtx1x;
    s32 vec1y = vtx2y - vtx1y;
    s32 vec2x = vtx3x - vtx1x;
    s32 vec2y = vtx3y - vtx1y;
    // TODO: There is a very small chance this will overflow for sizeof(int) == 4
    return vec1x*vec2y - vec1y*vec2x;
}

static void SetDepth(int x, int y, u16 value) {
    u16* depth_buffer = (u16*)get_pymembuffer(GPUregs[DEPTHBUFFER_ADDRESS] << 3);

    // Assuming 16-bit depth buffer format until actual format handling is implemented
    *(depth_buffer + x + y * (GPUregs[Framebuffer_FORMAT11E] & 0xFFF) / 2) = value;
}
static void DrawPixel(int x, int y, const struct clov4* color) {
    u8* color_buffer = (u8*)get_pymembuffer(GPUregs[COLORBUFFER_ADDRESS] << 3);

    // Assuming RGB8 format until actual framebuffer format handling is implemented
    *(color_buffer + x * 3 + y * (GPUregs[Framebuffer_FORMAT11E] & 0xFFF) /2 * 3) = color->v[0];
    *(color_buffer + x * 3 + y * (GPUregs[Framebuffer_FORMAT11E] & 0xFFF) / 2 * 3 + 1) = color->v[1];
    *(color_buffer + x * 3 + y * (GPUregs[Framebuffer_FORMAT11E] & 0xFFF) / 2 * 3 + 2) = color->v[2];
}
static float GetInterpolatedAttribute(float attr0, float attr1, float attr2, const struct OutputVertex *v0, const struct OutputVertex * v1, const struct OutputVertex * v2,float w0,float w1, float w2)
{
    float interpolated_attr_over_w = (attr0 / v0->pos.v[3])*w0 + (attr1 / v1->pos.v[3])*w1 + (attr2 / v2->pos.v[3])*w2;
    float interpolated_w_inverse = ((1.f) / v0->pos.v[3])*w0 + ((1.f) / v1->pos.v[3])*w1 + ((1.f) / v2->pos.v[3])*w2;
    return interpolated_attr_over_w / interpolated_w_inverse;
}
void GetColorModifier(u32 factor, struct clov3 * values)
{
    switch (factor)
    {
    case 0: //SourceColor
        return;
    default:
        DEBUG("Unknown color factor %d\n", (int)factor);
        return;
    }
}
u8 AlphaCombine(u32 op, struct clov3* input)
{
    switch (op) {
    case 0://Replace:
        return input->v[0];
    case 1://Modulate:
        return input->v[0] * input->v[1] / 255;
    default:
        DEBUG("Unknown alpha combiner operation %d\n", (int)op);
        return 0;
    }
};

void ColorCombine(u32 op, struct clov3 input[3])
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
    default:
        DEBUG("Unknown color combiner operation %d\n", (int)op);
    }
}
u8 GetAlphaModifier(u32 factor, u8 value){
    switch (factor) {
    case 0://SourceAlpha:
        return value;
    default:
        DEBUG("Unknown color factor %d\n", (int)factor);
        return 0;
    }
}
void rasterizer_ProcessTriangle(const struct OutputVertex *v0,
    const struct OutputVertex * v1,
    const struct OutputVertex * v2)
{
    // NOTE: Assuming that rasterizer coordinates are 12.4 fixed-point values

    struct vec3_12P4 vtxpos[3];
    for (int i = 0; i < 3; i++)vtxpos[0].v[i] = v0->screenpos.v[i] * 16.0f;
    for (int i = 0; i < 3; i++)vtxpos[1].v[i] = v1->screenpos.v[i] * 16.0f;
    for (int i = 0; i < 3; i++)vtxpos[2].v[i] = v2->screenpos.v[i] * 16.0f;
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
            primary_color.v[0] = GetInterpolatedAttribute(v0->color.v[0], v1->color.v[0], v2->color.v[0], v0, v1, v2, w0, w1, w2) * 255;
            primary_color.v[1] = GetInterpolatedAttribute(v0->color.v[1], v1->color.v[1], v2->color.v[1], v0, v1, v2, w0, w1, w2) * 255;
            primary_color.v[2] = GetInterpolatedAttribute(v0->color.v[2], v1->color.v[2], v2->color.v[2], v0, v1, v2, w0, w1, w2) * 255;
            primary_color.v[3] = GetInterpolatedAttribute(v0->color.v[3], v1->color.v[3], v2->color.v[3], v0, v1, v2, w0, w1, w2) * 255;
            /*Math::Vec4<u8> primary_color{
                (u8)(GetInterpolatedAttribute(v0.color.r(), v1.color.r(), v2.color.r()).ToFloat32() * 255),
                (u8)(GetInterpolatedAttribute(v0.color.g(), v1.color.g(), v2.color.g()).ToFloat32() * 255),
                (u8)(GetInterpolatedAttribute(v0.color.b(), v1.color.b(), v2.color.b()).ToFloat32() * 255),
                (u8)(GetInterpolatedAttribute(v0.color.a(), v1.color.a(), v2.color.a()).ToFloat32() * 255)
            };*/
            struct clov4 texture_color;
            
            float u = GetInterpolatedAttribute(v0->tc0.v[0], v1->tc0.v[0], v2->tc0.v[0], v0, v1, v2, w0, w1, w2);
            float v = GetInterpolatedAttribute(v0->tc0.v[1], v1->tc0.v[1], v2->tc0.v[1], v0, v1, v2, w0, w1, w2);
            if (GPUregs[TEXTURINGSETINGS80] & 0x1) {
                // TODO: This is currently hardcoded for RGB8
                u32* texture_data = (u32*)(get_pymembuffer(GPUregs[TEXTURCONFIG0ADDR] << 3));
                int s = (int)(u * (GPUregs[TEXTURCONFIG0SIZE] >> 16));
                int t = (int)(v * (GPUregs[TEXTURCONFIG0SIZE]&0xFFFF));
                int texel_index_within_tile = 0;
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
                const int row_stride = (GPUregs[TEXTURCONFIG0SIZE] >> 16) * 3;
                u8* source_ptr = (u8*)texture_data + coarse_s * block_height * 3 + coarse_t * row_stride + texel_index_within_tile * 3;
                texture_color.v[0] = source_ptr[2];
                texture_color.v[1] = source_ptr[1];
                texture_color.v[2] = source_ptr[0];
                texture_color.v[3] = 0xFF;
            }
            struct clov4 combiner_output;
            combiner_output.v[3] = 0xFF;
            for (int i = 0; i < 6; i++)
            {
                u32 regnumaddr = GLTEXENV + i * 8;
                if (i > 3)regnumaddr += 0x10;

                struct clov3 color_result[3]; /*= {
                    GetColorSource(tev_stage.color_source1)),
                    GetColorSource(tev_stage.color_source2)),
                    GetColorSource(tev_stage.color_source3))
                    };*/
                for (int j = 0; j < 3; j++)
                {
                    switch ((GPUregs[regnumaddr] >> (j * 4))&0xF) {
                    case 0://PrimaryColor
                        memcpy(&color_result[j], &primary_color, sizeof(struct clov3));
                        break;
                    case 3: //Texture0
                        memcpy(&color_result[j], &texture_color, sizeof(struct clov3));
                        break;
                    case 0xE: //Constant
                        color_result[j].v[0] = GPUregs[regnumaddr + 3] & 0xFF;
                        color_result[j].v[1] = (GPUregs[regnumaddr + 3] >> 8) & 0xFF;
                        color_result[j].v[2] = (GPUregs[regnumaddr + 3] >> 0x10) & 0xFF;
                        break;
                    case 0xF://Previous
                        memcpy(&color_result[j], &combiner_output, sizeof(struct clov3));
                        break;
                    default:
                        DEBUG("Unknown color combiner source %d\n", (int)(GPUregs[regnumaddr] >> (j * 4)) & 0xF);
                        break;
                    }
                }
                GetColorModifier((GPUregs[regnumaddr + 1] >> 0) & 0xF, &color_result[0]);
                GetColorModifier((GPUregs[regnumaddr + 1] >> 4) & 0xF, &color_result[1]);
                GetColorModifier((GPUregs[regnumaddr + 1] >> 8) & 0xF, &color_result[2]);
                // color combiner
                // NOTE: Not sure if the alpha combiner might use the color output of the previous
                // stage as input. Hence, we currently don't directly write the result to
                // combiner_output.rgb(), but instead store it in a temporary variable until
                // alpha combining has been done.
                ColorCombine(GPUregs[regnumaddr + 2] & 0xF, color_result);
                struct clov3 alpha_result;
                for (int j = 0; j < 3; j++)
                {
                    u8 alpha = 0;
                    switch ((GPUregs[regnumaddr] >> (16 + j * 4)) & 0xF)
                    {
                    case 0://PrimaryColor:
                        alpha = primary_color.v[3];
                        break;
                    case 3://Texture0:
                        alpha = texture_color.v[3];
                        break;
                    case 0xE://Constant:
                        alpha = (GPUregs[regnumaddr + 3] >> 0x18) & 0xFF;
                        break;
                    case 0xF://Previous:
                        alpha = combiner_output.v[3];
                        break;
                    default:
                        DEBUG("Unknown alpha combiner source %d\n", (int)((GPUregs[regnumaddr] >> (16 + j * 4))) & 0xF);
                        break;
                    }
                    alpha_result.v[j] = GetAlphaModifier((GPUregs[regnumaddr + 1] >> (12 + 3 * j)) & 0x7, alpha);
                }
                color_result[0].v[3] = AlphaCombine((GPUregs[regnumaddr + 2] >> 16) & 0xF, &alpha_result);
                memcpy(&combiner_output, &color_result[0], sizeof(struct clov4));
            }
            u16 z = (u16)(((float)v0->screenpos.v[2] * w0 +
                (float)v1->screenpos.v[2] * w1 +
                (float)v2->screenpos.v[2] * w2) * 65535.f / wsum); // TODO: Shouldn't need to multiply by 65536?
            
            SetDepth(x >> 4, y >> 4, z);

            /*struct clov4 combiner_output;
            combiner_output.v[0] = 0x0;
            combiner_output.v[1] = 0x0;
            combiner_output.v[2] = 0x0;
            combiner_output.v[3] = 0x0;*/

            DrawPixel(x >> 4, y >> 4, &combiner_output);
        }
    }
}
