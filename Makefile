### 3dmoo Makefile ###

CC      = gcc
CFLAGS  = -c -std=c99 -Iinc -Isrc/arm11 -I/usr/include/SDL2/ -DMODET -DMODE32
LIBS    = -lSDL2
LDFLAGS = $(LIBS)

SRC_FILES = src/mem.c src/screen.c src/handles.c src/loader.c src/svc.c src/filemon.c

INC_FILES = inc/*

ARM11_FILES = src/arm11/armemu.c src/arm11/armsupp.c src/arm11/arminit.c \
	src/arm11/thumbemu.c src/arm11/armcopro.c src/arm11/threads.c \
	src/arm11/wrap.c src/arm11/vfp/vfp.c src/arm11/vfp/vfpdouble.c \
	src/arm11/vfp/vfpinstr.c src/arm11/vfp/vfpsingle.c 

SYSCALLS_FILES= src/syscalls/events.c src/syscalls/memory.c \
	src/syscalls/ports.c src/syscalls/syn.c src/syscalls/arb.c

SERVICES_FILES = src/services/am_u.c src/services/apt_u.c src/services/cfg_u.c \
	src/services/dsp_dsp.c src/services/frd_u.c src/services/fs_user.c \
	src/services/gsp_gpu.c src/services/hid_user.c src/services/ir_u.c \
	src/services/ndm_u.c src/services/ns_s.c src/services/ptm_u.c \
	src/services/cecd_u.c src/services/boss_u.c src/services/srv.c

FS_FILES = src/fs/romfs.c src/fs/shared_extdata.c src/fs/util.c

GPU_FILES = src/gpu/gpu.c

DSP_FILES = src/dsp/dspemu.c

C_FILES = $(SRC_FILES) $(ARM11_FILES) $(SYSCALLS_FILES) $(SERVICES_FILES) $(FS_FILES) $(GPU_FILES) $(DSP_FILES)
OBJECTS=$(C_FILES:.c=.o)

MAIN_FILES=src/main.c
MAIN_OBJECTS=$(MAIN_FILES:.c=.o)

TEST_FILES=tests/test.c
TEST_OBJECTS=$(TEST_FILES:.c=.o)

ARMCC=arm-none-eabi-gcc
ARM_FILE=tests/arm_instr.s
ARM_OUT=$(ARM_FILE:.s=.elf)


all: $(FILES) $(MAIN_FILES) $(INC_FILES) $(TEST_FILES) $(ARM_FILE) 3dmoo test $(ARM_OUT)

# -- MAIN EXECUTABLE ---

3dmoo: $(OBJECTS) $(MAIN_OBJECTS)
	$(CC) $(OBJECTS) $(MAIN_OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
run: all
	./3dmoo $(f) -noscreen
debug: all
	gdb --args ./3dmoo $(f) -noscreen
clean:
	rm -rf 3dmoo test $(ARM_OUT) $(OBJECTS) $(MAIN_OBJECTS) $(TEST_OBJECTS)

# -- TEST EXECUTABLE ---

$(ARM_OUT): $(ARM_FILE)
	$(ARMCC) $(ARM_FILE) -nostdlib -o $(ARM_OUT)

test: $(FILES) $(TEST_FILES) $(OBJECTS) $(TEST_OBJECTS) $(ARM_FILE) $(ARM_OUT)
	$(CC) $(OBJECTS) $(TEST_OBJECTS) $(LDFLAGS) -o $@
	./test
