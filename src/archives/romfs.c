#include "util.h"
#include "mem.h"
#include "handles.h"
#include "archives.h"

/* RomFS info: this is given by loader. */
static FILE* in_fd = NULL;
static u32   romfs_off;
static u32   romfs_sz;


static u32 rawromfs_Read(u32 ptr, u32 sz, u64 off, u32* read_out) {
    if(sz > 0x10000) {
        ERROR("Too large read.\n");
        return 0;
    }

    u8 b[sz];

    // TODO: check retvals, make sure off and sz actually is RomFS
    fseek(in_fd, SEEK_SET, romfs_off + 0x1000 + off);
    fread(b, sz, 1, in_fd);

    mem_Write(b, ptr, sz);

    *read_out = sz;
    return 0; // Result
}

static u64 rawromfs_GetSize() {
    return romfs_sz - 0x1000;
}

static const file_type rawromfs_file = {
    &rawromfs_Read,
    NULL /*Write*/,
    &rawromfs_GetSize,
    NULL /*Close*/
};



bool romfs_FileExists(file_path path) {
    DEBUG("romfs_FileExists\n");
    // XXX
    return false;
}

u32 romfs_OpenFile(file_path path, u32 flags, u32 attr) {
    DEBUG("romfs_OpenFile\n");

    // RomFS has support for raw reads.
    if(path.type == PATH_BINARY)
        return handle_New(HANDLE_TYPE_FILE, (uintptr_t) &rawromfs_file);

    // XXX: return file handle
    return -1;
}

void romfs_Deinitialize() {
    DEBUG("romfs_Deinitialize\n");
}


static archive romfs = {
    &romfs_FileExists,
    &romfs_OpenFile,
    &romfs_Deinitialize
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
    in_fd = fd;
    romfs_off = off;
    romfs_sz  = sz;
}
