#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <SDL.h>
#include "util.h"

#include "filemon.h"


aufloeseentry* aufloeserfeld = 0;
u32 anzaufloeserfeld = 0;

u32 dirrealoffset = 0x84; //todo read form rom (guess at 0xC)
u32 filerealoffset = 0x1660; //todo read form rom (guess at 0x1C)
u32 dataoffset = 0x16DE0; //data offset (guess at 0x28)
u8* dirblock;
//u32 dirblocksize = 0x1000; //not sure if that is OK but I guess so

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
    anzaufloeserfeld++;
    aufloeserfeld = realloc(aufloeserfeld, anzaufloeserfeld * sizeof(aufloeseentry));
    aufloeserfeld[anzaufloeserfeld - 1].start = getle32(entry->dataoffset) + dataoffset;
    aufloeserfeld[anzaufloeserfeld - 1].size = getle32(entry->datasize);
    u8* namenfeld = malloc(getle32(entry->namesize) + 3);
    memcpy(namenfeld, entry->name, getle32(entry->namesize));
    namenfeld[getle32(entry->namesize)] = 0;
    namenfeld[getle32(entry->namesize) + 1] = 0;
    aufloeserfeld[anzaufloeserfeld - 1].name = namenfeld;
    return 1;
}
void fileana(u32 fileoffset)
{
    u32 siblingoffset = 0;
    romfs_fileentry entry;


    if (!romfs_fileblock_readentry(fileoffset, &entry))
        return;

    DEBUG("%ls\n", entry.name);
    siblingoffset = getle32(entry.siblingoffset);

    if (siblingoffset != (~0))
        fileana(siblingoffset);
}
void dirana(u32 offset) // romfs_visit_dir(romfs_context* ctx, u32 diroffset, u32 depth, u32 actions, filepath* rootpath)
{
    romfs_direntry entry;
    if (!romfs_dirblock_readentry(offset, &entry))
        return;
    DEBUG("%ls\n", entry.name);

    u32 siblingoffset = getle32(entry.siblingoffset);
    u32 childoffset = getle32(entry.childoffset);
    u32 fileoffset = getle32(entry.fileoffset);

    if (fileoffset != (~0))
        fileana(fileoffset);

    if (childoffset != (~0))
        dirana(childoffset);

    if (siblingoffset != (~0))
        dirana(siblingoffset);
}

void initfilemon(u8* level3buffer)
{
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

void filemonaufloesen(u32 addr, u32 size)
{
    for (int i = 0; i < anzaufloeserfeld; i++)
    {
        if(Contains(&aufloeserfeld[i], addr, 1))
        {
            DEBUG("%ls offset %08X size %08X\n", aufloeserfeld[i].name, addr - aufloeserfeld[i].start,size);
        }
    }
}