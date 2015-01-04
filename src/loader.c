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

#include <stdlib.h>

#include "util.h"
#include "arm11.h"
#include "mem.h"
#include "fs.h"
#include "threads.h"
#include "loader.h"
#include "3dsx.h"

#include "crypto/aes.h"
#include "crypto/nin_public_crypto.h"

u8 loader_key[0x10];
u32 loader_txt = 0;
u32 loader_data = 0;
u32 loader_bss = 0;
extern char* codepath;

bool loader_encrypted;
ctr_ncchheader loader_h;

exheader_header ex;

extern u32 modulenum;
extern thread threads[MAX_THREADS];

static u32 Read32(uint8_t p[4])
{
    u32 temp = p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
    return temp;
}
static u16 Read16(uint8_t p[2])
{
    u16 temp = p[0] | p[1] << 8;
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
    /*FILE * pFile;
    pFile = fopen("C:\\devkitPro\\code.code", "rb");
    if (pFile != NULL)
    {
        fread(decompressed, 1, decompressedsize, pFile);
        fclose(pFile);
        return 1;
    }*/

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

struct _3DSX_LoadInfo {
    void* segPtrs[3]; // code, rodata & data
    u32 segAddrs[3];
    u32 segSizes[3];
};

static u32 TranslateAddr(u32 addr, struct _3DSX_LoadInfo *d, u32* offsets)
{
    if (addr < offsets[0])
        return d->segAddrs[0] + addr;
    if (addr < offsets[1])
        return d->segAddrs[1] + addr - offsets[0];
    return d->segAddrs[2] + addr - offsets[1];
}

int Load3DSXFile(FILE* f, u32 baseAddr)
{
    u32 i, j, k, m;

    _3DSX_Header hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
        ERROR("error reading 3DSX header");
        return 2;
    }

    // Endian swap!
#define ESWAP(_field, _type) \
    hdr._field = le_##_type(hdr._field)
    ESWAP(magic, word);
    ESWAP(headerSize, hword);
    ESWAP(relocHdrSize, hword);
    ESWAP(formatVer, word);
    ESWAP(flags, word);
    ESWAP(codeSegSize, word);
    ESWAP(rodataSegSize, word);
    ESWAP(dataSegSize, word);
    ESWAP(bssSize, word);
#undef ESWAP

    if (hdr.magic != _3DSX_MAGIC) {
        ERROR("error not a 3DSX file");
        return 3;
    }

    struct _3DSX_LoadInfo d;
    d.segSizes[0] = (hdr.codeSegSize + 0xFFF) &~0xFFF;
    d.segSizes[1] = (hdr.rodataSegSize + 0xFFF) &~0xFFF;
    d.segSizes[2] = (hdr.dataSegSize + 0xFFF) &~0xFFF;
    u32 offsets[2] = { d.segSizes[0], d.segSizes[0] + d.segSizes[1] };
    u32 dataLoadSize = (hdr.dataSegSize - hdr.bssSize + 0xFFF) &~0xFFF;
    u32 bssLoadSize = d.segSizes[2] - dataLoadSize;
    u32 nRelocTables = hdr.relocHdrSize / 4;
    void* allMem = malloc(d.segSizes[0] + d.segSizes[1] + d.segSizes[2] + (4 *3 * nRelocTables));
    if (!allMem)
        return 3;
    d.segAddrs[0] = baseAddr;
    d.segAddrs[1] = d.segAddrs[0] + d.segSizes[0];
    d.segAddrs[2] = d.segAddrs[1] + d.segSizes[1];
    d.segPtrs[0] = (char*)allMem;
    d.segPtrs[1] = (char*)d.segPtrs[0] + d.segSizes[0];
    d.segPtrs[2] = (char*)d.segPtrs[1] + d.segSizes[1];

    // Skip header for future compatibility.
    fseek(f, hdr.headerSize, SEEK_SET);

    // Read the relocation headers
    u32* relocs = (u32*)((char*)d.segPtrs[2] + hdr.dataSegSize);

    for (i = 0; i < 3; i++)
        if (fread(&relocs[i*nRelocTables], nRelocTables * 4, 1, f) != 1) {
            ERROR("error reading reloc header");
            return 4;
        }

    // Read the segments
    if (fread(d.segPtrs[0], hdr.codeSegSize, 1, f) != 1) {
        ERROR("error reading code");
        return 5;
    }
    if (fread(d.segPtrs[1], hdr.rodataSegSize, 1, f) != 1) {
        ERROR("error reading rodata");
        return 5;
    }
    if (fread(d.segPtrs[2], hdr.dataSegSize - hdr.bssSize, 1, f) != 1) {
        ERROR("error reading data");
        return 5;
    }

    // BSS clear
    memset((char*)d.segPtrs[2] + hdr.dataSegSize - hdr.bssSize, 0, hdr.bssSize);

    // Relocate the segments
    for (i = 0; i < 3; i++) {
        for (j = 0; j < nRelocTables; j++) {
            u32 nRelocs = le_word(relocs[i*nRelocTables + j]);
            if (j >= 2) {
                // We are not using this table - ignore it
                fseek(f, nRelocs*sizeof(_3DSX_Reloc), SEEK_CUR);
                continue;
            }

#define RELOCBUFSIZE 512
            static _3DSX_Reloc relocTbl[RELOCBUFSIZE];

            u32* pos = (u32*)d.segPtrs[i];
            u32* endPos = pos + (d.segSizes[i] / 4);

            while (nRelocs) {
                u32 toDo = nRelocs > RELOCBUFSIZE ? RELOCBUFSIZE : nRelocs;
                nRelocs -= toDo;

                if (fread(relocTbl, toDo*sizeof(_3DSX_Reloc), 1, f) != 1)
                    return 6;

                for (k = 0; k < toDo && pos < endPos; k++) {
                    //DEBUG("(t=%d,skip=%u,patch=%u)\n", j, (u32)relocTbl[k].skip, (u32)relocTbl[k].patch);
                    pos += le_hword(relocTbl[k].skip);
                    u32 num_patches = le_hword(relocTbl[k].patch);
                    for (m = 0; m < num_patches && pos < endPos; m++) {
                        u32 inAddr = (char*)pos - (char*)allMem;
                        u32 addr = TranslateAddr(le_word(*pos), &d, offsets);
                        //DEBUG("Patching %08X <-- rel(%08X,%d) (%08X)\n", baseAddr + inAddr, addr, j, le_word(*pos));
                        switch (j) {
                        case 0:
                            *pos = le_word(addr);
                            break;
                        case 1:
                            *pos = le_word(addr - inAddr);
                            break;
                        }
                        pos++;
                    }
                }
            }
        }
    }

    // Write the data
    if (mem_AddSegment(baseAddr, d.segSizes[0] + d.segSizes[1] + d.segSizes[2], allMem)) {
        ERROR("error in AddSegment");
        return 7;
    }
    free(allMem);

    DEBUG("CODE:   %u pages\n", d.segSizes[0] / 0x1000);
    DEBUG("RODATA: %u pages\n", d.segSizes[1] / 0x1000);
    DEBUG("DATA:   %u pages\n", dataLoadSize / 0x1000);
    DEBUG("BSS:    %u pages\n", bssLoadSize / 0x1000);

    return 0; // Success.
}

static u32 LoadElfFile(u8 *addr)
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

        //round up (this fixes bad malloc implementation in some homebrew)
        memsz = (memsz + 0xFFF)&~0xFFF;

        u8* data = malloc(memsz);
        memset(data, 0, memsz);
        memcpy(data, addr + off, filesz);
        mem_AddSegment(dest, memsz, data);

        free(data);

        if ((phdr[6] & 0x5) == 0x5)loader_txt = dest; //read execute
        if ((phdr[6] & 0x6) == 0x6)loader_data = loader_bss = dest;//read write
    }

    return Read32((u8*) &header[6]);
}
u8* loader_Shared_page;
void loader_golbalmemsetup()
{
    loader_Shared_page = malloc(0x2000);
}
static void CommonMemSetup()
{
    // Add thread command buffer.

    for (int i = 0; i < MAX_THREADS; i++) {
        threads[i].servaddr = 0xFFFF0000 - (0x1000 * (MAX_THREADS-i)); //there is a mirrow of that addr in wrap.c fix this addr as well if you fix this
    }

    mem_AddSegment(0xFFFF0000 - 0x1000 * MAX_THREADS, 0x1000 * (MAX_THREADS+1), NULL);

    // 	Shared page
    mem_AddMappingShared(0x1FF80000, 0x2000, loader_Shared_page, PERM_RW, STAT_SHARED); //this should be 2 0x1000 pages but we don't support perm yet so we make it one

    //is readonly so don't bother if we writes this multible times

    mem_Write32(0x1FF80004, 0); //no update don't boot save mode
    mem_Write32(0x1FF80008, 0x00008002); //load NS
    mem_Write32(0x1FF8000C, 0x00040130);
    mem_Write32(0x1FF80010, 0x2); //SYSCOREVER
    mem_Write8 (0x1FF80014, 0x1); //Bit0 set for Retail Bit1 for debug
    mem_Write8 (0x1FF80016, 0);//PREV_FIRM
    mem_Write32(0x1FF80030, 0); //APPMEMTYPE
    mem_Write32(0x1FF80040, 64 * 1024 * 1024); //Set App Memory Size to 64MB? (Configmem-APPMEMTYPE)
    mem_Write32(0x1FF80044, 0x02C00000); //Set App Memory Size to 46MB?


    mem_Write8(0x1FF80060, 0);//firm 4.1.0
    mem_Write8(0x1FF80061, 0x0);
    mem_Write8(0x1FF80062, 0x22);
    mem_Write8(0x1FF80063, 0x2);
    mem_Write32(0x1FF80064, 0x2); //FIRM_SYSCOREVER 
    mem_Write32(0x1FF80068, 0xBA0E); //FIRM_CTRSDKVERSION 

    mem_Write8(0x1ff81004, 1); //RUNNING_HW (1=product, 2=devboard, 3=debugger, 4=capture) 
    mem_Write8(0x1ff81086, 1); //??? RUNNING_HW (1=product, 2=devboard, 3=debugger, 4=capture) 
    mem_Write8(0x1FF810C0, 1); //headset connected

}

int loader_LoadFile(FILE* fd)
{
    u32 ncch_off = 0;

    // Read header.
    if (fread(&loader_h, sizeof(loader_h), 1, fd) != 1) {
        ERROR("failed to read header.\n");
        return 1;
    }

    // Load NCSD
    if (memcmp(&loader_h.magic, "NCSD", 4) == 0) {
        ncch_off = 0x4000;
        fseek(fd, ncch_off, SEEK_SET);
        if (fread(&loader_h, sizeof(loader_h), 1, fd) != 1) {
            ERROR("failed to read header.\n");
            return 1;
        }
    }

    if (Read32(loader_h.signature) == 0x58534433) { // Load 3DSX
        // Add stack segment.
        mem_AddSegment(0x10000000 - 0x100000, 0x100000, NULL);


        fseek(fd, 0, SEEK_SET);
        // Set entrypoint and stack ptr.
        Load3DSXFile(fd, 0x00100000);
        arm11_SetPCSP(0x00100000, 0x10000000);
        //arm11_SetPCSP(LoadElfFile(data), 0x10000000);
        CommonMemSetup();

        return 0;
    }

    if (Read32(loader_h.signature) == 0x464c457f) { // Load ELF
        // Add stack segment.
        mem_AddSegment(0x10000000 - 0x100000, 0x100000, NULL);

        // Load elf.
        fseek(fd, 0, SEEK_END);
        u32 elfsize = ftell(fd);
        u8* data = malloc(elfsize);
        fseek(fd, 0, SEEK_SET);
        fread(data, elfsize, 1, fd);

        // Set entrypoint and stack ptr. ichfly some entrypoint are wrong so we force them to 0x00100000
        LoadElfFile(data);
        arm11_SetPCSP(0x00100000, 0x10000000);
        //arm11_SetPCSP(LoadElfFile(data), 0x10000000);
        CommonMemSetup();

        free(data);
        return 0;
    }

    if (Read32(loader_h.signature) == CIA_MAGIC) { // Load CIA
        cia_header* ch = (cia_header*)&loader_h;

        ncch_off = 0x20 + ch->hdr_sz;

        ncch_off += ch->cert_sz;
        ncch_off += ch->tik_sz;
        ncch_off += ch->tmd_sz;

        ncch_off = (u32)(ncch_off & ~0xff);
        ncch_off += 0x100;
        fseek(fd, ncch_off, SEEK_SET);

        if (fread(&loader_h, sizeof(loader_h), 1, fd) != 1) {
            ERROR("failed to read header.\n");
            return 1;
        }
    }
    // Load NCCH
    if (memcmp(&loader_h.magic, "NCCH", 4) != 0) {
        ERROR("invalid magic.. wrong file?\n");
        return 1;
    }

    // Read Exheader.
    if (fread(&ex, sizeof(ex), 1, fd) != 1) {
        ERROR("failed to read exheader.\n");
        return 1;
    }

    ctr_aes_context ctx;
    if (!memcmp(ex.arm11systemlocalcaps.programid, loader_h.programid, 8)) {
        // program id's match, so it's probably not encrypted
        loader_encrypted = false;
    } else if (loader_h.flags[7] & 4) {
        loader_encrypted = false; // not encrypted
    } else if (loader_h.flags[7] & 1) {
        if (programid_is_system(loader_h.programid)) {
            // fixed system key
#ifdef NYUU_DEC
            Nyuu_getkey(NINKEY_TYPE_FIX,&loader_h,loader_key);
#else
            loader_encrypted = true;
            ERROR("Fixed system key not included in 3dmoo\n");
            return 1;
#endif
        } else {
            // null key
            loader_encrypted = true;
            memset(loader_key, 0, 0x10);
        }
    } else {
        // secure key (cannot decrypt!)
        loader_encrypted = true;
#ifdef NYUU_DEC
        Nyuu_getkey(NINKEY_TYPE_SEC, &loader_h, loader_key);
#else
        ERROR("Secure keys are not included in 3dmoo.\n");
        return 1;
#endif
    }
    if (loader_encrypted) {
        ncch_extract_prepare(&ctx, &loader_h, NCCHTYPE_EXHEADER, loader_key);
        ctr_crypt_counter(&ctx, (u8*)&ex, (u8*)&ex, sizeof(ex));
    }

    bool is_compressed = ex.codesetinfo.flags.flag & 1;
    //ex.codesetinfo.name[7] = '\0';
    u8 namereal[9];
    strncpy(namereal, ex.codesetinfo.name, 9);
    DEBUG("Name: %s\n", namereal);
    DEBUG("Code compressed: %s\n", is_compressed ? "YES" : "NO");

    // Read ExeFS.
    u32 exefs_off = Read32(loader_h.exefsoffset) * 0x200;
    u32 exefs_sz = Read32(loader_h.exefssize) * 0x200;
    DEBUG("ExeFS offset:    %08x\n", exefs_off);
    DEBUG("ExeFS size:      %08x\n", exefs_sz);

    fseek(fd, exefs_off + ncch_off, SEEK_SET);

    exefs_header eh;
    if (fread(&eh, sizeof(eh), 1, fd) != 1) {
        ERROR("failed to read ExeFS header.\n");
        return 1;
    }
    if (loader_encrypted) {
        ncch_extract_prepare(&ctx, &loader_h, NCCHTYPE_EXEFS, loader_key);
        ctr_crypt_counter(&ctx, (u8*)&eh, (u8*)&eh, sizeof(eh));
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


        bool isfirmNCCH = false;

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
                free(sec);
                return 1;
            }

            if (loader_encrypted) {
                ncch_extract_prepare(&ctx, &loader_h, NCCHTYPE_EXEFS, loader_key);
                ctr_add_counter(&ctx, (sec_off - (exefs_off)) / 0x10);
                ctr_crypt_counter(&ctx, sec, sec, sec_size);
            }


            // Decompress first section if flag set.
            if (i == 0 && is_compressed) {
                u32 dec_size = GetDecompressedSize(sec, sec_size);
                u8* dec = malloc(AlignPage(dec_size));

                if (!dec) {
                    ERROR("decompressed data block allocation failed.\n");
                    free(sec);
                    return 1;
                }

                u32 firmexpected = Read32(ex.codesetinfo.text.codesize) + Read32(ex.codesetinfo.ro.codesize) + Read32(ex.codesetinfo.data.codesize);

                DEBUG("Decompressing..\n");
                if (Decompress(sec, sec_size, dec, dec_size) == 0) {
                    ERROR("section decompression failed.\n");
                    free(sec);
                    free(dec);
                    return 1;
                }
                DEBUG("  .. OK\n");

                /*FILE * pFile;
                pFile = fopen("code.code", "wb");
                if (pFile != NULL)
                {
                    fwrite(dec, 1, dec_size, pFile);
                    fclose(pFile);
                }*/


                if (codepath != NULL) {
                    FILE* pFile = fopen(codepath, "rb");
                    if (pFile != NULL) {
                        fread(dec, 1, dec_size, pFile);
                        fclose(pFile);
                    }
                }

                if (dec_size == firmexpected) {
                    isfirmNCCH = true;
                    DEBUG("firm NCCH detected\n");
                }

                free(sec);
                sec = dec;
                sec_size = dec_size;
            }

            // Load .code section.
            if (isfirmNCCH) {

                mem_AddSegment(Read32(ex.codesetinfo.text.address), Read32(ex.codesetinfo.text.codesize), sec);
                mem_AddSegment(Read32(ex.codesetinfo.ro.address), Read32(ex.codesetinfo.ro.codesize), sec + Read32(ex.codesetinfo.text.codesize));

                u8* temp = malloc(Read32(ex.codesetinfo.bsssize) + Read32(ex.codesetinfo.data.codesize));
                memcpy(temp, sec + Read32(ex.codesetinfo.ro.codesize) + Read32(ex.codesetinfo.text.codesize), Read32(ex.codesetinfo.data.codesize));


                mem_AddSegment(Read32(ex.codesetinfo.data.address), Read32(ex.codesetinfo.data.codesize) + Read32(ex.codesetinfo.bsssize), temp);
                free(temp);
            } else {
                sec_size = AlignPage(sec_size);
                mem_AddSegment(Read32(ex.codesetinfo.text.address), sec_size, sec);
                // Add .bss segment.
                u32 bss_off = AlignPage(Read32(ex.codesetinfo.data.address) +
                                        Read32(ex.codesetinfo.data.codesize));
                mem_AddSegment(bss_off, AlignPage(Read32(ex.codesetinfo.bsssize)), NULL);
            }
            loader_txt = Read32(ex.codesetinfo.text.address);
            loader_data = Read32(ex.codesetinfo.ro.address);
            loader_bss = Read32(ex.codesetinfo.data.address);
            free(sec);
        }
    }

    if (Read32(loader_h.romfsoffset) != 0 && Read32(loader_h.romfssize) != 0) {
        u32 romfs_off = ncch_off + (Read32(loader_h.romfsoffset) * 0x200) + 0x1000;
        u32 romfs_sz = (Read32(loader_h.romfssize) * 0x200) - 0x1000;

        DEBUG("RomFS offset:    %08x\n", romfs_off);
        DEBUG("RomFS size:      %08x\n", romfs_sz);

        ModuleSupport_fssetnumb(modulenum + 1);

        romfs_Setup(fd, romfs_off, romfs_sz, modulenum);
    }

    // Add stack segment.
    u32 stack_size = Read32(ex.codesetinfo.stacksize);
    mem_AddSegment(0x10000000 - stack_size, stack_size, NULL);

    // Set entrypoint and stack ptr.
    arm11_SetPCSP(Read32(ex.codesetinfo.text.address),
                  0x10000000);

    CommonMemSetup();
    return 0;
}
