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

#include "util.h"
#include "gpu.h"
#include "hid_user.h"
#include "color.h"
#include <SDL.h>
#include "arm11.h"


SDL_Window *win = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *bitmapTex = NULL;
SDL_Surface *bitmapSurface = NULL;

void screen_Init()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    int posX = 100, posY = 100, width = 400, height = 240*2;

    win = SDL_CreateWindow("3dmoo", posX, posY, width, height, 0);
    if (win == NULL) {
        DEBUG("Error creating SDL window\n");
        exit(1);
    }

    bitmapSurface = SDL_GetWindowSurface(win);
}

void screen_Free()
{
    // Destroy window
    SDL_DestroyWindow(win);
    win = NULL;

    // Quit SDL subsystems
    SDL_Quit();
}

void screen_RenderGPUaddr(u32 addr)
{
    int updateSurface = 0;

    //Top Screen
    u8* buffer = get_pymembuffer(addr);

    if (buffer != NULL) {
        SDL_LockSurface(bitmapSurface);

        u8 *bitmapPixels = (u8 *)bitmapSurface->pixels;

        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < 400; x++) {
                u8* row = (u8*)(bitmapPixels + ((239 - y) * 400 * 4) + (x * 4));
                *(row + 0) = buffer[((x * 240 + y) * 4) + 0];
                *(row + 1) = buffer[((x * 240 + y) * 4) + 1];
                *(row + 2) = buffer[((x * 240 + y) * 4) + 2];
                *(row + 3) = 0xFF;
            }
        }

        updateSurface = 1;
    }
    SDL_UnlockSurface(bitmapSurface);
    SDL_UpdateWindowSurface(win);
}

void screen_RenderFramebuffer(u8 *bitmapPixels, u8* buffer, u32 format, u32 width, u32 xofs)
{
    //DEBUG("format=%d\n", format & 7);
    Color color;

    switch(format & 7)
    {
        case 0: //RGBA8
        {
            for(u32 y = 0; y < 240; y++) {
                for (u32 x = 0; x < width; x++) {
                    u8* row = (u8*)(bitmapPixels + ((239 - y) * 400 * 4) + ((x + xofs) * 4));

                    color_decode(&buffer[((x * 240 + y) * 4)], RGBA8, &color);

                    u32 val = SDL_MapRGBA(bitmapSurface->format, color.r, color.g, color.b, color.a);
                    *(u32*)row = val;
                }
            }
            break;
        }
        case 1: //BGR8
        {
            for (u32 y = 0; y < 240; y++) {
                for (u32 x = 0; x < width; x++) {
                    u8* row = (u8*)(bitmapPixels + ((239 - y) * 400 * 4) + ((x + xofs) * 4));

                    color_decode(&buffer[((x * 240 + y) * 3)], RGB8, &color);

                    u32 val = SDL_MapRGBA(bitmapSurface->format, color.r, color.g, color.b, color.a);
                    *(u32*)row = val;
                }
            }
            break;
        }

        case 2: //RGB565
        {
            for (u32 y = 0; y < 240; y++) {
                for (u32 x = 0; x < width; x++) {
                    u8* row = (u8*)(bitmapPixels + ((239 - y) * 400 * 4) + ((x + xofs) * 4));

                    color_decode(&buffer[((x * 240 + y) * 2)], RGB565, &color);

                    u32 val = SDL_MapRGBA(bitmapSurface->format, color.r, color.g, color.b, color.a);
                    *(u32*)row = val;
                }
            }
            break;
        }
        case 3: //RGB5A1 - TODO
        {
            for (u32 y = 0; y < 240; y++) {
                for (u32 x = 0; x < width; x++) {
                    u8* row = (u8*)(bitmapPixels + ((239 - y) * 400 * 4) + ((x + xofs) * 4));

                    color_decode(&buffer[((x * 240 + y) * 2)], RGBA5551, &color);

                    u32 val = SDL_MapRGBA(bitmapSurface->format, color.r, color.g, color.b, color.a);
                    *(u32*)row = val;
                }
            }
            break;
        }
        case 4: //RGBA4
        {
            for (u32 y = 0; y < 240; y++) {
                for (u32 x = 0; x < width; x++) {
                    u8* row = (u8*)(bitmapPixels + ((239 - y) * 400 * 4) + ((x + xofs) * 4));

                    color_decode(&buffer[((x * 240 + y) * 2)], RGBA5551, &color);

                    u32 val = SDL_MapRGBA(bitmapSurface->format, color.r, color.g, color.b, color.a);
                    *(u32*)row = val;
                }
            }
            break;
        }
        default:
            ERROR("Unknown screen format %08X", format & 7);
            break;
    }
}

void screen_RenderGPU()
{
    u32 addr = 0;
    u8* buffer = 0;
    int updateSurface = 0;

    u32 topScreenFormat = gpu_ReadReg32(frameformattop);
    u32 bottomScreenFormat = gpu_ReadReg32(frameformatbot);

    //Top Screen
    u32 lcdColorFillMain = gpu_ReadReg32(LCDCOLORFILLMAIN);
    if (lcdColorFillMain & 1 << 24) { //Enabled
        u8 r = (lcdColorFillMain >> 0) & 0xFF;
        u8 g = (lcdColorFillMain >> 8) & 0xFF;
        u8 b = (lcdColorFillMain >> 16) & 0xFF;

        SDL_Rect rect;
        rect.x = 0;
        rect.y = 0;
        rect.w = 400;
        rect.h = 240;
        SDL_FillRect(bitmapSurface, &rect, SDL_MapRGB(bitmapSurface->format, r, g, b));
    } else {
        u32 addr = ((gpu_ReadReg32(frameselecttop) & 0x1) == 0) ? gpu_ReadReg32(RGBuponeleft) : gpu_ReadReg32(RGBuptwoleft);

        u8* buffer = get_pymembuffer(addr);

        if (buffer != NULL) {
            SDL_LockSurface(bitmapSurface);

            u8 *bitmapPixels = (u8 *)bitmapSurface->pixels;

            screen_RenderFramebuffer(bitmapPixels, buffer, topScreenFormat, 400, 0);

            updateSurface = 1;
        }
    }

    //Bottom Screen
    //addr = ((gpu_ReadReg32(frameselectunten) & 0x1) == 0) ? gpu_ReadReg32(RGBdownoneleft) : gpu_ReadReg32(RGBdowntwoleft);
    u32 lcdColorFillSub = gpu_ReadReg32(LCDCOLORFILLSUB);
    if (lcdColorFillSub & 1 << 24) { //Enabled
        u8 r = (lcdColorFillSub >> 0) & 0xFF;
        u8 g = (lcdColorFillSub >> 8) & 0xFF;
        u8 b = (lcdColorFillSub >> 16) & 0xFF;

        SDL_Rect rect;
        rect.x = 40;
        rect.y = 240;
        rect.w = 320;
        rect.h = 240;
        SDL_FillRect(bitmapSurface, &rect, SDL_MapRGB(bitmapSurface->format, r, g, b));
    } else {
        u32 addr = ((gpu_ReadReg32(frameselectbot) & 0x1) == 0) ? gpu_ReadReg32(RGBdownoneleft) : gpu_ReadReg32(RGBdowntwoleft);
        buffer = get_pymembuffer(addr);
        if (buffer != NULL) {
            if (!updateSurface) {
                SDL_LockSurface(bitmapSurface);
            }

            u8 *bitmapPixels = (u8 *)bitmapSurface->pixels +(240 * 400 * 4);

            screen_RenderFramebuffer(bitmapPixels, buffer, bottomScreenFormat, 320, 40);

            updateSurface = 1;
        }
    }


    if (updateSurface) {
        SDL_UnlockSurface(bitmapSurface);
        SDL_UpdateWindowSurface(win);
    }
}
bool mausedown = false;
void screen_HandleEvent()
{
    // Event handler
    SDL_Event e;

    while (SDL_PollEvent(&e) != 0) {
        switch (e.type) {
        case SDL_KEYUP:
            hid_keyup(&e.key);
            break;
        case SDL_KEYDOWN:
            hid_keypress(&e.key);
            break;
        case SDL_QUIT:
            exit(0);
            break;
        case SDL_MOUSEMOTION: /**< Mouse moved */
            if (mausedown)
                hid_position(e.motion.x - 40, e.motion.y - 240);
            break;
        case SDL_MOUSEBUTTONDOWN:/**< Mouse button pressed */
            mausedown = true;
            hid_position(e.motion.x - 40, e.motion.y - 240);
            break;
        case SDL_MOUSEBUTTONUP:          /**< Mouse button released */
            mausedown = false;
            break;
        default:
            break;
        }
    }
}
u32 svcsleep()
{
    //DEBUG("sleep %08x %08x\n", arm11_R(1),arm11_R(0) );
    return 0;
}
