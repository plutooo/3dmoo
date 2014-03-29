#!/bin/bash
gcc -g -o 3dmuu src/main.c src/svc.c src/handles.c src/syscalls/memory.c src/syscalls/ports.c \
    src/loader.c src/mem.c src/arm11/arm11.c src/services/srv.c src/services/apt_u.c \
    src/services/gsp_gpu.c src/syscalls/syn.c src/syscalls/events.c src/screen.c \
    src/services/hid_user.c src/arm11/threads.c -Werror -Wall -Iinc \
    -lSDL -I/usr/include/SDL/
exit $?
