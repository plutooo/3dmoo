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
    DEBUG("EnableAccelerometer (not working yet)\n");
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x120000)   //	DisableAccelerometer
{
    DEBUG("DisableAccelerometer (not working yet)\n");
    RESP(1, 0); // Result
    return 0;
}

SERVICE_CMD(0x130000)   //EnableGyroscopeLow
{
    DEBUG("EnableGyroscopeLow (not working yet)\n");
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

#define BITSET(n) (1<<(n))

enum
{
    KEY_A            = BITSET(0),
    KEY_B            = BITSET(1),
    KEY_SELECT       = BITSET(2),
    KEY_START        = BITSET(3),
    KEY_DRIGHT       = BITSET(4),
    KEY_DLEFT        = BITSET(5),
    KEY_DUP          = BITSET(6),
    KEY_DDOWN        = BITSET(7),
    KEY_R            = BITSET(8),
    KEY_L            = BITSET(9),
    KEY_X            = BITSET(10),
    KEY_Y            = BITSET(11),
    KEY_ZL           = BITSET(14), // (new 3DS only)
    KEY_ZR           = BITSET(15), // (new 3DS only)
    KEY_TOUCH        = BITSET(20), // Not actually provided by HID
    KEY_CSTICK_RIGHT = BITSET(24), // c-stick (new 3DS only)
    KEY_CSTICK_LEFT  = BITSET(25), // c-stick (new 3DS only)
    KEY_CSTICK_UP    = BITSET(26), // c-stick (new 3DS only)
    KEY_CSTICK_DOWN  = BITSET(27), // c-stick (new 3DS only)
    KEY_CPAD_RIGHT   = BITSET(28), // circle pad
    KEY_CPAD_LEFT    = BITSET(29), // circle pad
    KEY_CPAD_UP      = BITSET(30), // circle pad
    KEY_CPAD_DOWN    = BITSET(31), // circle pad

    // Generic catch-all directions
    KEY_UP    = KEY_DUP    | KEY_CPAD_UP,
    KEY_DOWN  = KEY_DDOWN  | KEY_CPAD_DOWN,
    KEY_LEFT  = KEY_DLEFT  | KEY_CPAD_LEFT,
    KEY_RIGHT = KEY_DRIGHT | KEY_CPAD_RIGHT,
};

u32 translate_to_bit(const SDL_KeyboardEvent* key)
{
    switch (SDL_GetScancodeFromKey(key->keysym.sym)) {
    case SDL_SCANCODE_V:
        return KEY_A;
    case SDL_SCANCODE_B :
        return KEY_B;
    case SDL_SCANCODE_KP_SPACE:
        return KEY_SELECT;
    case SDL_SCANCODE_KP_PLUS:
        return KEY_SELECT;
    case SDL_SCANCODE_KP_ENTER:
        return KEY_START;
    case SDL_SCANCODE_RIGHT:
        return KEY_DRIGHT;
    case SDL_SCANCODE_LEFT:
        return KEY_DLEFT;
    case SDL_SCANCODE_UP:
        return KEY_DUP;
    case SDL_SCANCODE_DOWN:
        return KEY_DDOWN;
    case SDL_SCANCODE_C:
        return KEY_R;
    case SDL_SCANCODE_N:
        return KEY_L;
    case SDL_SCANCODE_G:
        return KEY_X;
    case SDL_SCANCODE_H:
        return KEY_Y;
    case SDL_SCANCODE_F1:
    {
        mem_Dbugdump();
        return 0;
    }
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
