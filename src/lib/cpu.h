#ifndef CPU_H
#define CPU_H

#define REG_COUNT 0x10
#define RAM_WORDS 0x10000 /* 64K words */

typedef struct {
	/* Decode registers */
	uint16_t pc, ir, adr;
	/* General purpose registers */
	uint16_t reg[REG_COUNT];
	/* RAM */
	uint16_t ram[RAM_WORDS];
} s16cpu;

/*
 * Execute one instruction
 * Returns zero if TRAP_EXIT was run, otherise non-zero
 */
int
execute(s16cpu *cpu);

/*
 * Load program into RAM
 */
ssize_t
load_program(const char *path, s16cpu *cpu);

#endif
