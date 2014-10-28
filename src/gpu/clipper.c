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
#include "vec4.h"

/*
struct ClippingEdge {
public:
    enum Type {
        POS_X = 0,
        NEG_X = 1,
        POS_Y = 2,
        NEG_Y = 3,
        POS_Z = 4,
        NEG_Z = 5,
    };
    ClippingEdge(Type type, float24 position) : type(type), pos(position) {}
    bool IsInside(const OutputVertex& vertex) const {
        switch (type) {
        case POS_X: return vertex.pos.x <= pos * vertex.pos.w;
        case NEG_X: return vertex.pos.x >= pos * vertex.pos.w;
        case POS_Y: return vertex.pos.y <= pos * vertex.pos.w;
        case NEG_Y: return vertex.pos.y >= pos * vertex.pos.w;
            // TODO: Check z compares ... should be 0..1 instead?
        case POS_Z: return vertex.pos.z <= pos * vertex.pos.w;
        default:
        case NEG_Z: return vertex.pos.z >= pos * vertex.pos.w;
        }
    }
    bool IsOutSide(const OutputVertex& vertex) const {
        return !IsInside(vertex);
    }
    OutputVertex GetIntersection(const OutputVertex& v0, const OutputVertex& v1) const {
        auto dotpr = [this](const OutputVertex& vtx) {
            switch (type) {
            case POS_X: return vtx.pos.x - vtx.pos.w;
            case NEG_X: return -vtx.pos.x - vtx.pos.w;
            case POS_Y: return vtx.pos.y - vtx.pos.w;
            case NEG_Y: return -vtx.pos.y - vtx.pos.w;
                // TODO: Verify z clipping
            case POS_Z: return vtx.pos.z - vtx.pos.w;
            default:
            case NEG_Z: return -vtx.pos.w;
            }
        };
        float24 dp = dotpr(v0);
        float24 dp_prev = dotpr(v1);
        float24 factor = dp_prev / (dp_prev - dp);
        return OutputVertex::Lerp(factor, v0, v1);
    }
private:
    Type type;
    float24 pos;
};*/

void InitScreenCoordinates(struct OutputVertex *vtx)
{
    struct {
        float halfsize_x;
        float offset_x;
        float halfsize_y;
        float offset_y;
        float zscale;
        float offset_z;
    } viewport;
    f24to32(GPUregs[VIEWPORT_WIDTH], &viewport.halfsize_x);
    f24to32(GPUregs[VIEWPORT_HEIGHT], &viewport.halfsize_y);
    viewport.offset_x = GPUregs[GLViewport]&0xFFFF;
    viewport.offset_y = (GPUregs[GLViewport]>>16) & 0xFFFF;
    f24to32(GPUregs[Viewport_depth_range], &viewport.zscale);
    f24to32(GPUregs[Viewport_depth_far_plane], &viewport.offset_z);


    // TODO: Not sure why the viewport width needs to be divided by 2 but the viewport height does not
    vtx->screenpos.v[0] = (vtx->pos.v[0] / vtx->pos.v[3] + 1.0) * viewport.halfsize_x / 2.0 + viewport.offset_x;
    vtx->screenpos.v[1] = (vtx->pos.v[1] / vtx->pos.v[3] + 1.0) * viewport.halfsize_y + viewport.offset_y;
    vtx->screenpos.v[2] = viewport.offset_z - vtx->pos.v[2] / vtx->pos.v[3] * viewport.zscale;
}

#define max_vertices 1000


u32 buffer_vertices_num;
struct OutputVertex buffer_vertices[max_vertices];

u32 output_list_num;
struct OutputVertex output_list[max_vertices];

u32 input_list_num;
struct OutputVertex input_list[max_vertices];

void Lerp(float factor, const struct OutputVertex *vtx, struct OutputVertex *change) {
    change->pos.v[0] = change->pos.v[0] * factor + vtx->pos.v[0] * ((1.0) - factor);
    change->pos.v[1] = change->pos.v[1] * factor + vtx->pos.v[1] * ((1.0) - factor);
    change->pos.v[2] = change->pos.v[2] * factor + vtx->pos.v[2] * ((1.0) - factor);
    change->pos.v[3] = change->pos.v[3] * factor + vtx->pos.v[3] * ((1.0) - factor);

    // TODO: Should perform perspective correct interpolation here...
    change->tc0.v[0] = change->tc0.v[0] * factor + vtx->tc0.v[0] * ((1.0) - factor);
    change->tc0.v[1] = change->tc0.v[1] * factor + vtx->tc0.v[1] * ((1.0) - factor);

    change->screenpos.v[0] = change->screenpos.v[0] * factor + vtx->screenpos.v[0] * ((1.0) - factor);
    change->screenpos.v[1] = change->screenpos.v[1] * factor + vtx->screenpos.v[1] * ((1.0) - factor);
    change->screenpos.v[2] = change->screenpos.v[2] * factor + vtx->screenpos.v[2] * ((1.0) - factor);

    change->color.v[0] = change->color.v[0] * factor + vtx->color.v[0] * ((1.0) - factor);
    change->color.v[1] = change->color.v[1] * factor + vtx->color.v[1] * ((1.0) - factor);
    change->color.v[2] = change->color.v[2] * factor + vtx->color.v[2] * ((1.0) - factor);
    change->color.v[3] = change->color.v[3] * factor + vtx->color.v[3] * ((1.0) - factor);
}

void Clipper_ProcessTriangle(struct OutputVertex *v0, struct OutputVertex *v1, struct OutputVertex *v2) {
    // todo make this save and remove buffer_vertices
    memcpy(&output_list[0], v0, sizeof(struct OutputVertex));
    memcpy(&output_list[1], v1, sizeof(struct OutputVertex));
    memcpy(&output_list[2], v2, sizeof(struct OutputVertex));

    buffer_vertices_num = 0;

    output_list_num = 3;

    for (int i = 0; i < 3; i++)
    {

        memcpy(input_list, output_list, sizeof(struct OutputVertex)*output_list_num);
        input_list_num = output_list_num;

        output_list_num = 0;

        struct OutputVertex* reference_vertex = &input_list[input_list_num - 1];

        for (u32 j = 0; j < input_list_num; j++)
        {
            if (input_list[j].pos.v[i] <= +1.0 * input_list[j].pos.v[3]) //is inside
            {
                if (!(reference_vertex->pos.v[i] <= +1.0 * reference_vertex->pos.v[3]))
                {
                    memcpy(&buffer_vertices[buffer_vertices_num], &input_list[j], sizeof(struct OutputVertex));
                    
                    //GetIntersection

                    float dp = input_list[j].pos.v[i] - input_list[j].pos.v[3];
                    float dp_prev = reference_vertex->pos.v[i] - reference_vertex->pos.v[3];
                    float factor = dp_prev / (dp_prev - dp);
                    Lerp(factor, reference_vertex, &buffer_vertices[buffer_vertices_num]);
                    buffer_vertices_num++;


                    memcpy(&output_list[output_list_num++], &buffer_vertices[buffer_vertices_num - 1], sizeof(struct OutputVertex));
                }
                memcpy(&output_list[output_list_num++], &input_list[j], sizeof(struct OutputVertex));


            }
            else if (reference_vertex->pos.v[i] <= +1.0 * reference_vertex->pos.v[3]) //is inside
            {
                memcpy(&buffer_vertices[buffer_vertices_num], &input_list[j], sizeof(struct OutputVertex));

                //GetIntersection

                float dp = input_list[j].pos.v[i] - input_list[j].pos.v[3];
                float dp_prev = reference_vertex->pos.v[i] - reference_vertex->pos.v[3];
                float factor = dp_prev / (dp_prev - dp);
                Lerp(factor, reference_vertex, &buffer_vertices[buffer_vertices_num]);
                buffer_vertices_num++;

                memcpy(&output_list[output_list_num++], &buffer_vertices[buffer_vertices_num - 1], sizeof(struct OutputVertex));
            }
            reference_vertex = &input_list[j];

        }



        //do it again only for neg this time

        memcpy(input_list, output_list, sizeof(struct OutputVertex)*output_list_num);
        input_list_num = output_list_num;

        output_list_num = 0;

        reference_vertex = &input_list[input_list_num - 1];

        for (u32 j = 0; j < input_list_num; j++)
        {
            if (input_list[j].pos.v[i] >= -1.0 * input_list[j].pos.v[3]) //is inside
            {
                if (!(reference_vertex->pos.v[i] >= -1.0 * reference_vertex->pos.v[3]))
                {
                    memcpy(&buffer_vertices[buffer_vertices_num], &input_list[j], sizeof(struct OutputVertex));

                    //GetIntersection

                    float dp = -input_list[j].pos.v[i] - input_list[j].pos.v[3];
                    float dp_prev = reference_vertex->pos.v[i] - reference_vertex->pos.v[3];
                    float factor = dp_prev / (dp_prev - dp);
                    Lerp(factor, reference_vertex, &buffer_vertices[buffer_vertices_num]);
                    buffer_vertices_num++;


                    memcpy(&output_list[output_list_num++], &buffer_vertices[buffer_vertices_num - 1], sizeof(struct OutputVertex));
                }
                memcpy(&output_list[output_list_num++], &input_list[j], sizeof(struct OutputVertex));


            }
            else if (reference_vertex->pos.v[i] >= -1.0 * reference_vertex->pos.v[3]) //is inside
            {
                memcpy(&buffer_vertices[buffer_vertices_num], &input_list[j], sizeof(struct OutputVertex));

                //GetIntersection

                float dp = -input_list[j].pos.v[i] - input_list[j].pos.v[3];
                float dp_prev = reference_vertex->pos.v[i] - reference_vertex->pos.v[3];
                float factor = dp_prev / (dp_prev - dp);
                Lerp(factor, reference_vertex, &buffer_vertices[buffer_vertices_num]);
                buffer_vertices_num++;

                memcpy(&output_list[output_list_num++], &buffer_vertices[buffer_vertices_num - 1], sizeof(struct OutputVertex));
            }
            reference_vertex = &input_list[j];

        }

    }
    int i = 0;
    InitScreenCoordinates(&output_list[0]);
    InitScreenCoordinates(&output_list[1]);

    if (output_list_num > 2)
    {
        for (u32 i = 0; i < output_list_num - 2; i++) {
            struct OutputVertex* vtx0 = &output_list[0];
            struct OutputVertex* vtx1 = &output_list[i + 1];
            struct OutputVertex* vtx2 = &output_list[i + 2];
            InitScreenCoordinates(vtx2);
            GPUDEBUG(
                "Triangle %d/%d (%d buffer vertices) at position (%.3f, %.3f, %.3f, %.3f), "
                "(%.3f, %.3f, %.3f, %.3f), (%.3f, %.3f, %.3f, %.3f) and "
                "screen position (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f), (%.2f, %.2f, %.2f)\n",
                i, output_list_num, buffer_vertices_num,
                vtx0->pos.v[0], vtx0->pos.v[1], vtx0->pos.v[2], vtx0->pos.v[3],
                vtx1->pos.v[0], vtx1->pos.v[1], vtx1->pos.v[2], vtx1->pos.v[3],
                vtx2->pos.v[0], vtx2->pos.v[1], vtx2->pos.v[2], vtx2->pos.v[3],
                vtx0->screenpos.v[0], vtx0->screenpos.v[1], vtx0->screenpos.v[2],
                vtx1->screenpos.v[0], vtx1->screenpos.v[1], vtx1->screenpos.v[2],
                vtx2->screenpos.v[0], vtx2->screenpos.v[1], vtx2->screenpos.v[2]);
            rasterizer_ProcessTriangle(vtx0, vtx1, vtx2);
        }
    }
}