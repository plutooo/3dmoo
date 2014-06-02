#include "util.h"
#include "mem.h"
#include "handles.h"
#include "fs.h"

bool sharedextd_FileExists(file_path path) {
    DEBUG("sharedextd_FileExists\n");
    // XXX
    return false;
}

u32 sharedextd_OpenFile(file_path path, u32 flags, u32 attr) {
    DEBUG("sharedextd_OpenFile\n");
    // XXX: return file handle
    return -1;
}

void sharedextd_Deinitialize() {
    DEBUG("sharedextd_Deinitialize\n");
}


static archive sharedextd = {
    &sharedextd_FileExists,
    &sharedextd_OpenFile,
    &sharedextd_Deinitialize
};

archive* sharedextd_OpenArchive(file_path path) {
    return NULL;
}

void sharedextd_Setup(FILE* fd, u32 off, u32 sz) {

}
