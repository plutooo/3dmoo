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

void screen_Free()
{
    DEBUG("%s\n", __func__);
    SDL_Quit();
}

SDL_Renderer *renderer = NULL;
SDL_Texture *bitmapTex = NULL;
SDL_Surface *bitmapSurface = NULL;

void screen_Init()
{
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window *win = NULL;
    int posX = 100, posY = 100, width = 400, height = 240;

    win = SDL_CreateWindow("3dmoo", posX, posY, width, height, 0);
    if (win == NULL) {
        DEBUG("error creating window");
        return;
    }
    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        DEBUG("error creating renderer");
        return;
    }
    //bitmapSurface =  SDL_LoadBMP("img/hello.bmp");
    Uint32 rmask, gmask, bmask, amask;
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;

    bitmapSurface = SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask);
    if (bitmapSurface == NULL) {
        fprintf(stderr, "CreateRGBSurface failed: %s\n", SDL_GetError());
        return;
    }

    SDL_LockSurface(bitmapSurface);
    Uint32 *bitmapPixels = (Uint32 *)bitmapSurface->pixels;
    int i = 0;
    while (i < 0x1000) {
		bitmapPixels[i] = 0xFFFFFFFF;
        i++;
    }

    SDL_UnlockSurface(bitmapSurface);

    bitmapTex = SDL_CreateTextureFromSurface(renderer, bitmapSurface);
    if (bitmapTex == NULL) {
        DEBUG("error creation bitmaptex");
        return;
    }
    //SDL_FreeSurface(bitmapSurface);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, bitmapTex, NULL, NULL);
    SDL_RenderPresent(renderer);

	SDL_DestroyTexture(bitmapTex);

}
void cycelGPU()
{
	u32 addr = ((GPUreadreg32(frameselectoben) & 0x1) == 0) ? GPUreadreg32(RGBuponeleft) : GPUreadreg32(RGBuptwoleft);
	u8* buffer = get_pymembuffer(addr);
	if (buffer != NULL)
	{
		SDL_LockSurface(bitmapSurface);
		Uint8 *bitmapPixels = (Uint8 *)bitmapSurface->pixels;

		/*int i = 0;
		while (i < 0x1000) {
			bitmapPixels[i] = 0xFFFFFFFF;
			i++;
		}*/
		for (int y = 0; y < 240; y++)
		{
			for (int x = 0; x < 400; x++)
			{
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
