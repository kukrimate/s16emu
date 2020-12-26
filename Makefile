# Flags
CFLAGS  := -std=c99 -Wall -D_GNU_SOURCE -Ilibkm -O1
LDFLAGS :=
LIBS    := -lncurses

# Assembler
ASM_OBJ := \
	src/asm/lexer.o \
	src/asm/parser.o \
	src/asm/insn.o \
	src/asm/main.o

# Disassembler
DIS_OBJ := \
	src/lib/disasm.o \
	src/dis.o

# Debugger
DBG_OBJ := \
	src/lib/alu.o \
	src/lib/cpu.o \
	src/lib/disasm.o \
	src/dbg.o

# Emulator
EMU_OBJ := \
	src/lib/alu.o \
	src/lib/cpu.o \
	src/lib/disasm.o \
	src/emu.o

# Programs
.PHONY: all
all: s16asm s16dis s16dbg s16emu

s16asm: $(ASM_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

s16dis: $(DIS_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

s16dbg: $(DBG_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

s16emu: $(EMU_OBJ)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -f $(ASM_OBJ) $(DIS_OBJ) $(DBG_OBJ) $(EMU_OBJ) s16emu s16dis s16dbg s16asm
