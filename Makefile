### 3dmoo Makefile ###
PLATFORM	:=	$(shell uname -s)

ifneq (,$(findstring MINGW32,$(PLATFORM)))
MINGW_LIBS	:=	-lws2_32
MINGW_LDFLAGS	:=	-mconsole
endif

CC      = gcc
CFLAGS  = -c -g -std=c99 -Wno-format-zero-length -iquoteinc -iquotesrc/arm11 -iquotesrc/arm11/vfp `pkg-config sdl2 --cflags` -DMODET -DMODE32 -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_SOURCE -DGDB_STUB
LIBS    = `pkg-config sdl2 --libs` -lm $(MINGW_LIBS)
LDFLAGS = $(MINGW_LDFLAGS)

SRC_FILES = src/color.c src/mem.c src/screen.c src/handles.c src/loader.c src/utils.c src/svc.c src/config.c src/gdb/gdbstubchelper.c src/gdb/gdbstub.c src/gdb/gdb.c

INC_FILES = inc/*

ARM11_FILES = $(foreach dir, src/arm11, $(wildcard $(dir)/*.c)) \
	$(foreach dir, src/arm11/vfp, $(wildcard $(dir)/*.c))

SYSCALLS_FILES = $(foreach dir, src/syscalls,   $(wildcard $(dir)/*.c))
SERVICES_FILES = $(foreach dir, src/services,   $(wildcard $(dir)/*.c))
FS_FILES       = $(foreach dir, src/fs,         $(wildcard $(dir)/*.c))
GPU_FILES      = $(foreach dir, src/gpu,        $(wildcard $(dir)/*.c))
DSP_FILES      = $(foreach dir, src/dsp,        $(wildcard $(dir)/*.c))

# Libraries.
CRYPTO_FILES   = $(foreach dir, src/lib/crypto, $(wildcard $(dir)/*.c))
HTTP_FILES     = $(foreach dir, src/lib/http,   $(wildcard $(dir)/*.c))

C_FILES = $(SRC_FILES) $(ARM11_FILES) $(SYSCALLS_FILES) $(SERVICES_FILES) $(FS_FILES) $(GPU_FILES) $(DSP_FILES) $(CRYPTO_FILES) $(HTTP_FILES)
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
	$(CC) $(OBJECTS) $(MAIN_OBJECTS) $(LDFLAGS) -o $@ $(LIBS)

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
	$(CC) $(OBJECTS) $(TEST_OBJECTS) $(LDFLAGS) -o $@ $(LIBS)
	./test
