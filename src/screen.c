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

void screen_Free()
{
    DEBUG("%s\n", __func__);
    SDL_Quit();
}

void screen_Init()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(400, 480, 16, SDL_SWSURFACE);
    SDL_WM_SetCaption("3dmoo", "3dmoo");
}
