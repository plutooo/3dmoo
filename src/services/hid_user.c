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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sdl.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"

u8 HIDsharedbuff[0x2000];

u32 hid_user_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);
    ERROR("STUBBED, cid=%08x\n", cid);
    PAUSE();

    return 0;
}
void hid_keyup(SDL_KeyboardEvent key)
{
	*(u32*)&HIDsharedbuff[0x1C] &= ~translate_to_bit(key);
}
void hid_keypress(SDL_KeyboardEvent key)
{
	*(u32*)&HIDsharedbuff[0x1C] |= translate_to_bit(key);
}
u32 translate_to_bit(SDL_KeyboardEvent key)
{
	switch (SDL_GetScancodeFromKey(key.keysym.sym))
	{
	case SDL_SCANCODE_V:
		return 1 <<0;
	case SDL_SCANCODE_B :
		return 1 << 1;
	case SDL_SCANCODE_KP_SPACE:
		return 1 << 2;
	case SDL_SCANCODE_KP_ENTER:
		return 1 << 3;
	case SDL_SCANCODE_RIGHT:
		return 1 << 4;
	case SDL_SCANCODE_LEFT:
		return 1 << 5;
	case SDL_SCANCODE_UP:
		return 1 << 6;
	case SDL_SCANCODE_DOWN:
		return 1 << 7;
	case SDL_SCANCODE_C:
		return 1 << 8;
	case SDL_SCANCODE_N:
		return 1 << 9;
	case SDL_SCANCODE_G:
		return 1 << 10;
	case SDL_SCANCODE_H:
		return 1 << 11;
	default:
		return 0;
	}
	
}