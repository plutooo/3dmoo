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

#include "util.h"
#include "arm11.h"

int loader_LoadFile(FILE* fd);


int main(int argc, char* argv[]) {
    if(argc != 2) {
	printf("Usage:\n");
	printf("%s <in.ncch>\n", argv[0]);
	return 1;
    }

    FILE* fd = fopen(argv[1], "rb");
    if(fd == NULL) {
	perror("Error opening file");
	return 1;
    }

    arm11_Init();

    // Load file.
    if(loader_LoadFile(fd) != 0) {
	fclose(fd);
	return 1;
    }

    // Execute.
    while(1)
	arm11_Step();

    fclose(fd);
    return 0;
}
