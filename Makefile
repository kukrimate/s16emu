ifeq ($(BUILD_WIN),1)

include Makefile.win

else

# Compiler flags
CFLAGS  := -Ilib -D_GNU_SOURCE -std=c99 -Wpedantic -Wall -O2 -g
LDFLAGS := -g
LIBS    :=

# Object files
OBJ := \
	lib/dynarr.o \
	lib/htab_ui16.o \
	src/alu.o \
	src/emu.o

ASM_OBJ := \
	lib/dynarr.o \
	lib/htab.o \
	asm/insn.o \
	asm/tok.o \
	asm/main.o

.PHONY: all
all: s16asm s16emu

s16asm: $(ASM_OBJ)
	$(CC) $(LDFLAGS) $(ASM_OBJ) -o $@ $(LIBS)

s16emu: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -f $(OBJ) $(ASM_OBJ) s16emu s16asm

endif
