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

#include "util.h"
#include "arm11.h"
#include "handles.h"
#include "mem.h"
#include "SrvtoIO.h"


#define CPUsvcbuffer 0xFFFF0000


u32 fs_user_SyncRequest()
{
    char cstring[0x200];
    u32 cid = mem_Read32(CPUsvcbuffer + 0x80);

    // Read command-id.
    switch (cid) {
    case 0x080201C2:
    {
                       u32 handleHigh = mem_Read32(CPUsvcbuffer + 0x88);
                       u32 handleLow = mem_Read32(CPUsvcbuffer + 0x8C);
                       u32 type = mem_Read32(CPUsvcbuffer + 0x90);
                       u32 size = mem_Read32(CPUsvcbuffer + 0x94);
                       u32 openflags = mem_Read32(CPUsvcbuffer + 0x98);
                       u32 attributes = mem_Read32(CPUsvcbuffer + 0x9C);
                       u32 data = mem_Read32(CPUsvcbuffer + 0xA4);
                       if (size > 0x100)
                       {
                           DEBUG("to big");
                           return 0;
                       }
                       switch (type)
                       {
                       case PATH_WCHAR:
                       {
                                          int i = 0;
                                          while (i < size)
                                          {
                                              cstring[i] = (char)mem_Read16(data + i * 2);
                                              i++;
                                          }
                                          break;
                       }
                       default:
                           DEBUG("unsupprtet type");

                           return 0;
                       }
                       switch (handleHigh)
                       {
                       case 2://Gamecard???
                       {
                                 u32 handel = handle_New(HANDLE_TYPE_FILE, 0);
                                 mem_Write32(CPUsvcbuffer + 0x8C, handel); //return handle
                       }
                       default:
                           break;
                       }
                       mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
                       return 0;
    }
    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}