# Flags
CFLAGS  := -std=c99 -Wall -Wpedantic -D_GNU_SOURCE -Isrc/lib -g
LDFLAGS := -g
LIBS    :=

# Shared
LIB_OBJ := \
	src/lib/dynarr.o \
	src/lib/htab.o \
	src/lib/htab_ui16.o

# Assembler
ASM_OBJ := \
	src/asm/insn.o \
	src/asm/tok.o \
	src/asm/main.o

# Emulator
EMU_OBJ := \
	src/emu/alu.o \
	src/emu/emu.o

.PHONY: all
all: s16asm s16emu

s16asm: $(ASM_OBJ) $(LIB_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

s16emu: $(EMU_OBJ) $(LIB_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -f $(LIB_OBJ) $(ASM_OBJ) $(EMU_OBJ) s16emu s16asm
