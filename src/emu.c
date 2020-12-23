/*
 * Emulator
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "lib/cpu.h"

int
main(int argc, char *argv[])
{
	struct s16cpu cpu;
	ssize_t prog_size;

	if (argc < 2)
		goto print_usage;

	/* Make sure all registers and RAM is zeroed */
	memset(&cpu, 0, sizeof cpu);

	/* Load program into the CPU's RAM */
	prog_size = load_program(argv[1], &cpu);
	if (prog_size < 0)
		return 1;

	/* Execute until an EXIT trap is hit */
	while (execute(&cpu))
		;
	return 0;

print_usage:
	fprintf(stderr, "Usage: %s BIN\n", argv[0]);
	return 1;
}
