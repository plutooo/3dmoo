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

FILE* filesevhand[0x10];
bool fileisfree[0x10];
#define filesevhandnumb 0x10

char* filearchhandst[0x10];
u64 filearchhand[0x10];
bool filearchisfree[0x10];
#define filearchhandnumb 0x10

void fsinit()
{
    for (int i = 0; i < filesevhandnumb; i++)
    {
        fileisfree[i] = true;
    }
    for (int i = 0; i < filearchhandnumb; i++)
    {
        filearchisfree[i] = true;
        filearchhand[i] = i + 0xDEADAFFEB00B;
    }
}
s32 findfree()
{
    s32 q = -1;
    for (int i = 0; i < filesevhandnumb; i++)
    {
        if (fileisfree[i] == true)q = i;
    }
    return q;
}
s32 findfreearch()
{
    s32 q = -1;
    for (int i = 0; i < filearchhandnumb; i++)
    {
        if (filearchisfree[i] == true)q = i;
    }
    return q;
}
s32 findarch(u32 low ,u32 high)
{
    s32 q = -1;
    for (int i = 0; i < filearchhandnumb; i++)
    {
        if ((filearchhand[i] & 0xFFFFFFFF) == low && ((filearchhand[i] >> 32) & 0xFFFFFFFF) == high)q = i;
    }
    return q;
}

void getendfix(u32 numb, char* str)
{
    char temp[0x200];
    switch (numb)
    {
    case 0x7: //Shared ExtSaveData
        strcpy(temp, str);
        sprintf(str, "sex/%s", temp);
        return;
    default:
        strcpy(temp, str);
        sprintf(str, "junko/%s", temp);
        return;
    }
}

u32 fs_user_SyncRequest()
{
    char raw[0x200];
    char cstring[0x200];
    u32 cid = mem_Read32(CPUsvcbuffer + 0x80);

    // Read command-id.
    switch (cid)
    {
    case 0x08030204:
    {
                       u32 aidconde = mem_Read32(CPUsvcbuffer + 0x88);
                       u32 atype = mem_Read32(CPUsvcbuffer + 0x8C);
                       u32 asize = mem_Read32(CPUsvcbuffer + 0x90);
                       u32 ftyp = mem_Read32(CPUsvcbuffer + 0x94);
                       u32 fsize = mem_Read32(CPUsvcbuffer + 0x98);
                       u32 oflags = mem_Read32(CPUsvcbuffer + 0x9C);
                       u32 attr = mem_Read32(CPUsvcbuffer + 0xA0);
                       u32 adata = mem_Read32(CPUsvcbuffer + 0xA8);
                       u32 fdata = mem_Read32(CPUsvcbuffer + 0xB0);

                       break;
    }
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
                       case PATH_CHAR:
                           mem_Read(cstring, data, size);
                           break;
                       case PATH_WCHAR:
                       {
                                          int i = 0;
                                          while (i < size)
                                          {
                                              cstring[i] = (char)mem_Read16(data + i * 2);
                                              i++;
                                          }
                                          cstring[i] = 0;
                                          break;
                       }
                       default:
                           mem_Read(raw, data, size);
                           DEBUG("unsupported type");
                           return 0;
                       }
                       char fulladdr[0x500];
                       DEBUG("fs:USER:OpenFile(%s);\n", cstring);
                       s32 p = findarch(handleLow, handleHigh);
                       sprintf(fulladdr, "%s%s", filearchhandst[p], cstring);
                       FILE *fileh = fopen(fulladdr, "r");
                       if (fileh == 0)
                       {
                           mem_Write32(CPUsvcbuffer + 0x8C, 0); //return handle
                           mem_Write32(CPUsvcbuffer + 0x84, 0xFFFFFFFF); //error
                           return 0xFFFFFFFF;
                       }
                       s32 j = findfree();
                       filesevhand[j] = fileh;
                       fileisfree[j] = false;
                       u32 handel = handle_New(HANDLE_TYPE_FILE, j);
                       mem_Write32(CPUsvcbuffer + 0x8C, handel); //return handle
                       mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
                       return 0;
    }
        
        case 0x080C00C2:
        {
            u32 archiveID = mem_Read32(CPUsvcbuffer + 0x84);
            FS_pathType type = (FS_pathType)mem_Read32(CPUsvcbuffer + 0x88);
            u32 size = mem_Read32(CPUsvcbuffer + 0x8C);
            u32 data = mem_Read32(CPUsvcbuffer + 0x94);
            if (size > 0x100)
            {
                DEBUG("to big");
                return 0;
            }

            switch (type)
            {
                case PATH_CHAR:
                    mem_Read(cstring, data, size);
                    break;
                case PATH_WCHAR:
                {
                    mem_Read(raw, data, size);
                    int i = 0;
                    while (i < size)
                    {
                        cstring[i] = (char)mem_Read16(data + i * 2);
                        i++;
                    }
                    cstring[i] = 0;
                    break;
                }
                case PATH_BINARY:
                {
                                    s32 p = findfreearch();
                                    filearchhandst[p] = malloc(9 + size * 2);
                                    filearchhandst[p][0] = 0;
                                    for (int k = 0; k < size; k++)sprintf(filearchhandst[p], "%s%02X", filearchhandst[p], (char)mem_Read8(data + k));
                                    getendfix(archiveID, filearchhandst[p]);
                                    sprintf(filearchhandst[p], "%s/",filearchhandst[p]);
                                    filearchisfree[p] = false;
                                    mem_Write32(CPUsvcbuffer + 0x8C, (filearchhand[p] & 0xFFFFFFFF));
                                    mem_Write32(CPUsvcbuffer + 0x88, ((filearchhand[p] >> 32) & 0xFFFFFFFF));
                                    break;
                                    
                }
                default:
                    mem_Read(raw, data, size);
                    DEBUG("unsupported type");
                    return 0;
            }
            DEBUG("fs:USER:OpenArchive(%s);\n", cstring);

            mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
            return 0;
        }
    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}
u32 file_SyncRequest(handleinfo* h, bool *locked)
{
    *locked = false;
}