/*
* Copyright (C) 2014 - plutoo
* Copyright (C) 2014 - ichfly
* Copyright (C) 2014 - Normmatt
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "mem.h"
#include "handles.h"
#include "fs.h"


/* ____ Save Data implementation ____ */

static u32 savedatafile_Read(file_type* self, u32 ptr, u32 sz, u64 off, u32* read_out)
{
    FILE* fd = self->type_specific.sysdata.fd;
    *read_out = 0;

    if(off >> 32) {
        ERROR("64-bit offset not supported.\n");
        return -1;
    }

    if(fseek(fd, off, SEEK_SET) == -1) {
        ERROR("fseek failed.\n");
        return -1;
    }

    u8* b = malloc(sz);
    if(b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    u32 read = fread(b, 1, sz, fd);
    if(read == 0) {
        ERROR("fread failed\n");
        free(b);
        return -1;
    }

    if(mem_Write(b, ptr, read) != 0) {
        ERROR("mem_Write failed.\n");
        free(b);
        return -1;
    }

    *read_out = read;
    free(b);

    return 0; // Result
}

static u32 savedatafile_Write(file_type* self, u32 ptr, u32 sz, u64 off, u32 flush_flags, u32* written_out)
{
    FILE* fd = self->type_specific.sysdata.fd;
    *written_out = 0;

    if (off >> 32) {
        ERROR("64-bit offset not supported.\n");
        return -1;
    }

    if (fseek(fd, off, SEEK_SET) == -1) {
        ERROR("fseek failed.\n");
        return -1;
    }

    u8* b = malloc(sz);
    if (b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    if (mem_Read(b, ptr, sz) != 0) {
        ERROR("mem_Read failed.\n");
        free(b);
        return -1;
    }

    u32 written = fwrite(b, 1, sz, fd);
    if (written == 0) {
        ERROR("fwrite failed\n");
        free(b);
        return -1;
    }

    *written_out = written;
    free(b);

    return 0; // Result
}

static u64 savedatafile_GetSize(file_type* self)
{
    return self->type_specific.sysdata.sz;
}

static u64 savedatafile_SetSize(file_type* self, u64 sz)
{
    FILE* fd = self->type_specific.sysdata.fd;
    u64 current_size = self->type_specific.sysdata.sz;

    if (sz >= current_size)
    {
        if (fseek(fd, sz, SEEK_SET) == -1) {
            ERROR("fseek failed.\n");
            return -1;
        }
    }
    else
    {
        DEBUG("Truncating a file is unsupported.\n");
    }

    return 0;
}

static u32 savedatafile_Close(file_type* self)
{
    // Close file and free yourself
    fclose(self->type_specific.sysdata.fd);
    free(self);

    return 0;
}



/* ____ FS implementation ____ */

static bool savedata_FileExists(archive* self, file_path path)
{
    char p[256], tmp[256];
    struct stat st;

    // Generate path on host file system
    snprintf(p, 256, "savedata/%s",
             fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if(!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return false;
    }

    return stat(p, &st) == 0;
}

static u32 savedata_OpenFile(archive* self, file_path path, u32 flags, u32 attr)
{
    char p[256], tmp[256];

    // Generate path on host file system
    snprintf(p, 256, "savedata/%s",
             fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if(!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }

    char mode[10];
    memset(mode, 0, 10);

    switch (flags)
    {
        case 1: //R
            strcpy(mode, "rb");
            break;
        case 2: //W
        case 3: //RW
            strcpy(mode, "rb+");
            break;
        case 4: //C
        case 6: //W+C
            strcpy(mode, "wb");
            break;
    }

    FILE* fd = fopen(p, mode);

    if(fd == NULL) {
        ERROR("Failed to open SaveData, path=%s\n", p);
        return 0;
    }

    fseek(fd, 0, SEEK_END);
    u32 sz;

    if((sz=ftell(fd)) == -1) {
        ERROR("ftell() failed.\n");
        fclose(fd);
        return 0;
    }

    // Create file object
    file_type* file = calloc(sizeof(file_type), 1);

    if(file == NULL) {
        ERROR("calloc() failed.\n");
        fclose(fd);
        return 0;
    }

    file->type_specific.sysdata.fd = fd;
    file->type_specific.sysdata.sz = (u64) sz;

    // Setup function pointers.
    file->fnRead = &savedatafile_Read;
    file->fnWrite = &savedatafile_Write;
    file->fnGetSize = &savedatafile_GetSize;
    file->fnSetSize = &savedatafile_SetSize;
    file->fnClose = &savedatafile_Close;

    file->handle = handle_New(HANDLE_TYPE_FILE, (uintptr_t)file);
    return file->handle;
}


int savedata_CreateDir(archive* self, file_path path)
{
    char p[256], tmp[256];

    // Generate path on host file system
    snprintf(p, 256, "savedata/%s",
        fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if (!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }
    return mkdir(p, 0777);
}


static void savedata_Deinitialize(archive* self)
{
    // Free yourself
    free(self);
}

archive* savedata_OpenArchive(file_path path)
{
    archive* arch = calloc(sizeof(archive), 1);

    if (arch == NULL) {
        ERROR("malloc failed.\n");
        return NULL;
    }

    // Setup function pointers
    arch->fnCreateDir = &savedata_CreateDir;
    arch->fnOpenDir = NULL;
    arch->fnFileExists = &savedata_FileExists;
    arch->fnOpenFile = &savedata_OpenFile;
    arch->fnDeinitialize = &savedata_Deinitialize;

    snprintf(arch->type_specific.sysdata.path,
        sizeof(arch->type_specific.sysdata.path),
        "");
    return arch;
}
