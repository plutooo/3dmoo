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


static u32 exefsfile_Read(file_type* self, u32 ptr, u32 sz, u64 off, u32* read_out)
{
    FILE* fd = self->type_specific.sharedextd.fd;
    *read_out = 0;

    if(off >> 32) {
        ERROR("64-bit offset not supported.\n");
        return -1;
    }

    if(fseek64(fd, off, SEEK_SET) == -1) {
        ERROR("fseek failed.\n");
        return -1;
    }

    u8* b = malloc(sz);
    if(b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    u32 read = fread(b, 1, sz, fd);
    if(read != sz) {
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

static u64 exefsfile_GetSize(file_type* self)
{
    return self->type_specific.sharedextd.sz;
}


static u32 exefsfile_Close(file_type* self)
{
    // Close file and free yourself
    fclose(self->type_specific.sharedextd.fd);
    free(self);

    return 0;
}



/* ____ FS implementation ____ */


static u32 exefsfs_OpenFile(archive* self, file_path path, u32 flags, u32 attr)
{
    char p[256], tmp[256];

    char me[0x10];

    if (mem_Read(me, path.ptr, 0x10) != 0) {
        ERROR("Got unsafe path.\n");
        return 0;
    }

    snprintf(tmp,
        sizeof(tmp),
        "%s",
        &me[4]);


    // Generate path on host file system
    snprintf(p, 256, "title/%s/%s",
             self->type_specific.sharedextd.path,
             tmp);

    if(!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }

    FILE* fd = fopen(p, "rb");
    if (fd == NULL) {
        if (flags & OPEN_CREATE) {
            fd = fopen(p, "wb");
            if (fd == NULL) {
                ERROR("Failed to open/create sharedexd, path=%s\n", p);
                return 0;
            }
        } else {
            ERROR("Failed to open sharedexd, path=%s\n", p);
            return 0;
        }
    }
    fclose(fd);

    switch (flags& (OPEN_READ | OPEN_WRITE)) {
    case 0:
        ERROR("Error open without write and read fallback to read only, path=%s\n", p);
        fd = fopen(p, "rb");
        break;
    case OPEN_READ:
        fd = fopen(p, "rb");
        break;
    case OPEN_WRITE:
        DEBUG("--todo-- write only, path=%s\n", p);
        fd = fopen(p, "r+b");
        break;
    case OPEN_WRITE | OPEN_READ:
        fd = fopen(p, "r+b");
        break;
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
    file->fnWrite = NULL;
    file->fnRead = &exefsfile_Read;
    file->fnGetSize = &exefsfile_GetSize;
    file->fnClose = &exefsfile_Close;

    file->handle = handle_New(HANDLE_TYPE_FILE, (uintptr_t)file);
    return file->handle;
}


static void exefsfs_Deinitialize(archive* self)
{
    // Free yourself
    free(self);
}


archive* exefsfs_OpenArchive(file_path path)
{
    // Extdata needs a binary path with a 12-byte id.
    if(path.type != PATH_BINARY || path.size != 8) {
        ERROR("Unknown exefs path.\n");
        return NULL;
    }

    archive* arch = calloc(sizeof(archive), 1);

    if(arch == NULL) {
        ERROR("malloc failed.\n");
        return NULL;
    }

    // Setup function pointers
    arch->fnCreateFile   = NULL;
    arch->fnRenameFile   = NULL;
    arch->fnDeleteFile   = NULL;
    arch->fnCreateDir    = NULL;
    arch->fnDeleteDir    = NULL;
    arch->fnRenameDir    = NULL;
    arch->fnOpenDir      = NULL;
    arch->fnFileExists   = NULL;
    arch->fnOpenFile     = &exefsfs_OpenFile;
    arch->fnDeinitialize = &exefsfs_Deinitialize;
    arch->result = 0;

    u8 buf[12];

    if(mem_Read(buf, path.ptr, 8) != 0) {
        ERROR("Failed to read path.\n");
        free(arch);
        return NULL;
    }

    snprintf(arch->type_specific.sharedextd.path,
        sizeof(arch->type_specific.sharedextd.path),
        "%02x%02x%02x%02x/%02x%02x%02x%02x",
        buf[7], buf[6], buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);

    char p[256];

    // Generate path on host file system
    snprintf(p, 256, "title/%s/",
        arch->type_specific.sharedextd.path);

    DIR* dir = opendir(p);
    if (dir == NULL)
    {
        //sharedextdfile_Close(arch);
        return NULL;
    }
    closedir(dir);
    return arch;
}
