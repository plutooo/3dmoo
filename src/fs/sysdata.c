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

#include "config.h"
#ifdef _WIN32
#include <direct.h>
#endif

/* ____ File implementation ____ */

static u32 sysdatafile_Read(file_type* self, u32 ptr, u32 sz, u64 off, u32* read_out)
{
    FILE* fd = self->type_specific.sysdata.fd;
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
static u32 sysdatafile_Write(file_type* self, u32 ptr, u32 sz, u64 off, u32 flush_flags, u32* written_out)
{
    FILE* fd = self->type_specific.sysdata.fd;
    *written_out = 0;

    if (off >> 32) {
        ERROR("64-bit offset not supported.\n");
        return -1;
    }

    if (fseek64(fd, off, SEEK_SET) == -1) {
        ERROR("fseek failed.\n");
        return -1;
    }

    u8* b = malloc(sz);
    if (b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    if (mem_Read(b, ptr, sz) != 0) {
        ERROR("mem_Write failed.\n");
        free(b);
        return -1;
    }

    u32 write = fwrite(b, 1, sz, fd);
    if (write == 0) {
        ERROR("fwrite failed\n");
        free(b);
        return -1;
    }

    *written_out = write;
    free(b);

    return 0; // Result
}


static u64 sysdatafile_GetSize(file_type* self)
{
    return self->type_specific.sysdata.sz;
}

static u32 sysdatafile_Close(file_type* self)
{
    // Close file and free yourself
    fclose(self->type_specific.sysdata.fd);
    free(self);

    return 0;
}



/* ____ FS implementation ____ */

static bool sysdata_FileExists(archive* self, file_path path)
{
    char p[256], tmp[256];
    struct stat st;

    // Generate path on host file system
    if (config_usesys) {
        snprintf(p, 256, "%s/%s/%s", config_sysdataoutpath,
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    } else {
        snprintf(p, 256, "sys/system/%s/%s",
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    }

    if(!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return false;
    }

    return stat(p, &st) == 0;
}

static u32 sysdata_DeleteFile(archive* self, file_path path)
{
    char p[256], tmp[256];

    // Generate path on host file system
    if (config_usesys) {
        snprintf(p, 256, "%s/%s/%s", config_sysdataoutpath,
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    } else {
        snprintf(p, 256, "sys/system/%s/%s",
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    }

    if (!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 1;
    }

    return remove(p);
}


static u32 sysdata_OpenFile(archive* self, file_path path, u32 flags, u32 attr)
{
    char p[256], tmp[256];

    // Generate path on host file system
    if (config_usesys) {
        snprintf(p, 256, "%s/%s/%s", config_sysdataoutpath,
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    } else {
        snprintf(p, 256, "sys/system/%s/%s",
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    }

    if(!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }

    FILE* fd = fopen(p, "rb");
    if (fd == NULL) {
        if (flags & OPEN_CREATE) {
            fd = fopen(p, "wb");
            if (fd == NULL) {
                ERROR("Failed to open/create sdmc, path=%s\n", p);
                return 0;
            }
        } else {
            ERROR("Failed to open sdmc, path=%s\n", p);
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


    if(fd == NULL) {
        ERROR("Failed to open SysData, path=%s\n", p);
        return 0;
    }

    fseek(fd, 0, SEEK_END);
    u32 sz;

    if ((sz = ftell(fd)) == -1) {
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
    file->fnWrite = &sysdatafile_Write;
    file->fnRead = &sysdatafile_Read;
    file->fnGetSize = &sysdatafile_GetSize;
    file->fnClose = &sysdatafile_Close;

    file->handle = handle_New(HANDLE_TYPE_FILE, (uintptr_t)file);
    return file->handle;
}

static void sysdata_Deinitialize(archive* self)
{
    // Free yourself
    free(self);
}

int sysdata_DeleteDir(archive* self, file_path path)
{
    char p[256], tmp[256];

    // Generate path on host file system
    if (config_usesys) {
        snprintf(p, 256, "%s/%s/%s", config_sysdataoutpath,
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    } else {
        snprintf(p, 256, "sys/system/%s/%s",
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    }

    if (!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }
#ifdef _MSC_VER 
    return _rmdir(p);
#else
    return rmdir(p, 0777);
#endif
}

int sysdata_CreateDir(archive* self, file_path path)
{
    char p[256], tmp[256];

    // Generate path on host file system
    if (config_usesys) {
        snprintf(p, 256, "%s/%s/%s", config_sysdataoutpath,
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    } else {
        snprintf(p, 256, "sys/system/%s/%s",
                 self->type_specific.sysdata.path,
                 fs_PathToString(path.type, path.ptr, path.size, tmp, 256));
    }

    if (!fs_IsSafePath(p)) {
        ERROR("Got unsafe path.\n");
        return 0;
    }
#ifdef _MSC_VER 
    return _mkdir(p);
#else
    return mkdir(p, 0777);
#endif
}

archive* sysdata_OpenArchive(file_path path)
{
    // SysData needs a binary path with an 8-byte id.
    if(path.type != PATH_BINARY || path.size != 8) {
        ERROR("Unknown SysData path.\n");
        return NULL;
    }

    archive* arch = calloc(sizeof(archive), 1);

    if(arch == NULL) {
        ERROR("malloc failed.\n");
        return NULL;
    }

    // Setup function pointers
    arch->fnRenameFile   = NULL;
    arch->fnDeleteFile   = &sysdata_DeleteFile;
    arch->fnCreateDir    = &sysdata_CreateDir;
    arch->fnDeleteDir    = &sysdata_DeleteDir;
    arch->fnRenameDir    = NULL;
    arch->fnOpenDir      = NULL;
    arch->fnFileExists   = &sysdata_FileExists;
    arch->fnOpenFile     = &sysdata_OpenFile;
    arch->fnDeinitialize = &sysdata_Deinitialize;
    arch->result = 0;

    u8 buf[8];

    if(mem_Read(buf, path.ptr, 8) != 0) {
        ERROR("Failed to read path.\n");
        free(arch);
        return NULL;
    }

    snprintf(arch->type_specific.sysdata.path,
             sizeof(arch->type_specific.sysdata.path),
             "%02x%02x%02x%02x%02x%02x%02x%02x",
             buf[7], buf[6], buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);

    printf("%s\n", arch->type_specific.sysdata.path);
    return arch;
}
