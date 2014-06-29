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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"
#include "mem.h"
#include "handles.h"
#include "fs.h"


/* ____ File implementation ____ */

static u32 sharedextdfile_Read(file_type* self, u32 ptr, u32 sz, u64 off, u32* read_out)
{
    FILE* fd = self->type_specific.sharedextd.fd;
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

static u64 sharedextdfile_GetSize(file_type* self)
{
    return self->type_specific.sharedextd.sz;
}

static u32 sharedextdfile_Close(file_type* self)
{
    // Close file and free yourself
    fclose(self->type_specific.sharedextd.fd);
    free(self);

    return 0;
}



/* ____ FS implementation ____ */

static bool sharedextd_FileExists(archive* self, file_path path)
{
    char p[256], tmp[256];
    struct stat st;

    // Generate path on host file system
    snprintf(p, 256, "sys/shared/%s/%s",
             self->type_specific.sharedextd.path,
             fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if(!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return false;
    }

    return stat(p, &st) == 0;
}

static u32 sharedextd_OpenFile(archive* self, file_path path, u32 flags, u32 attr)
{
    char p[256], tmp[256];

    // Generate path on host file system
    snprintf(p, 256, "sys/shared/%s/%s",
             self->type_specific.sharedextd.path,
             fs_PathToString(path.type, path.ptr, path.size, tmp, 256));

    if(!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }

    if(flags != OPEN_READ) {
        ERROR("Trying to write/create in SharedExtData\n");
        return 0;
    }

    FILE* fd = fopen(p, "rb");

    if(fd == NULL) {
        ERROR("Failed to open SharedExtData, path=%s\n", p);
        return 0;
    }

    fseek(fd, 0, SEEK_END);
    u32 sz = 0;

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

    file->type_specific.sharedextd.fd = fd;
    file->type_specific.sharedextd.sz = (u64) sz;

    // Setup function pointers.
    file->fnRead = &sharedextdfile_Read;
    file->fnGetSize = &sharedextdfile_GetSize;
    file->fnClose = &sharedextdfile_Close;

    return handle_New(HANDLE_TYPE_FILE, (uintptr_t) file);
}

static void sharedextd_Deinitialize(archive* self)
{
    // Free yourself
    free(self);
}


archive* sharedextd_OpenArchive(file_path path)
{
    // Extdata needs a binary path with a 12-byte id.
    if(path.type != PATH_BINARY || path.size != 12) {
        ERROR("Unknown SharedExtData path.\n");
        return NULL;
    }

    archive* arch = calloc(sizeof(archive), 1);

    if(arch == NULL) {
        ERROR("malloc failed.\n");
        return NULL;
    }

    // Setup function pointers
    arch->fnFileExists   = &sharedextd_FileExists;
    arch->fnOpenFile     = &sharedextd_OpenFile;
    arch->fnDeinitialize = &sharedextd_Deinitialize;

    u8 buf[12];

    if(mem_Read(buf, path.ptr, 12) != 0) {
        ERROR("Failed to read path.\n");
        free(arch);
        return NULL;
    }

    snprintf(arch->type_specific.sharedextd.path,
             sizeof(arch->type_specific.sharedextd.path),
             "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
             buf[0], buf[1], buf[2], buf[3], buf[ 4], buf[ 5],
             buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
    return arch;
}
