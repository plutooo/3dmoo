/*
 * Copyright (C) 2014 - plutoo
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

#include <SDL.h>

#include <stdio.h>

#include "util.h"

#include "SrvtoIO.h"

SDL_Window *win = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *bitmapTex = NULL;
SDL_Surface *bitmapSurface = NULL;

void screen_Init()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    int posX = 100, posY = 100, width = 400, height = 240;

    win = SDL_CreateWindow("3dmoo", posX, posY, width, height, 0);
    if (win == NULL) {
        DEBUG("error creating window");
        return;
    }
    bitmapSurface = SDL_GetWindowSurface(win);
}

void screen_Free()
{
    DEBUG("%s\n", __func__);

    //Destroy window
    SDL_DestroyWindow(win);
    win = NULL;

    //Quit SDL subsystems
    SDL_Quit();
}

/*
* Set the pixel at (x, y) to the given value
* NOTE: The surface must be locked before calling this!
*/
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        }
        else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;

    default:
        break;           /* shouldn't happen, but avoids warnings */
    } // switch
}

void DrawTopScreen()
{
    u32 addr = ((GPUreadreg32(frameselectoben) & 0x1) == 0) ? GPUreadreg32(RGBuponeleft) : GPUreadreg32(RGBuptwoleft);
    u8* buffer = get_pymembuffer(addr);
    if (buffer != NULL)
    {
        SDL_LockSurface(bitmapSurface);
        for (int y = 0; y < 240; y++)
        {
            for (int x = 0; x < 400; x++)
            {
                u8 b = 0;
                u8 g = 0;
                u8 r = 0;
                u32 temp = (addr + (x * 240 + y) * 3);

                r = buffer[((x * 240 + y) * 3) + 0];
                g = buffer[((x * 240 + y) * 3) + 1];
                b = buffer[((x * 240 + y) * 3) + 2];

                putpixel(bitmapSurface, x, (239 - y), SDL_MapRGB(bitmapSurface->format, r, g, b));
            }
        }
        SDL_UnlockSurface(bitmapSurface);

        //Update the surface
        SDL_UpdateWindowSurface(win);
    }
}

void cycelGPU()
{
    DrawTopScreen();
    return;

    u32 addr = ((GPUreadreg32(frameselectoben) & 0x1) == 0) ? GPUreadreg32(RGBuponeleft) : GPUreadreg32(RGBuptwoleft);
    u8* buffer = get_pymembuffer(addr);
    if (buffer != NULL) {
        SDL_LockSurface(bitmapSurface);
        Uint8 *bitmapPixels = (Uint8 *)bitmapSurface->pixels;

        /*int i = 0;
        while (i < 0x1000) {
        	bitmapPixels[i] = 0xFFFFFFFF;
        	i++;
        }*/
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < 400; x++) {
                u8* row = (u8*)(bitmapPixels + ((239 - y) * 400 * 4) + (x * 4));
                *(row + 0) = buffer[((x * 240 + y) * 3) + 0];
                *(row + 1) = buffer[((x * 240 + y) * 3) + 1];
                *(row + 2) = buffer[((x * 240 + y) * 3) + 2];
                *(row + 3) = 0xFF;
            }
        }
        SDL_UnlockSurface(bitmapSurface);
        bitmapTex = SDL_CreateTextureFromSurface(renderer, bitmapSurface);
        if (bitmapTex == NULL) {
            DEBUG("error creation bitmaptex");
            return;
        }
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bitmapTex, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}
