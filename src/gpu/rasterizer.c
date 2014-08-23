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
            /*auto GetInterpolatedAttribute = [&](float24 attr0, float24 attr1, float24 attr2) {
                auto attr_over_w = Math::MakeVec(attr0 / v0.pos.w,
                    attr1 / v1.pos.w,
                    attr2 / v2.pos.w);
                auto w_inverse = Math::MakeVec(float24::FromFloat32(1.f) / v0.pos.w,
                    float24::FromFloat32(1.f) / v1.pos.w,
                    float24::FromFloat32(1.f) / v2.pos.w);
                auto baricentric_coordinates = Math::MakeVec(float24::FromFloat32(w0),
                    float24::FromFloat32(w1),
                    float24::FromFloat32(w2));
                float24 interpolated_attr_over_w = Math::Dot(attr_over_w, baricentric_coordinates);
                float24 interpolated_w_inverse = Math::Dot(w_inverse, baricentric_coordinates);
                return interpolated_attr_over_w / interpolated_w_inverse;
            };
            Math::Vec4<u8> primary_color{
                (u8)(GetInterpolatedAttribute(v0.color.r(), v1.color.r(), v2.color.r()).ToFloat32() * 255),
                (u8)(GetInterpolatedAttribute(v0.color.g(), v1.color.g(), v2.color.g()).ToFloat32() * 255),
                (u8)(GetInterpolatedAttribute(v0.color.b(), v1.color.b(), v2.color.b()).ToFloat32() * 255),
                (u8)(GetInterpolatedAttribute(v0.color.a(), v1.color.a(), v2.color.a()).ToFloat32() * 255)
            };
            Math::Vec4<u8> texture_color{};
            
            float u = GetInterpolatedAttribute(v0.tc0.u(), v1.tc0.u(), v2.tc0.u());
            float v = GetInterpolatedAttribute(v0.tc0.v(), v1.tc0.v(), v2.tc0.v());
            if (registers.texturing_enable) {
                // TODO: This is currently hardcoded for RGB8
                u32* texture_data = (u32*)Memory::GetPointer(registers.texture0.GetPhysicalAddress());
                int s = (int)(u * float24::FromFloat32(registers.texture0.width)).ToFloat32();
                int t = (int)(v * float24::FromFloat32(registers.texture0.height)).ToFloat32();
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
                const int row_stride = registers.texture0.width * 3;
                u8* source_ptr = (u8*)texture_data + coarse_s * block_height * 3 + coarse_t * row_stride + texel_index_within_tile * 3;
                texture_color.r() = source_ptr[2];
                texture_color.g() = source_ptr[1];
                texture_color.b() = source_ptr[0];
                texture_color.a() = 0xFF;
                DebugUtils::DumpTexture(registers.texture0, (u8*)texture_data);
            }
            Math::Vec4<u8> combiner_output;
            for (auto tev_stage : registers.GetTevStages()) {
                using Source = Regs::TevStageConfig::Source;
                using ColorModifier = Regs::TevStageConfig::ColorModifier;
                using AlphaModifier = Regs::TevStageConfig::AlphaModifier;
                using Operation = Regs::TevStageConfig::Operation;
                auto GetColorSource = [&](Source source) -> Math::Vec3<u8> {
                    switch (source) {
                    case Source::PrimaryColor:
                        return primary_color.rgb();
                    case Source::Texture0:
                        return texture_color.rgb();
                    case Source::Constant:
                        return{ tev_stage.const_r, tev_stage.const_g, tev_stage.const_b };
                    case Source::Previous:
                        return combiner_output.rgb();
                    default:
                        ERROR_LOG(GPU, "Unknown color combiner source %d\n", (int)source);
                        return{};
                    }
                };
                auto GetAlphaSource = [&](Source source) -> u8 {
                    switch (source) {
                    case Source::PrimaryColor:
                        return primary_color.a();
                    case Source::Texture0:
                        return texture_color.a();
                    case Source::Constant:
                        return tev_stage.const_a;
                    case Source::Previous:
                        return combiner_output.a();
                    default:
                        ERROR_LOG(GPU, "Unknown alpha combiner source %d\n", (int)source);
                        return 0;
                    }
                };
                auto GetColorModifier = [](ColorModifier factor, const Math::Vec3<u8>& values) -> Math::Vec3<u8> {
                    switch (factor)
                    {
                    case ColorModifier::SourceColor:
                        return values;
                    default:
                        ERROR_LOG(GPU, "Unknown color factor %d\n", (int)factor);
                        return{};
                    }
                };
                auto GetAlphaModifier = [](AlphaModifier factor, u8 value) -> u8 {
                    switch (factor) {
                    case AlphaModifier::SourceAlpha:
                        return value;
                    default:
                        ERROR_LOG(GPU, "Unknown color factor %d\n", (int)factor);
                        return 0;
                    }
                };
                auto ColorCombine = [](Operation op, const Math::Vec3<u8> input[3]) -> Math::Vec3<u8> {
                    switch (op) {
                    case Operation::Replace:
                        return input[0];
                    case Operation::Modulate:
                        return ((input[0] * input[1]) / 255).Cast<u8>();
                    default:
                        ERROR_LOG(GPU, "Unknown color combiner operation %d\n", (int)op);
                        return{};
                    }
                };
                auto AlphaCombine = [](Operation op, const std::array<u8, 3>& input) -> u8 {
                    switch (op) {
                    case Operation::Replace:
                        return input[0];
                    case Operation::Modulate:
                        return input[0] * input[1] / 255;
                    default:
                        ERROR_LOG(GPU, "Unknown alpha combiner operation %d\n", (int)op);
                        return 0;
                    }
                };
                // color combiner
                // NOTE: Not sure if the alpha combiner might use the color output of the previous
                // stage as input. Hence, we currently don't directly write the result to
                // combiner_output.rgb(), but instead store it in a temporary variable until
                // alpha combining has been done.
                Math::Vec3<u8> color_result[3] = {
                    GetColorModifier(tev_stage.color_modifier1, GetColorSource(tev_stage.color_source1)),
                    GetColorModifier(tev_stage.color_modifier2, GetColorSource(tev_stage.color_source2)),
                    GetColorModifier(tev_stage.color_modifier3, GetColorSource(tev_stage.color_source3))
                };
                auto color_output = ColorCombine(tev_stage.color_op, color_result);
                // alpha combiner
                std::array<u8, 3> alpha_result = {
                    GetAlphaModifier(tev_stage.alpha_modifier1, GetAlphaSource(tev_stage.alpha_source1)),
                    GetAlphaModifier(tev_stage.alpha_modifier2, GetAlphaSource(tev_stage.alpha_source2)),
                    GetAlphaModifier(tev_stage.alpha_modifier3, GetAlphaSource(tev_stage.alpha_source3))
                };
                auto alpha_output = AlphaCombine(tev_stage.alpha_op, alpha_result);
                combiner_output = Math::MakeVec(color_output, alpha_output);
            }*/
            u16 z = (u16)(((float)v0->screenpos.v[2] * w0 +
                (float)v1->screenpos.v[2] * w1 +
                (float)v2->screenpos.v[2] * w2) * 65535.f / wsum); // TODO: Shouldn't need to multiply by 65536?
            SetDepth(x >> 4, y >> 4, z);

            struct clov4 combiner_output;
            combiner_output.v[0] = 0xFF;
            combiner_output.v[1] = 0xFF;
            combiner_output.v[2] = 0xFF;
            combiner_output.v[3] = 0xFF;

            DrawPixel(x >> 4, y >> 4, &combiner_output);
        }
    }
}