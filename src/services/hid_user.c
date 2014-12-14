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
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "service_macros.h"
#include <SDL.h>

extern ARMul_State s;

extern u8 HIDsharedbuffSPVR[0x2000];
u8 HIDsharedbuff[0x2000];

u32 memhandel;

void hid_user_init()
{
    memhandel = handle_New(HANDLE_TYPE_SHAREDMEM, MEM_TYPE_HID_0);

    HIDsharedbuff[0xb8] = 0x3;

    *(u32*)&HIDsharedbuffSPVR[0x8] = *(u32*)&HIDsharedbuff[0x8] = 1;
}

SERVICE_START(hid_user);

SERVICE_CMD(0xA0000)   //GetIPCHandles
{
    RESP(1, 0); // Result
    RESP(2, 0xDEADF00D); // Unused
    RESP(3, memhandel);
    RESP(4, handle_New(HANDLE_TYPE_UNK, 0));
    RESP(5, handle_New(HANDLE_TYPE_UNK, 0));
    RESP(6, handle_New(HANDLE_TYPE_UNK, 0));
    RESP(7, handle_New(HANDLE_TYPE_UNK, 0));
    RESP(8, handle_New(HANDLE_TYPE_UNK, 0));
    return 0;
}

SERVICE_CMD(0x110000)   //EnableAccelerometer
{
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x130000)   //EnableGyroscopeLow
{
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x150000)
{
    DEBUG("GetGyroscopeLowRawToDpsCoefficient (not working yet)\n");

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x160000)
{
    DEBUG("GetGyroscopeLowCalibrateParam (not working yet)\n");

    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x170000)
{
    DEBUG("GetSoundVolume --todo--\n");

    RESP(1, 0); // Result
    RESP(1, 0); // off?
    return 0;
}

SERVICE_END();

u32 translate_to_bit(const SDL_KeyboardEvent* key)
{
    switch (SDL_GetScancodeFromKey(key->keysym.sym)) {
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
void hid_update() //todo
{
    *(u64*)&HIDsharedbuff[0x8] = *(u64*)&HIDsharedbuff[0x0];
    *(u64*)&HIDsharedbuffSPVR[0x8] = *(u64*)&HIDsharedbuffSPVR[0x0];
    *(u64*)&HIDsharedbuff[0x0] = s.NumInstrs;
    *(u64*)&HIDsharedbuffSPVR[0x0] = s.NumInstrs;
}
void hid_updatetouch() //todo
{
    *(u64*)&HIDsharedbuff[0xB0] = *(u64*)&HIDsharedbuff[0xA8];
    *(u64*)&HIDsharedbuffSPVR[0xB0] = *(u64*)&HIDsharedbuffSPVR[0xA8];
    *(u64*)&HIDsharedbuff[0xA8] = s.NumInstrs;
    *(u64*)&HIDsharedbuffSPVR[0xA8] = s.NumInstrs;
}

void hid_position(const Sint32 x, const Sint32 y)
{
    if (x > 0 && x < 240 && y > 0 && y < 320)
    {
        hid_updatetouch();
        u32 offset = *(u32*)&HIDsharedbuff[0x10];
        if (offset == 7)offset = 0;
        else offset++;

        *(u32*)&HIDsharedbuff[0xB8] = offset;

        *(u32*)&HIDsharedbuff[0xC0] = x;
        *(u32*)&HIDsharedbuff[0xC4] = y;
        *(u32*)&HIDsharedbuffSPVR[0xC0] = *(u32*)&HIDsharedbuff[0xC0];
        *(u32*)&HIDsharedbuffSPVR[0xC4] = *(u32*)&HIDsharedbuff[0xC4];

        *(u16*)&HIDsharedbuff[0xC8 + offset * 0x8] = x; //	Current PAD state
        *(u16*)&HIDsharedbuffSPVR[0xC8 + offset * 0x8] = *(u16*)&HIDsharedbuff[0xC8 + offset * 0x8];

        *(u16*)&HIDsharedbuff[0xC8 + offset * 0x8 + 2] = y; //pressed
        *(u16*)&HIDsharedbuffSPVR[0xC8 + offset * 0x8 + 2] = *(u16*)&HIDsharedbuff[0xC8 + offset * 0x8 + 2];

        *(u32*)&HIDsharedbuff[0xC8 + offset * 0x8 + 4] |= 1; //contain data
        *(u32*)&HIDsharedbuffSPVR[0xC8 + offset * 0x8 + 4] = *(u32*)&HIDsharedbuff[0xC8 + offset * 0x8 + 4];

        *(u32*)&HIDsharedbuff[0xB8] = offset;
        *(u32*)&HIDsharedbuffSPVR[0xB8] = *(u32*)&HIDsharedbuff[0xB8];
    }
}

void hid_keyup(const SDL_KeyboardEvent* key)
{
    *(u32*)&HIDsharedbuff[0x1C] &= ~translate_to_bit(key);
    *(u32*)&HIDsharedbuffSPVR[0x1C] = *(u32*)&HIDsharedbuff[0x1C];
    u32 offset = *(u32*)&HIDsharedbuff[0x10];
    if (offset == 7)offset = 0;
    else offset++;

    *(u32*)&HIDsharedbuff[0x28 + offset * 0x10] = *(u32*)&HIDsharedbuffSPVR[0x1C]; //	Current PAD state
    *(u32*)&HIDsharedbuffSPVR[0x28 + offset * 0x10] = *(u32*)&HIDsharedbuff[0x28 + offset * 0x10];

    *(u32*)&HIDsharedbuff[0x28 + offset * 0x10 + 4] = 0; //pressed
    *(u32*)&HIDsharedbuffSPVR[0x28 + offset * 0x10 + 4] = *(u32*)&HIDsharedbuff[0x28 + offset * 0x10 + 4];

    *(u32*)&HIDsharedbuff[0x28 + offset * 0x10 + 8] = translate_to_bit(key); //released
    *(u32*)&HIDsharedbuffSPVR[0x28 + offset * 0x10 + 8] = *(u32*)&HIDsharedbuff[0x28 + offset * 0x10 + 8];

    *(u32*)&HIDsharedbuff[0x10] = offset;
    *(u32*)&HIDsharedbuffSPVR[0x10] = *(u32*)&HIDsharedbuff[0x10];

    hid_update();
}
void hid_keypress(const SDL_KeyboardEvent* key)
{
    *(u32*)&HIDsharedbuff[0x1C] |= translate_to_bit(key);
    *(u32*)&HIDsharedbuffSPVR[0x1C] = *(u32*)&HIDsharedbuff[0x1C];

    u32 offset = *(u32*)&HIDsharedbuff[0x10];
    if (offset == 7)offset = 0;
    else offset++;

    *(u32*)&HIDsharedbuff[0x28 + offset * 0x10] = *(u32*)&HIDsharedbuffSPVR[0x1C]; //	Current PAD state
    *(u32*)&HIDsharedbuffSPVR[0x28 + offset * 0x10] = *(u32*)&HIDsharedbuff[0x28 + offset * 0x10];

    *(u32*)&HIDsharedbuff[0x28 + offset * 0x10 + 4] = translate_to_bit(key); //pressed
    *(u32*)&HIDsharedbuffSPVR[0x28 + offset * 0x10 + 4] = *(u32*)&HIDsharedbuff[0x28 + offset * 0x10 + 4];

    *(u32*)&HIDsharedbuff[0x28 + offset * 0x10 + 8] = 0; //released
    *(u32*)&HIDsharedbuffSPVR[0x28 + offset * 0x10 + 8] = *(u32*)&HIDsharedbuff[0x28 + offset * 0x10 + 8];

    *(u32*)&HIDsharedbuffSPVR[0x10] = *(u32*)&HIDsharedbuff[0x10] = offset;
    hid_update();
}
