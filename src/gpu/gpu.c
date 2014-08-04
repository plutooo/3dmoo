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

#define VSBeginLoadProgramData 0x2cb
#define VSLoadProgramData 0x2cc

#define VSBeginLoadSwizzleData 0x2d5
#define VSLoadSwizzleData 0x2d6
// untill 0x2DD

#define VSFloatUniformSetup 0x2c0
// untill 0x2C8

u8 VSFloatUniformSetuptembuffercurrent = 0;
u32 VSFloatUniformSetuptembuffer[4];
//s32 swizzledataaddr = 0; //todo that may be a pointer that is also in mem but unk
//s32 vs_swizzle_write_offset = 0; //todo that may be a pointer that is also in mem but unk

writeGPUID(u16 ID, u8 mask, u32 size, u32* buffer)
{
    int i;
    switch (ID)
    {
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
                    vectors[index].w = *(float*)(&VSFloatUniformSetuptembuffer[0]);
                    vectors[index].z = *(float*)(&VSFloatUniformSetuptembuffer[1]);
                    vectors[index].y = *(float*)(&VSFloatUniformSetuptembuffer[2]);
                    vectors[index].x = *(float*)(&VSFloatUniformSetuptembuffer[3]);
                }
                else {
                    f24to32(VSFloatUniformSetuptembuffer[0] >> 8, &vectors[index].w);
                    f24to32(((VSFloatUniformSetuptembuffer[0] & 0xFF) << 16) | ((VSFloatUniformSetuptembuffer[1] >> 16) & 0xFFFF), &vectors[index].z);
                    f24to32(((VSFloatUniformSetuptembuffer[1] & 0xFFFF) << 8) | ((VSFloatUniformSetuptembuffer[2] >> 24) & 0xFF), &vectors[index].y);
                    f24to32(VSFloatUniformSetuptembuffer[2] & 0xFFFFFF, &vectors[index].x);

                }

                DEBUG("Set uniform %02x to (%f %f %f %f)\n", index,
                    vectors[index].x, vectors[index].y, vectors[index].z,
                    vectors[index].w);
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
