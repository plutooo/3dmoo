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

enum {
    OPEN_READ  =1,
    OPEN_WRITE =2,
    OPEN_CREATE=4
};

enum {
    ATTR_ISREADONLY=1,
    ATTR_ISARCHIVE =0x100,
    ATTR_ISHIDDEN  =0x10000,
    ATTR_ISDIR     =0x1000000
};

enum {
    PATH_INVALID,
    PATH_EMPTY,
    PATH_BINARY,
    PATH_CHAR,
    PATH_WCHAR
};

typedef struct {
    u32 type;
    u32 size;
    u32 ptr;
} file_path;

typedef struct _archive archive;
struct _archive {
    int(*fncreateDir)(archive* self, file_path path);
    int (*fndelDir)(archive* self, file_path path);
    u32(*fnOpenDir)(archive* self, file_path path);
    bool (*fnFileExists)  (archive* self, file_path path);
    u32  (*fnOpenFile)    (archive* self, file_path path, u32 flags, u32 attr);
    void (*fnDeinitialize)(archive* self);

    union {
        struct {
            u8 path[24 + 1];
        } sharedextd;

        struct {
            u8 path[32 + 1];
        } sysdata;

    } type_specific;
};

typedef struct _dir_type dir_type;
struct _dir_type {
    u32(*fnRead) (dir_type* self, u32 ptr, u32 entrycount, u32* read_out);
    file_path f_path;
    archive* self;
    u8 path[255];
    WIN32_FIND_DATAA * ffd;
    HANDLE hFind;
};

typedef struct _file_type file_type;
struct _file_type {
    u32 (*fnRead) (file_type* self, u32 ptr, u32 sz, u64 off, u32* read_out);
    u32 (*fnWrite)(file_type* self, u32 ptr, u32 sz, u64 off, u32 flush_flags, u32* written_out);
    u64 (*fnGetSize)(file_type* self);
    u64 (*fnSetSize)(file_type* self, u64 sz);
    u32 (*fnClose)(file_type* self);

    union {
        struct {
            FILE* fd;
            u64   sz;
        } sharedextd;

        struct {
            FILE* fd;
            u64   sz;
            char *path;
        } sysdata;

    } type_specific;
};



// fs/romfs.c
archive* romfs_OpenArchive(file_path path);
void romfs_Setup(FILE* fd, u32 off, u32 sz);

// fs/shared_extdata.c
archive* sharedextd_OpenArchive(file_path path);

// fs/sysdata.c
archive* sysdata_OpenArchive(file_path path);

// fs/sdmc.c
archive* sdmc_OpenArchive(file_path path);

// fs/savedata.c
archive* savedata_OpenArchive(file_path path);

// fs/extsavedata.c
archive* extsavedata_OpenArchive(file_path path);

archive* SaveDatacheck_OpenArchive(file_path path);

typedef struct {
    const char* name;
    u32 id;
    archive* (*fnOpenArchive)(file_path path);
} archive_type;

static archive_type archive_types[] =  {
    {
        "RomFS",
        3,
        romfs_OpenArchive
    },
    {
        "SaveData",
        4,
        savedata_OpenArchive
    },
    {
        "ExtData",
        6,
        extsavedata_OpenArchive
    },
    {
        "SharedExtData",
        7,
        sharedextd_OpenArchive
    },
    {
        "SysData",
        8,
        sysdata_OpenArchive
    },
    {
        "SDMC",
        9,
        sdmc_OpenArchive
    },
    {
        "SaveDatacheck",
        0x2345678A,
        SaveDatacheck_OpenArchive
    }
};

// archives/fs_util.c
archive_type* fs_GetArchiveTypeById(u32 id);
const char* fs_FlagsToString(u32 flags, char* buf_out);
const char* fs_AttrToString(u32 attr, char* buf_out);
const char* fs_PathTypeToString(u32 type);
const char* fs_PathToString(u32 type, u32 ptr, u32 size, char* buf_out, size_t size_out);
bool fs_IsSafePath(const char* p);
