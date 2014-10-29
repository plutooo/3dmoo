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

#include "util.h"
#include "mem.h"
#include "handles.h"
#include "fs.h"
#include "polarssl/aes.h"
#include "loader.h"

/* RomFS info: this is given by loader. */
static FILE* in_fd = NULL;
static u32   romfs_off;
static u32   romfs_sz;

extern bool loader_encrypted;

/* ____ Raw RomFS ____ */

static u32 rawromfs_Read(file_type* self, u32 ptr, u32 sz, u64 off, u32* read_out)
{
    *read_out = 0;

    if((off >> 32) || (off >= romfs_sz) || ((off+sz) >= romfs_sz)) {
        ERROR("Invalid read params.\n");
        return -1;
    }

    if(fseek(in_fd, romfs_off + off, SEEK_SET) == -1) {
        ERROR("fseek failed.\n");
        return -1;
    }

    u8* b = malloc(sz);
    if(b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    u32 read = fread(b, 1, sz, in_fd);
    if (read == 0 && sz != 0) { //eshop dose this
        ERROR("fread failed\n");
        free(b);
        return -1;
    }

    ctr_aes_context ctx;

    if (loader_encrypted)
    {
        int i;
        u8* temp = calloc(sz + (off & 0xF) + (sz & 0xF), sizeof(u8));
        memcpy(temp + (off & 0xF), b, sz);
        ncch_extract_prepare(&ctx, &loader_h, NCCHTYPE_ROMFS, loader_key);
        ctr_add_counter(&ctx, (0x1000 + off) / 0x10); //this is from loader
        ctr_crypt_counter(&ctx, temp, temp, sz);
        memcpy(b, temp + (off & 0xF), sz);
        free(temp);
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

static u64 rawromfs_GetSize(file_type* self)
{
    return romfs_sz;
}

static const file_type rawromfs_file = {
    .fnRead    = &rawromfs_Read,
    .fnWrite   = NULL,
    .fnGetSize = &rawromfs_GetSize,
    .fnClose   = NULL
};


/* ____ RomFS ____ */

bool romfs_FileExists(archive* self, file_path path)
{
    DEBUG("romfs_FileExists\n");
    return false;
}

u32 romfs_OpenFile(archive* self, file_path path, u32 flags, u32 attr)
{
    DEBUG("romfs_OpenFile\n");

    // RomFS has support for raw reads.
    if(path.type == PATH_BINARY)
        return handle_New(HANDLE_TYPE_FILE, (uintptr_t) &rawromfs_file);

    // XXX: Return actual file handle here
    return 0;
}


static archive romfs = {
    .fncreateDir    = NULL,
    .fnOpenDir      = NULL,
    .fnFileExists   = &romfs_FileExists,
    .fnOpenFile     = &romfs_OpenFile,
    .fnDeinitialize = NULL,
    .fndelDir       = NULL,
    .fndelfile      = NULL
};


archive* romfs_OpenArchive(file_path path)
{
    if(path.type != 1) {
        ERROR("LowPath was not EMPTY.\n");
        return NULL;
    }

    if(in_fd == NULL) {
        ERROR("Trying to open RomFS archive, but none has been loaded.\n");
        return NULL;
    }

    return &romfs;
}

void romfs_Setup(FILE* fd, u32 off, u32 sz)
{
    // This function is called by loader if loaded file contains RomFS.
    in_fd = fd;
    romfs_off = off;
    romfs_sz  = sz;
}
