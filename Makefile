# Compiler flags
CFLAGS  := -std=c99 -D_GNU_SOURCE -pedantic -Wall \
	-Wdeclaration-after-statement -Wno-parentheses -g
LDFLAGS :=
LIBS    :=

# Object files
OBJ := dynarr.o emu.o

.PHONY: all
all: s16emu

s16emu: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

.PHONY: clean
clean:
	find -name '*.o' -delete
	rm -f s16emu
