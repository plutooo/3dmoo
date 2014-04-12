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
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "arm11.h"
#include "mem.h"

// Shamelessly stolen from ctrtool.
typedef struct {
    u8 signature[0x100];
    u8 magic[4];
    u8 contentsize[4];
    u8 partitionid[8];
    u8 makercode[2];
    u8 version[2];
    u8 reserved0[4];
    u8 programid[8];
    u8 tempflag;
    u8 reserved1[0x2f];
    u8 productcode[0x10];
    u8 extendedheaderhash[0x20];
    u8 extendedheadersize[4];
    u8 reserved2[4];
    u8 flags[8];
    u8 plainregionoffset[4];
    u8 plainregionsize[4];
    u8 reserved3[8];
    u8 exefsoffset[4];
    u8 exefssize[4];
    u8 exefshashregionsize[4];
    u8 reserved4[4];
    u8 romfsoffset[4];
    u8 romfssize[4];
    u8 romfshashregionsize[4];
    u8 reserved5[4];
    u8 exefssuperblockhash[0x20];
    u8 romfssuperblockhash[0x20];
} ctr_ncchheader;

// ExeFS
typedef struct {
    u8 name[8];
    u8 offset[4];
    u8 size[4];
} exefs_sectionheader;

typedef struct {
    exefs_sectionheader section[8];
    u8 reserved[0x80];
    u8 hashes[8][0x20];
} exefs_header;

// Exheader
typedef struct {
    u8 reserved[5];
    u8 flag;
    u8 remasterversion[2];
} exheader_systeminfoflags;

typedef struct {
    u8 address[4];
    u8 nummaxpages[4];
    u8 codesize[4];
} exheader_codesegmentinfo;

typedef struct {
    u8 name[8];
    exheader_systeminfoflags flags;
    exheader_codesegmentinfo text;
    u8 stacksize[4];
    exheader_codesegmentinfo ro;
    u8 reserved[4];
    exheader_codesegmentinfo data;
    u8 bsssize[4];
} exheader_codesetinfo;

typedef struct {
    u8 programid[0x30][8];
} exheader_dependencylist;

typedef struct {
    u8 savedatasize[4];
    u8 reserved[4];
    u8 jumpid[8];
    u8 reserved2[0x30];
} exheader_systeminfo;

typedef struct {
    u8 extsavedataid[8];
    u8 systemsavedataid[8];
    u8 reserved[8];
    u8 accessinfo[7];
    u8 otherattributes;
} exheader_storageinfo;

typedef struct {
    u8 programid[8];
    u8 flags[8];
    u8 resourcelimitdescriptor[0x10][2];
    exheader_storageinfo storageinfo;
    u8 serviceaccesscontrol[0x20][8];
    u8 reserved[0x1f];
    u8 resourcelimitcategory;
} exheader_arm11systemlocalcaps;

typedef struct {
    u8 descriptors[28][4];
    u8 reserved[0x10];
} exheader_arm11kernelcapabilities;

typedef struct {
    u8 descriptors[15];
    u8 descversion;
} exheader_arm9accesscontrol;

typedef struct {
    // systemcontrol info {
    //   coreinfo {
    exheader_codesetinfo codesetinfo;
    exheader_dependencylist deplist;
    //   }
    exheader_systeminfo systeminfo;
    // }
    // accesscontrolinfo {
    exheader_arm11systemlocalcaps arm11systemlocalcaps;
    exheader_arm11kernelcapabilities arm11kernelcaps;
    exheader_arm9accesscontrol arm9accesscontrol;
    // }
    struct {
        u8 signature[0x100];
        u8 ncchpubkeymodulus[0x100];
        exheader_arm11systemlocalcaps arm11systemlocalcaps;
        exheader_arm11kernelcapabilities arm11kernelcaps;
        exheader_arm9accesscontrol arm9accesscontrol;
    } accessdesc;
} exheader_header;

typedef struct {
    u32 hdr_sz;
    u16 type;
    u16 version;
    u32 cert_sz;
    u32 tik_sz;
    u32 tmd_sz;
    u32 meta_sz;
    u64 content_sz;
    u8 content_idx[0x2000];
} cia_header;

#define CIA_MAGIC sizeof(cia_header)


static u32 Read32(uint8_t p[4])
{
    u32 temp = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
    return temp;
}

static u32 AlignPage(u32 in)
{
    return ((in + 0xFFF) / 0x1000) * 0x1000;
}

u32 GetDecompressedSize(u8* compressed, u32 compressedsize)
{
    u8* footer = compressed + compressedsize - 8;

    u32 originalbottom = Read32(footer+4);
    return originalbottom + compressedsize;
}

int Decompress(u8* compressed, u32 compressedsize, u8* decompressed, u32 decompressedsize)
{
    u8* footer = compressed + compressedsize - 8;
    u32 buffertopandbottom = Read32(footer+0);
    u32 i, j;
    u32 out = decompressedsize;
    u32 index = compressedsize - ((buffertopandbottom>>24)&0xFF);
    u32 segmentoffset;
    u32 segmentsize;
    u8 control;
    u32 stopindex = compressedsize - (buffertopandbottom&0xFFFFFF);

    memset(decompressed, 0, decompressedsize);
    memcpy(decompressed, compressed, compressedsize);

    while(index > stopindex) {
        control = compressed[--index];

        for(i=0; i<8; i++) {
            if(index <= stopindex)
                break;
            if(index <= 0)
                break;
            if(out <= 0)
                break;

            if(control & 0x80) {
                if(index < 2) {
                    fprintf(stderr, "Error, compression out of bounds\n");
                    goto clean;
                }

                index -= 2;

                segmentoffset = compressed[index] | (compressed[index+1]<<8);
                segmentsize = ((segmentoffset >> 12)&15)+3;
                segmentoffset &= 0x0FFF;
                segmentoffset += 2;

                if(out < segmentsize) {
                    fprintf(stderr, "Error, compression out of bounds\n");
                    goto clean;
                }

                for(j=0; j<segmentsize; j++) {
                    u8 data;

                    if(out+segmentoffset >= decompressedsize) {
                        fprintf(stderr, "Error, compression out of bounds\n");
                        goto clean;
                    }

                    data  = decompressed[out+segmentoffset];
                    decompressed[--out] = data;
                }
            } else {
                if(out < 1) {
                    fprintf(stderr, "Error, compression out of bounds\n");
                    goto clean;
                }
                decompressed[--out] = compressed[--index];
            }
            control <<= 1;
        }
    }

    return 1;
clean:
    return 0;
}


static u32 load_elf_image(u8 *addr)
{
    u32 *header = (u32*) addr;
    u32 *phdr = (u32*) (addr + Read32((u8*) &header[7]));
    u32 n = Read32((u8*)&header[11]) &0xFFFF;
    u32 i;

    for (i = 0; i < n; i++, phdr += 8) {
        if (phdr[0] != 1) // PT_LOAD
            continue;

        u32 off = Read32((u8*) &phdr[1]);
        u32 dest = Read32((u8*) &phdr[3]);
        u32 filesz = Read32((u8*) &phdr[4]);
        u32 memsz = Read32((u8*) &phdr[5]);

        u8* data = malloc(memsz);
        memset(data, 0, memsz);
        memcpy(data, addr + off, filesz);
        mem_AddSegment(dest, memsz, data);

    }

    return Read32((u8*) &header[6]);
}


int loader_LoadFile(FILE* fd)
{
    u32 ncch_off = 0;

    // Read header.
    ctr_ncchheader h;
    if (fread(&h, sizeof(h), 1, fd) != 1) {
        ERROR("failed to read header.\n");
        return 1;
    }

    if (Read32(h.signature) == 0x464c457f) { // Load ELF
        // Add stack segment.
        mem_AddSegment(0x10000000 - 0x100000, 0x100000, NULL);

        // Add thread command buffer.
        mem_AddSegment(0xFFFF0000, 0x1000, NULL);

        // Load elf.
        fseek(fd, 0, SEEK_END);
        u32 elfsize = ftell(fd);
        u8* data = malloc(elfsize);
        fseek(fd, 0, SEEK_SET);
        fread(data, elfsize, 1, fd);

        // Set entrypoint and stack ptr.
        arm11_SetPCSP(load_elf_image(data),
                      0x10000000);

        free(data);
        return 0;
    }

    if (Read32(h.signature) == CIA_MAGIC) {		// Load CIA
        cia_header* ch = (cia_header*)&h;

        ncch_off = 0x20 + ch->hdr_sz;

        ncch_off += ch->cert_sz;
        ncch_off += ch->tik_sz;
        ncch_off += ch->tmd_sz;

        ncch_off = (u32)(ncch_off & ~0xff);
        ncch_off += 0x100;
        fseek(fd, ncch_off, SEEK_SET);

        if (fread(&h, sizeof(h), 1, fd) != 1) {
            ERROR("failed to read header.\n");
            return 1;
        }
    }
    // Load NCCH
    if (memcmp(&h.magic, "NCCH", 4) != 0) {
        ERROR("invalid magic.. wrong file?\n");
        return 1;
    }

    // Read Exheader.
    exheader_header ex;
    if (fread(&ex, sizeof(ex), 1, fd) != 1) {
        ERROR("failed to read exheader.\n");
        return 1;
    }

    bool is_compressed = ex.codesetinfo.flags.flag & 1;
    ex.codesetinfo.name[7] = '\0';
    DEBUG("Name: %s\n", ex.codesetinfo.name);
    DEBUG("Code compressed: %s\n", is_compressed ? "YES" : "NO");

    // Read ExeFS.
    u32 exefs_off = Read32(h.exefsoffset) * 0x200;
    u32 exefs_sz = Read32(h.exefssize) * 0x200;
    DEBUG("ExeFS offset:    %08x\n", exefs_off);
    DEBUG("ExeFS size:      %08x\n", exefs_sz);

    fseek(fd, exefs_off + ncch_off, SEEK_SET);

    exefs_header eh;
    if (fread(&eh, sizeof(eh), 1, fd) != 1) {
        ERROR("failed to read ExeFS header.\n");
        return 1;
    }

    u32 i;
    for (i = 0; i < 8; i++) {
        u32 sec_size = Read32(eh.section[i].size);
        u32 sec_off = Read32(eh.section[i].offset);

        if (sec_size == 0)
            continue;

        DEBUG("ExeFS section %d:\n", i);
        eh.section[i].name[7] = '\0';
        DEBUG("    name:   %s\n", eh.section[i].name);
        DEBUG("    offset: %08x\n", sec_off);
        DEBUG("    size:   %08x\n", sec_size);


        if (strcmp((char*) eh.section[i].name, ".code") == 0) {
            sec_off += exefs_off + sizeof(eh);
            fseek(fd, sec_off + ncch_off, SEEK_SET);

            uint8_t* sec = malloc(AlignPage(sec_size));
            if (sec == NULL) {
                ERROR("section malloc failed.\n");
                return 1;
            }

            if (fread(sec, sec_size, 1, fd) != 1) {
                ERROR("section fread failed.\n");
                return 1;
            }

            // Decompress first section if flag set.
            if (i == 0 && is_compressed) {
                u32 dec_size = GetDecompressedSize(sec, sec_size);
                u8* dec = malloc(AlignPage(dec_size));

                DEBUG("Decompressing..\n");
                if (Decompress(sec, sec_size, dec, dec_size) == 0) {
                    ERROR("section decompression failed.\n");
                    return 1;
                }
                DEBUG("  .. OK\n");

                free(sec);
                sec = dec;
                sec_size = dec_size;
            }

            // Load .code section.
            sec_size = AlignPage(sec_size);
            mem_AddSegment(Read32(ex.codesetinfo.text.address), sec_size, sec);
        }
    }

    // Add .bss segment.
    u32 bss_off = AlignPage(Read32(ex.codesetinfo.data.address) +
                            Read32(ex.codesetinfo.data.codesize));
    mem_AddSegment(bss_off, AlignPage(Read32(ex.codesetinfo.bsssize)), NULL);

    // Add stack segment.
    u32 stack_size = Read32(ex.codesetinfo.stacksize);
    mem_AddSegment(0x10000000 - stack_size, stack_size, NULL);

    // Add thread command buffer.
    mem_AddSegment(0xFFFF0000, 0x1000, NULL);

    // Set entrypoint and stack ptr.
    arm11_SetPCSP(Read32(ex.codesetinfo.text.address),
                  0x10000000);

    // Add Read Only Shared Info
    mem_AddSegment(0x1FF80000, 0x100, NULL);
    mem_Write8(0x1FF80014, 1); //Bit0 set for Retail
    mem_Write32(0x1FF80040, 64 * 1024 * 1024); //Set App Memory Size to 64MB?\

    return 0;
}
