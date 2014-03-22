#!/bin/bash
gcc -g -o 3dmuu src/main.c src/svc.c src/handles.c src/svc/memory.c src/svc/ports.c \
    src/loader.c src/mem.c src/arm11/arm11.c src/services/srv.c src/services/apt_u.c \
    src/services/gsp_gpu.c -Werror -Wall
