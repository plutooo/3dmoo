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

void IPC_readstruct(u32 addr,u8* buffer)
{
    IPC_debugprint(addr);
    u32 head = mem_Read32(addr);
    u32 translated = head & 0x3F;
    u32 normal = (head >> 6) & 0x3F;
    u32 unused = (head >> 12) & 0xF;

    mem_Read(buffer, addr, 4 * (normal + 1));
    buffer += 4 * (normal + 1);
    addr += 4 * (normal + 1);


    for (u32 i = 0; i < translated; i++)
    {
        u32 desc = mem_Read32(addr);
        if (desc & 1)
            DEBUG("secret flag set pannic\n");
        switch (desc & 0xE)
        {
        case 0x0:
            switch (desc)
            {
            case 0: //send and close KHandle (all handles are global so don't need)
                i++;
                break;
            case 0x10://send KHandle (all handles are global so don't need)
                i++;
                break;
            case 0x20://0 is OK
                i++;
                break;
            default:
                DEBUG("unknown desc %08X %08X\n", desc, mem_Read32(addr + 4));
                i++;
                break;
            }
            mem_Read(buffer, addr, 8);
            buffer += 8;
            addr += 8;
            break;
        case 0x2:
        {
            u32 size = (desc >> 14);
            u32 EXaddr = mem_Read32(addr + 4);
            i++;

            mem_Read(buffer, addr, 4);
            u32* buffera = malloc(size);
            mem_Read((u8*)buffera, EXaddr, size);

            *((u32**)buffer + 1) = (u32*)buffera;

            buffer += 4 + sizeof(u32*);
            addr += 8;

            break;
        }
        case 0x4:
            DEBUG("unsupported:PXI I/O addr = %08x (size %08X)\n", mem_Read32(addr + 4), (desc >> 4));

            mem_Read(buffer, addr, 8);
            buffer += 8;
            addr += 8;
            i++;
            break;
        case 0x6:
            DEBUG("unsupported:nothing %08X\n", desc);

            mem_Read(buffer, addr, 4);
            buffer += 4;
            addr += 4;

            break;
        case 0x8:
            DEBUG("unsupported:kernelpanic %08X\n", desc);

            mem_Read(buffer, addr, 4);
            buffer += 4;
            addr += 4;

            break;
        case 0xA:
        {
            u32 size = (desc >> 4);
            u32 EXaddr = mem_Read32(addr + 4);
            i++;

            mem_Read(buffer, addr, 4);
            u32* buffera = mem_rawaddr(EXaddr, size);

            *((u32**)buffer + 1) = (u32*)buffera;

            buffer += 4 + sizeof(u32*);
            addr += 8;
            break;
        }
        case 0xC:
        {
            u32 size = (desc >> 4);
            u32 EXaddr = mem_Read32(addr + 4);
            i++;

            mem_Read(buffer, addr, 4);
            u32* buffera = mem_rawaddr(EXaddr, size);

            *((u32**)buffer + 1) = (u32*)buffera;

            buffer += 4 + sizeof(u32*);
            addr += 8;
            break;
        }
        case 0xE:
        {
            u32 size = (desc >> 4);
            u32 EXaddr = mem_Read32(addr + 4);
            i++;

            mem_Read(buffer, addr, 4);
            u32* buffera = mem_rawaddr(EXaddr, size);

            *((u32**)buffer + 1) = (u32*)buffera;

            buffer += 4 + sizeof(u32*);
            addr += 8;
            break;
        }
        default:
            DEBUG("unknown desc %08X %08X\n", desc, mem_Read32(addr + 4));
            mem_Read(buffer, addr, 8);
            buffer += 8;
            addr += 8;
            i++;
            break;

        }
    }
}
void IPC_writestruct(u32 addr, u8* buffer)
{
    u32 originaladdr = addr;
    u32 extaddr = addr + 0x100;
    u32* buf = (u32*)buffer;
    u32 head = *buf;
    u32 translated = head & 0x3F;
    u32 normal = (head >> 6) & 0x3F;
    u32 unused = (head >> 12) & 0xF;

    mem_Write(buf, addr, 4 * (normal + 1));
    buf += (normal + 1);
    addr += 4 * (normal + 1);


    for (u32 i = 0; i < translated; i++)
    {
        u32 desc = *buf;
        if (desc & 1)
            DEBUG("secret flag set pannic\n");
        switch (desc & 0xE)
        {
        case 0x0:
            switch (desc)
            {
            case 0: //send and close KHandle (all handles are global so don't need)
                i++;
                break;
            case 0x10://send KHandle (all handles are global so don't need)
                i++;
                break;
            case 0x20://0 is OK
                i++;
                break;
            default:
                DEBUG("unknown desc %08X %08X\n", desc, mem_Read32(addr + 4));
                i++;
                break;
            }
            mem_Write(buf, addr, 8);
            buf += 2;
            addr += 8;
            break;
        case 0x2:
        {
            u32 size = (desc >> 14);
            u32 id = (desc >> 10) &0xF;
            u32** temp = (u32**)(buf + 1);
            u32* internaladdr = *temp;
            i++;

            u32 tranaddr = mem_Read32(extaddr + id * 8 + 4); //there is a way to size check find out how and implement 

            mem_Write(internaladdr, tranaddr, size);
            mem_Write32(addr, *buf);
            buf = (u8*)buf + sizeof(u32*);
            addr += 4;

            mem_Write32(addr, tranaddr);

            buf++;
            addr += 8;

            break;
        }
        case 0x4:
            DEBUG("unsupported:PXI I/O addr = %08x (size %08X)\n", mem_Read32(addr + 4), (desc >> 4));

            mem_Write((u8*)buf, addr, 8);
            buf += 2;
            addr += 8;
            i++;
            break;
        case 0x6:
            DEBUG("unsupported:nothing %08X\n", desc);

            mem_Write((u8*)buf, addr, 4);
            buf += 1;
            addr += 4;

            break;
        case 0x8:
            DEBUG("unsupported:kernelpanic %08X\n", desc);

            mem_Write((u8*)buf, addr, 4);
            buf += 1;
            addr += 4;

            break;
        case 0xA:
        {
            u32 size = (desc >> 4);
            u32 id = (desc >> 10) & 0xF;
            u32** temp = (u32**)(buf + 1);
            u32* internaladdr = *temp;
            i++;

            mem_AddMappingShared(0x70A00000, size, internaladdr);

            u32 tranaddr = 0x70A00000; //there is a way to size check find out how and implement 

            mem_Write32(addr, *buf);
            buf = (u8*)buf + sizeof(u32*);
            addr += 4;

            mem_Write32(addr, tranaddr);

            buf++;
            addr += 8;

            break;
        }
        case 0xC:
        {
            u32 size = (desc >> 4);
            u32 id = (desc >> 10) & 0xF;
            u32** temp = (u32**)(buf + 1);
            u32* internaladdr = *temp;
            i++;

            mem_AddMappingShared(0x70A00000, size, internaladdr);

            u32 tranaddr = 0x70A00000; //there is a way to size check find out how and implement 

            mem_Write32(addr, *buf);
            buf = (u8*)buf + sizeof(u32*);
            addr += 4;

            mem_Write32(addr, tranaddr);

            buf++;
            addr += 8;

            break;
        }
        case 0xE:
        {
            u32 size = (desc >> 4);
            u32 id = (desc >> 10) & 0xF;
            u32** temp = (u32**)(buf + 1);
            u32* internaladdr = *temp;
            i++;

            mem_AddMappingShared(0x70A00000, size, internaladdr);

            u32 tranaddr = 0x70A00000; //there is a way to size check find out how and implement 

            mem_Write32(addr, *buf);
            buf = (u8*)buf + sizeof(u32*);
            addr += 4;

            mem_Write32(addr, tranaddr);

            buf++;
            addr += 8;

            break;
        }
        default:
            DEBUG("unknown desc %08X %08X\n", desc, mem_Read32(addr + 4));
            mem_Write((u8*)buf, addr, 8);
            buf += 2;
            addr += 8;
            i++;
            break;

        }
    }
    IPC_debugprint(originaladdr);
    /*originaladdr += 0x100;
    for (u32 i = 0; i < 0x10; i++)
    {
        DEBUG("%08X %08X\n", mem_Read32(originaladdr), mem_Read32(originaladdr + 4));
        originaladdr += 8;
    }*/
}

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
         u32 temp = mem_Read32(addr);
         DEBUG("%08X\n", temp);
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
         {
             u32 inaddr = mem_Read32(addr + 4);
             u32 size = (desc >> 14);
             DEBUG("copy EXTENDED_CMD size %08X id %01X ptr %08X\n", size, (desc >> 10) & 0xF, inaddr);

             for (u32 i = 0; i < size; i++)
             {
                 u8 temp = mem_Read8(inaddr);
                 printf("%02X ", temp);
                 inaddr++;
             }
             printf("\n");

             addr += 4;
             i++;
             break;
         }
         case 0x4:
         {
             u32 inaddr = mem_Read32(addr + 4);
             u32 size = (desc >> 4);
             DEBUG("PXI I/O addr = %08x (size %08X)\n", inaddr, size);

             for (u32 i = 0; i < size; i++)
             {
                 u8 temp = mem_Read8(inaddr);
                 printf("%02X ", temp);
                 inaddr++;
             }
             printf("\n");

             addr += 4;
             i++;
             break;
         }
         case 0x6:
             DEBUG("nothing %08X\n", desc);
             break;
         case 0x8:
             DEBUG("kernelpanic %08X\n", desc);
             break;
         case 0xA:
         {
             u32 inaddr = mem_Read32(addr + 4);
             u32 size = (desc >> 4);

             DEBUG("Readonly (size %08X) (ptr %08X)\n", size, inaddr);

             for (u32 i = 0; i < size; i++)
             {
                 u8 temp = mem_Read8(inaddr);
                 printf("%02X ", temp);
                 inaddr++;
             }
             printf("\n");

             addr += 4;
             i++;
             break;
         }
         case 0xC:
         {
             u32 inaddr = mem_Read32(addr + 4);
             u32 size = (desc >> 4);

             DEBUG("Writeonly (size %08X) (ptr %08X)\n", size, inaddr);

             for (u32 i = 0; i < size; i++)
             {
                 u8 temp = mem_Read8(inaddr);
                 printf("%02X ", temp);
                 inaddr++;
             }
             printf("\n");
             addr += 4;
             i++;
             break;
         }
         case 0xE:
         {
             u32 inaddr = mem_Read32(addr + 4);
             u32 size = (desc >> 4);

             DEBUG("RW (size %08X) (ptr %08X)\n", size, inaddr);

             for (u32 i = 0; i < size; i++)
             {
                 u8 temp = mem_Read8(inaddr);
                 printf("%02X ", temp);
                 inaddr++;
             }
             printf("\n");
             addr += 4;
             i++;
             break;
         }
         default:
             DEBUG("unknown desc %08X %08X\n", desc, mem_Read32(addr + 4));
             addr += 4;
             i++;
             break;

         }
         addr += 4;
     }
}