#pragma once
#define ROMFS_MAXNAMESIZE	254		// limit set by ichfly

typedef struct {
    u8 parentoffset[4];
    u8 siblingoffset[4];
    u8 childoffset[4];
    u8 fileoffset[4];
    u8 weirdoffset[4]; // this one is weird. it always points to a dir entry, but seems unrelated to the romfs structure.
    u8 namesize[4];
    u8 name[ROMFS_MAXNAMESIZE];
} romfs_direntry;

typedef struct {
    u8 parentdiroffset[4];
    u8 siblingoffset[4];
    u8 dataoffset[8];
    u8 datasize[8];
    u8 weirdoffset[4]; // this one is also weird. it always points to a file entry, but seems unrelated to the romfs structure.
    u8 namesize[4];
    u8 name[ROMFS_MAXNAMESIZE];
} romfs_fileentry;

typedef struct {
    u32 start;
    u32 size;
    u8* name;
} aufloeseentry;

typedef struct {
    u8 unk1[4];
    u8 unk2[4];
    u8 unk3[4];
    u8 dirrealoffset[4];
    u8 unk5[4];
    u8 unk6[4];
    u8 unk7[4];
    u8 filerealoffset[4];
    u8 unk9[4];
    u8 dataoffset[4];
} romfs_header;

void initfilemon(u8* level3buffer);
void filemontranslate(u32 addr, u32 size);