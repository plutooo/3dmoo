/*
 * Copyright (C) 2014 - plutoo
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
#include <stdint.h>
#include <string.h>
#include "util.h"

typedef struct {
    uint32_t base;
    uint32_t size;
    uint8_t* phys;
    bool     ro;
} memmap_t;

#define MAX_MAPPINGS 16
static memmap_t mappings[MAX_MAPPINGS];
static size_t   num_mappings;


static int Overlaps(memmap_t* a, memmap_t* b) {
    if(a->base <= b->base && b->base < (a->base+a->size))
        return 1;
    if(a->base <= (b->base+b->size) && (b->base+b->size) < (a->base+a->size))
        return 1;
    return 0;
}

static int Contains(memmap_t* m, uint32_t addr, uint32_t sz) {
    return (m->base <= addr && (addr+sz) <= (m->base+m->size));
}

static int AddMapping(uint32_t base, uint32_t size) {
    if(size == 0)
	return 0;

    if(num_mappings == MAX_MAPPINGS) {
        DEBUG("too many mappings.\n");
        return 1;
    }

    size_t i = num_mappings, j;
    mappings[i].base = base;
    mappings[i].size = size;

    for(j=0; j<num_mappings; j++) {
        if(Overlaps(&mappings[j], &mappings[i])) {
            DEBUG("trying to add overlapping mapping %08x, size=%08x.\n",
		   base, size);
            return 2;
        }
    }

    mappings[i].phys = calloc(sizeof(uint8_t), size);
    if(mappings[i].phys == NULL) {
	DEBUG("calloc failed for %08x, size=%08x\n", base, size);
	return 3;
    }

    num_mappings++;
    return 0;
}

int mem_AddSegment(uint32_t base, uint32_t size, uint8_t* data) {
    int rc;

    rc = AddMapping(base, size);
    if(rc != 0)
	return rc;

    if(data != NULL)
	memcpy(mappings[num_mappings-1].phys, data, size);
    return 0;
}

int mem_Write8(uint32_t addr, uint8_t w) {
    size_t i;

    for(i=0; i<num_mappings; i++) {
        if(Contains(&mappings[i], addr, 1)) {
            mappings[i].phys[addr - mappings[i].base] = w;
            return 0;
	}
    }

    DEBUG("trying to write8 unmapped addr %08x, w=%02x\n", addr, w & 0xff);
    arm11_Dump();
    exit(1);
    return 1;
}

uint8_t mem_Read8(uint32_t addr) {
    size_t i;

    for(i=0; i<num_mappings; i++) {
	if(Contains(&mappings[i], addr, 1)) {
            return mappings[i].phys[addr - mappings[i].base];
	}
    }

    DEBUG("trying to read8 unmapped addr %08x\n", addr);
    arm11_Dump();
    exit(1);
    return 0;
}

int mem_Write16(uint32_t addr, uint16_t w) {
    size_t i;

    for(i=0; i<num_mappings; i++) {
	if(Contains(&mappings[i], addr, 2)) {
	    *(uint16_t*) (&mappings[i].phys[addr - mappings[i].base]) = w; 
	    return 0;
	}
    }

    DEBUG("trying to write16 unmapped addr %08x, w=%04x\n", addr, w & 0xffff);
    arm11_Dump();
    exit(1);
    return 1;
}

uint16_t mem_Read16(uint32_t addr) {
    size_t i;

    for(i=0; i<num_mappings; i++) {
	if(Contains(&mappings[i], addr, 2)) {
            return *(uint16_t*) (&mappings[i].phys[addr - mappings[i].base]);
	}
    }

    DEBUG("trying to read16 unmapped addr %08x\n", addr);
    arm11_Dump();
    exit(1);
    return 0;
}

int mem_Write32(uint32_t addr, uint32_t w) {
    size_t i;

    for(i=0; i<num_mappings; i++) {
	if(Contains(&mappings[i], addr, 4)) {
	    *(uint32_t*) (&mappings[i].phys[addr - mappings[i].base]) = w;
	    return 0;
	}
    }

    DEBUG("trying to write32 unmapped addr %08x, w=%04x\n", addr, w & 0xffff);
    arm11_Dump();
    exit(1);
    return 0;
}

int mem_Read32(uint32_t addr) {
    size_t i;

    for(i=0; i<num_mappings; i++) {
	if(Contains(&mappings[i], addr, 4)) {
            return *(uint32_t*) (&mappings[i].phys[addr - mappings[i].base]);
	}
    }

    DEBUG("trying to read32 unmapped addr %08x\n", addr);
    arm11_Dump();
    exit(1);
    return 0;
}
