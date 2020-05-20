# Compiler flags
CFLAGS  := -D_GNU_SOURCE -std=c99 -Wpedantic -Wall -g
LDFLAGS :=
LIBS    :=

# Object files
OBJ := emu.o

.PHONY: all
all: s16emu

s16emu: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	rm -f *.o s16emu
