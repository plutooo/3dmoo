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
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <SDL.h>

#include "util.h"
#include "arm11.h"
#include "screen.h"
#include "SrvtoIO.h"

int loader_LoadFile(FILE* fd);

static int running = 1;
static int noscreen = 0;

void AtSig(int t)
{
    running = 0;
    screen_Free();
    exit(1);
}

void AtExit()
{
    if(!noscreen)
        screen_Free();

    printf("\nEXIT\n");
}

int main(int argc, char* argv[])
{
    atexit(AtExit);
    if(argc < 2) {
        printf("Usage:\n");
        printf("%s <in.ncch> [-d|-noscreen]\n", argv[0]);
        return 1;
    }

    bool disasm = (argc > 2) && (strcmp(argv[2], "-d") == 0);
    noscreen =    (argc > 2) && (strcmp(argv[2], "-noscreen") == 0);

    FILE* fd = fopen(argv[1], "rb");
    if(fd == NULL) {
        perror("Error opening file");
        return 1;
    }

    signal(SIGINT, AtSig);

    if(!noscreen)
        screen_Init();

    arm11_Init();
    if(!noscreen)
        initGPU();

    // Load file.
    if(loader_LoadFile(fd) != 0) {
        fclose(fd);
        return 1;
    }

    // Execute.
    while(running) {
        if(!noscreen)
            screen_HandleEvent();

        for (int i = 0; i < 0x1000; i++) { // todo
            uint32_t pc = arm11_R(15);

            if (disasm) {
                printf("[%08x] ", pc);
                arm11_Disasm32(pc);
            }

            arm11_Step();
        }
        if(!noscreen)
            screen_RenderGPU();
    }


    fclose(fd);
    return 0;
}
