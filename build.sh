#!/bin/bash
gcc -g -o 3dmuu src/main.c src/svc.c src/svc/memory.c src/svc/ports.c src/loader.c src/mem.c src/arm11/arm11.c
