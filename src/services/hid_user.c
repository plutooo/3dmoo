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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"

u8 HIDsharedbuff[0x2000];

#define CPUsvcbuffer 0xFFFF0000

u32 memhandel;

u32 hid_user_init()
{
    memhandel = handle_New(HANDLE_TYPE_SHAREDMEM, MEM_TYPE_HID_0);
}

u32 hid_user_SyncRequest()
{
    u32 cid = mem_Read32(0xFFFF0080);
    switch (cid)
    {
    case 0x000A0000:
        mem_Write32(CPUsvcbuffer + 0x8C, memhandel);
        mem_Write32(CPUsvcbuffer + 0x90, handle_New(HANDLE_TYPE_UNK, 0));
        mem_Write32(CPUsvcbuffer + 0x94, handle_New(HANDLE_TYPE_UNK, 0));
        mem_Write32(CPUsvcbuffer + 0x98, handle_New(HANDLE_TYPE_UNK, 0));
        mem_Write32(CPUsvcbuffer + 0x9C, handle_New(HANDLE_TYPE_UNK, 0));
        mem_Write32(CPUsvcbuffer + 0x100, handle_New(HANDLE_TYPE_UNK, 0));

        mem_Write32(CPUsvcbuffer + 0x84, 0); //worked
        return 0;
    case 0x00110000: //EnableAccelerometer
        mem_Write32(CPUsvcbuffer + 0x84, 0); //worked
        return 0;
    case 0x00130000: //EnableGyroscopeLow
        mem_Write32(CPUsvcbuffer + 0x84, 0); //worked
        return 0;
    default:
        break;
    }
    ERROR("STUBBED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();

    return 0;
}
u32 translate_to_bit(SDL_KeyboardEvent key)
{
    switch (SDL_GetScancodeFromKey(key.keysym.sym)) {
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
void hid_keyup(SDL_KeyboardEvent key)
{
    *(u32*)&HIDsharedbuff[0x1C] &= ~translate_to_bit(key);
}
void hid_keypress(SDL_KeyboardEvent key)
{
    *(u32*)&HIDsharedbuff[0x1C] |= translate_to_bit(key);
}
