# Flags
CFLAGS  := -std=c99 -Wall -D_GNU_SOURCE -DCOMPGOTO -Ilibkm -g -O1
LDFLAGS := -g
LIBS    := -lncurses

# Shared
LIB_OBJ :=

# Assembler
ASM_OBJ := \
	src/asm/lexer.o \
	src/asm/parser.o \
	src/asm/insn.o \
	src/asm/main.o

# Emulator
EMU_OBJ := \
	src/emu/alu.o \
	src/emu/cpu.o \
	src/emu/disasm.o \
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
