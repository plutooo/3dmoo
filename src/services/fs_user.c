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
        case 0x3: //Application RomFS
            strcpy(temp, str);
            sprintf(str, "romfs/%s", temp);
            break;
        case 0x7: //Shared ExtSaveData
            strcpy(temp, str);
            sprintf(str, "sex/%s", temp);
            break;
        default:
            strcpy(temp, str);
            sprintf(str, "junko/%s", temp);
            break;
    }

    if (strlen(temp) == 0)
    {
        str[strlen(str)-1] = 0;
    }
}

int DecodePath(FS_pathType type, u32 data, u32 size, char *out)
{
    memset(out, 0, size + 1);

    switch (type)
    {
    case PATH_CHAR:
        mem_Read(out, data, size);
        return 1;
    case PATH_WCHAR:
    {
        int i = 0;
        while (i < size)
        {
            out[i] = (char)mem_Read16(data + i * 2);
            i++;
        }
        out[i] = 0;
        return 1;
    }
    case PATH_BINARY:
    {
        int i = 0;
        //out[0] = '/';
        while (i < size)
        {
            u8 dat = mem_Read8(data + i);
            sprintf(out, "%s%02X", out, dat);
            i++;
        }
        return 1;
    }
    case PATH_EMPTY:
    {
        sprintf(out, "");
        break;
    }
    default:
        //mem_Read(out, data, size);
        DEBUG("unsupported type");

        int i = 0;
        while (i < size)
        {
            u8 dat = mem_Read8(data + i);
            sprintf(&out[i * 2], "%02X", dat);
            i++;
        }
        out[i] = 0;

        return 0;
    }
}

u32 fs_user_SyncRequest()
{
    char raw[0x200];
    char cstring[0x200];
    cstring[0] = 0;
    u32 cid = mem_Read32(CPUsvcbuffer + 0x80);

    // Read command-id.
    switch (cid)
    {
        case 0x08030204: //FS:OpenFileDirectly
        {
            char cstringz[0x200];
            u32 archiveID = mem_Read32(CPUsvcbuffer + 0x88);
            u32 atype = mem_Read32(CPUsvcbuffer + 0x8C);
            u32 asize = mem_Read32(CPUsvcbuffer + 0x90);
            u32 ftype = mem_Read32(CPUsvcbuffer + 0x94);
            u32 fsize = mem_Read32(CPUsvcbuffer + 0x98);
            u32 oflags = mem_Read32(CPUsvcbuffer + 0x9C);
            u32 attr = mem_Read32(CPUsvcbuffer + 0xA0);
            u32 adata = mem_Read32(CPUsvcbuffer + 0xA8);
            u32 fdata = mem_Read32(CPUsvcbuffer + 0xB0);

            if (asize > 0x100)
            {
                DEBUG("to big");
                return 0;
            }

            DecodePath(atype,adata,asize,cstring);
            getendfix(archiveID, cstring);
            sprintf(cstring, "%s/", cstring);


            if (fsize > 0x100)
            {
                DEBUG("to big");
                return 0;
            }

            DecodePath(ftype, fdata, fsize, cstringz);

            char fulladdr[0x500];
            sprintf(cstring, "%s%s", cstring, cstringz);
            DEBUG("fs:USER:OpenFileDirect(%s);\n", cstring);
            arm11_Dump();
            FILE *fileh = fopen(cstring, "r");
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


            break;
        }
        case 0x080201C2: //FS:OpenFile
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

            DecodePath(type, data, size, cstring);

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

        case 0x080C00C2: //FS:OpenArchive
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

            DecodePath(type, data, size, cstring);

            s32 p = findfreearch();
            getendfix(archiveID, cstring);
            sprintf(cstring, "%s/", cstring);
            filearchhandst[p] = malloc(0x200);
            filearchisfree[p] = false;
            strcpy_s(filearchhandst[p], 0x200, cstring);
            mem_Write32(CPUsvcbuffer + 0x8C, (filearchhand[p] & 0xFFFFFFFF));
            mem_Write32(CPUsvcbuffer + 0x88, ((filearchhand[p] >> 32) & 0xFFFFFFFF));
            mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
            DEBUG("fs:USER:OpenArchive(%s);\n", cstring);
            return 0;
        }
        case 0x080E0080: //FS:CloseArchive
        {
            u32 handleHigh = mem_Read32(CPUsvcbuffer + 0x84);
            u32 handleLow = mem_Read32(CPUsvcbuffer + 0x88);

            s32 p = findarch(handleLow, handleHigh);

            DEBUG("fs:USER:CloseArchive(%s);\n", filearchhandst[p]);

            filearchhandst[p] = NULL;
            filearchisfree[p] = true;

            //TODO: Remove handle?

            mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
            return 0;
        }
        default:
            break;
    }

    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}
u32 file_SyncRequest(handleinfo* h, bool *locked)
{
    *locked = false;
    u32 cid = mem_Read32(CPUsvcbuffer + 0x80);
    switch (cid)
    {
    case 0x080200C2:
    {
                       u32 offseto = mem_Read32(CPUsvcbuffer + 0x84);
                       u32 offsett = mem_Read32(CPUsvcbuffer + 0x88);
                       u32 size = mem_Read32(CPUsvcbuffer + 0x8C);
                       u32 pointer = mem_Read32(CPUsvcbuffer + 0x94);
                       DEBUG("read");
                       char* data = malloc(size);
                       fseek(filesevhand[h->subtype], offseto + offsett << 32, SEEK_SET);
                       u32 temp = fread(data, 1, size, filesevhand[h->subtype]);
                       mem_Write32(CPUsvcbuffer + 0x88, temp); //no error
                       mem_Write(data, pointer, temp);
                       mem_Write32(CPUsvcbuffer + 0x84, 0); //no error
                       free(data);
                       break;
    }
    default:
        break;
    }
    ERROR("NOT IMPLEMENTED, cid=%08x\n", cid);
    arm11_Dump();
    PAUSE();
    return 0;
}
file_CloseHandle(handleinfo* h)
{
    fclose(filesevhand[h->subtype]);
    return 0;
}