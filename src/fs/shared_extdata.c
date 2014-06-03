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

#ifdef _WIN32
#define snprintf sprintf_s
#endif


static bool sharedextd_FileExists(archive* self, file_path path)
{
    char p[256], tmp[256];
    struct stat st;

    // Generate path on host file system
    snprintf(p, 256, "sys/shared/%s/%s",
             &self->type_specific.sharedextd.path,
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
             &self->type_specific.sharedextd.path,
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

    // XXX: Create file handle
    fclose(fd);
    return 0;
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
