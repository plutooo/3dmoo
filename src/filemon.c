#include "util.h"
#include "filemon.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <wchar.h>

#include <SDL.h>


aufloeseentry* translaterfild = 0;
u32 translaterfildcount = 0;

u32 dirrealoffset;
u32 filerealoffset;
u32 dataoffset;
u8* dirblock;

u8* foldername;
u32 foldernamelen = 2;

u32 getle32(const u8* p)
{
    return (p[0] << 0) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

int romfs_dirblock_read(u32 diroffset, u32 dirsize, void* buffer)
{
    memcpy(buffer, dirblock + diroffset + dirrealoffset, dirsize);
    return 1;
}
int romfs_fileblock_read(u32 fileoffset, u32 filesize, void* buffer)
{
    memcpy(buffer, dirblock + fileoffset + filerealoffset, filesize);
    return 1;
}
int romfs_dirblock_readentry( u32 diroffset, romfs_direntry* entry)
{
    u32 size_without_name = sizeof(romfs_direntry)-ROMFS_MAXNAMESIZE;
    u32 namesize;


    if (!romfs_dirblock_read(diroffset, size_without_name, entry))
        return 0;

    namesize = getle32(entry->namesize);
    if (namesize > (ROMFS_MAXNAMESIZE - 2))
        namesize = (ROMFS_MAXNAMESIZE - 2);
    memset(entry->name + namesize, 0, 2);
    if (!romfs_dirblock_read(diroffset + size_without_name, namesize, entry->name))
        return 0;

    return 1;
}

int romfs_fileblock_readentry(u32 fileoffset, romfs_fileentry* entry)
{
    u32 size_without_name = sizeof(romfs_fileentry)-ROMFS_MAXNAMESIZE;
    u32 namesize;

    if (!romfs_fileblock_read(fileoffset, size_without_name, entry))
        return 0;

    namesize = getle32(entry->namesize);
    if (namesize > (ROMFS_MAXNAMESIZE - 2))
        namesize = (ROMFS_MAXNAMESIZE - 2);
    memset(entry->name + namesize, 0, 2);
    if (!romfs_fileblock_read(fileoffset + size_without_name, namesize, entry->name))
        return 0;

    memcpy(foldername + foldernamelen, entry->name, namesize);
    foldername[foldernamelen + namesize] = 0;
    foldername[foldernamelen + namesize + 1] = 0;

    wprintf(L"%ls\n", (wchar_t*)foldername);

    translaterfildcount++;
    translaterfild = realloc(translaterfild, translaterfildcount * sizeof(aufloeseentry));
    translaterfild[translaterfildcount - 1].start = getle32(entry->dataoffset) + dataoffset;
    translaterfild[translaterfildcount - 1].size = getle32(entry->datasize);
    u8* namenfeld = malloc(foldernamelen + namesize + 5);
    memcpy(namenfeld, foldername, foldernamelen + namesize + 4);
    translaterfild[translaterfildcount - 1].name = namenfeld;
    return 1;
}

void fileana(u32 fileoffset)
{
    u32 siblingoffset = 0;
    romfs_fileentry entry;

    if (!romfs_fileblock_readentry(fileoffset, &entry))
        return;

    siblingoffset = getle32(entry.siblingoffset);

    if (siblingoffset != (~0))
        fileana(siblingoffset);
}

void dirana(u32 offset) // romfs_visit_dir(romfs_context* ctx, u32 diroffset, u32 depth, u32 actions, filepath* rootpath)
{
    u32 foldernamelentemp = foldernamelen;
    romfs_direntry entry;
    u32 namesize;
    if (!romfs_dirblock_readentry(offset, &entry))
        return;
    namesize = getle32(entry.namesize);

    memcpy(foldername + foldernamelen, entry.name, namesize);
    foldernamelen += namesize;

    foldername[foldernamelen] = '/';
    foldername[foldernamelen + 1] = 0;
    foldername[foldernamelen + 2] = 0;
    foldername[foldernamelen + 3] = 0;

    foldernamelen += 2;
    wprintf(L"%ls\n", (wchar_t*)foldername);

    u32 siblingoffset = getle32(entry.siblingoffset);
    u32 childoffset = getle32(entry.childoffset);
    u32 fileoffset = getle32(entry.fileoffset);

    if (fileoffset != (~0))
        fileana(fileoffset);

    if (childoffset != (~0))
        dirana(childoffset);
    foldernamelen = foldernamelentemp;

    if (siblingoffset != (~0))
        dirana(siblingoffset);
}

void initfilemon(u8* level3buffer)
{
    foldername = (u8*)malloc(0x1000); //this is enough
    foldername[0] = '/';
    foldername[1] = 0;
    foldername[2] = 0;
    foldername[3] = 0;

    romfs_header fs_header;
    memcpy(&fs_header, level3buffer, sizeof(fs_header));

    dirrealoffset = getle32(fs_header.dirrealoffset);
    filerealoffset = getle32(fs_header.filerealoffset);
    dataoffset = getle32(fs_header.dataoffset);

    dirblock = level3buffer;
    dirana(0);
}

static int Contains(aufloeseentry* m, uint32_t addr, uint32_t sz)
{
    return (m->start <= addr && (addr + sz) <= (m->start + m->size));
}

void filemontranslate(u32 addr, u32 size)
{
    for (unsigned int i = 0; i < translaterfildcount; i++) {
        if (Contains(&translaterfild[i], addr, 1)) {
            DEBUG("%ls offset %08X size %08X\n", (wchar_t*)translaterfild[i].name, addr - translaterfild[i].start, size);
        }
    }
}
