#include "util.h"
#include "mem.h"
#include "handles.h"
#include "fs.h"

/* RomFS info: this is given by loader. */
static FILE* in_fd = NULL;
static u32   romfs_off;
static u32   romfs_sz;


/* ____ Raw RomFS ____ */

static u32 rawromfs_Read(file_type* self, u32 ptr, u32 sz, u64 off, u32* read_out)
{
    if((off >> 32) || (off >= romfs_sz) || ((off+sz) >= romfs_sz)) {
        ERROR("Invalid read params.\n");
        return -1;
    }

    if(fseek(in_fd, romfs_off + 0x1000 + off, SEEK_SET) == -1) {
        ERROR("fseek failed.\n");
        return -1;
    }

    u8* b = malloc(sz);
    if(b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    u32 read = fread(b, 1, sz, in_fd);
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

static u64 rawromfs_GetSize(file_type* self) {
    return romfs_sz - 0x1000;
}

static const file_type rawromfs_file = {
    .fnRead    = &rawromfs_Read,
    .fnWrite   = NULL,
    .fnGetSize = &rawromfs_GetSize,
    .fnClose   = NULL
};


/* ____ RomFS ____ */

bool romfs_FileExists(archive* self, file_path path) {
    DEBUG("romfs_FileExists\n");
    return false;
}

u32 romfs_OpenFile(archive* self, file_path path, u32 flags, u32 attr) {
    DEBUG("romfs_OpenFile\n");

    // RomFS has support for raw reads.
    if(path.type == PATH_BINARY)
        return handle_New(HANDLE_TYPE_FILE, (uintptr_t) &rawromfs_file);

    // XXX: Return actual file handle here
    return -1;
}


static archive romfs = {
    .fnFileExists   = &romfs_FileExists,
    .fnOpenFile     = &romfs_OpenFile,
    .fnDeinitialize = NULL
};


archive* romfs_OpenArchive(file_path path) {
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

void romfs_Setup(FILE* fd, u32 off, u32 sz) {
    // This function is called by loader if loaded file contains RomFS.
    in_fd = fd;
    romfs_off = off;
    romfs_sz  = sz;
}
