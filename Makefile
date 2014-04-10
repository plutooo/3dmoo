### 3dmoo Makefile ###

CC=gcc
CFLAGS=-c -std=c99 -Iinc -I/usr/include/SDL2/ -DMODET -DMODE32 -w
LDFLAGS=-lSDL2

FILES=src/mem.c src/screen.c src/handles.c src/loader.c src/svc.c \
	src/syscalls/events.c src/syscalls/memory.c src/syscalls/ports.c src/syscalls/syn.c \
	src/services/apt_u.c src/services/gsp_gpu.c src/services/hid_user.c src/services/srv.c \
	src/arm11/armemu.c src/arm11/armsupp.c src/arm11/arminit.c src/arm11/thumbemu.c \
	src/arm11/threads.c src/arm11/wrap.c \
	src/arm11/vfp/vfp.c src/arm11/vfp/vfpdouble.c src/arm11/vfp/vfpinstr.c src/arm11/vfp/vfpsingle.c \
	src/IO/SrvtoIO.c
OBJECTS=$(FILES:.c=.o)

MAIN_FILES=src/main.c
MAIN_OBJECTS=$(MAIN_FILES:.c=.o)

TEST_FILES=tests/test.c
TEST_OBJECTS=$(TEST_FILES:.c=.o)

ARMCC=arm-none-eabi-gcc
ARM_FILE=tests/arm_instr.s
ARM_OUT=$(ARM_FILE:.s=.elf)


all: $(FILES) $(MAIN_FILES) $(TEST_FILES) $(ARM_FILE) 3dmoo test $(ARM_OUT)

# -- MAIN EXECUTABLE ---

3dmoo: $(OBJECTS) $(MAIN_OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(MAIN_OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
run: all
	./3dmoo $(f) -noscreen
debug: all
	gdb --args ./3dmoo $(f) -noscreen

# -- TEST EXECUTABLE ---

$(ARM_OUT): $(ARM_FILE)
	$(ARMCC) $(ARM_FILE) -nostdlib -o $(ARM_OUT)

test: $(FILES) $(TEST_FILES) $(OBJECTS) $(TEST_OBJECTS) $(ARM_FILE) $(ARM_OUT)
	$(CC) $(LDFLAGS) $(OBJECTS) $(TEST_OBJECTS) -o $@
	./test
