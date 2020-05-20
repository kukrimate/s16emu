# Compiler flags
CFLAGS  := -Ilib -D_GNU_SOURCE -std=c99 -Wpedantic -Wall -g
LDFLAGS :=
LIBS    :=

# Object files
OBJ := \
	lib/dynarr.o \
	lib/htab.o \
	src/math.o \
	src/emu.o


.PHONY: all
all: s16emu

s16emu: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -f $(OBJ) s16emu
