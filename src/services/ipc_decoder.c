/*
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
#include "handles.h"
#include "mem.h"
#include "arm11.h"

#include "service_macros.h"

void IPC_debugprint(u32 addr)
{
     u32 head = mem_Read32(addr);
     u32 translated = head & 0x3F;
     u32 normal = (head >> 6) & 0x3F;
     u32 unused = (head >> 12) & 0xF;
     DEBUG("cmd %08X\n", head);
     addr += 4;
     for (u32 i = 0; i < normal; i++)
     {
         DEBUG("%08X\n", mem_Read32(addr));
         addr += 4;
     }
     for (u32 i = 0; i < translated; i++)
     {
         u32 desc = mem_Read32(addr);
         if (desc & 1)
             DEBUG("secret flag set\n");
         switch (desc & 0xE)
         {
         case 0x0:
             switch (desc)
             {
             case 0:
                 DEBUG("send and close KHandle %08X\n", mem_Read32(addr + 4));
                 addr += 4;
                 i++;
                 break;
             case 0x10:
                 DEBUG("send KHandle duplicate %08X\n", mem_Read32(addr + 4));
                 addr += 4;
                 i++;
                 break;
             case 0x20:
                 DEBUG("send ProcessID\n");
                 addr += 4;
                 i++;
                 break;
             default:
                 DEBUG("unknown desc %08X %08X\n", desc, mem_Read32(addr + 4));
                 addr += 4;
                 i++;
                 break;
             }
             break;
         case 0x2:
             DEBUG("copy EXTENDED_CMD size %08X id %01X ptr %08X\n", (desc >> 14), (desc >> 10)&0xF, mem_Read32(addr + 4));
             addr += 4;
             i++;
             break;
         case 0x4:
             DEBUG("PXI I/O addr = %08x (size %08X)\n",mem_Read32(addr + 4), (desc >> 4));
             addr += 4;
             i++;
             break;
         case 0x6:
             DEBUG("nothing %08X\n", desc);
             break;
         case 0x8:
             DEBUG("kernelpanic %08X\n", desc);
             break;
         case 0xA:
             DEBUG("Readonly (size %08X) (ptr %08X)\n", (desc >> 4), mem_Read32(addr + 4));
             addr += 4;
             i++;
             break;
         case 0xC:
             DEBUG("Writeonly (size %08X) (ptr %08X)\n", (desc >> 4), mem_Read32(addr + 4));
             addr += 4;
             i++;
             break;
         case 0xE:
             DEBUG("RW (size %08X) (ptr %08X)\n", (desc >> 4), mem_Read32(addr + 4));
             addr += 4;
             i++;
             break;
         default:
             DEBUG("unknown desc %08X %08X\n", desc, mem_Read32(addr + 4));
             addr += 4;
             i++;
             break;

         }
         addr += 4;
     }
}