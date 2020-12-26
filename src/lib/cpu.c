/*
 * CPU emulator
 */

#include <stdio.h>
#include <stdint.h>
#include "alu.h"
#include "cpu.h"

/*
 * Traps
 */

#define TRAP_EXIT  0
#define TRAP_READ  1
#define TRAP_WRITE 2

static
void
trap_read(struct s16cpu *cpu, uint16_t a, uint16_t b)
{
	if (a + b > RAM_WORDS) {
		fprintf(stderr, "WARN: out of bounds trap read detected!!\n");
		return;
	}

	while (b--)
		cpu->ram[a++] = getchar();
}

static
void
trap_write(struct s16cpu *cpu, uint16_t a, uint16_t b)
{
	if (a + b > RAM_WORDS) {
		fprintf(stderr, "WARN: out of bounds trap write detected!!\n");
		return;
	}

	while (b--)
		putchar(cpu->ram[a++]);
}

/*
 * Instruction dispatcher
 */

#define INSN_OP(insn) (insn >> 12 & 0xf)
#define INSN_RD(insn) (insn >> 8 & 0xf)
#define INSN_RA(insn) (insn >> 4 & 0xf)
#define INSN_RB(insn) (insn & 0xf)

int
execute(struct s16cpu *cpu)
{
	static void *jmp_RRR[] = {
		&&op_add, &&op_sub, &&op_mul, &&op_div, &&op_cmp, &&op_cmplt,
		&&op_cmpeq, &&op_cmpgt, &&op_inv, &&op_and, &&op_or, &&op_xor,
		&&op_addc, &&op_trap, &&op_exp, &&op_rx
	};

	static void *jmp_RX[] = {
		&&op_lea, &&op_load, &&op_store, &&op_jump, &&op_jumpc0,
		&&op_jumpc1, &&op_jumpf, &&op_jumpt, &&op_jal
	};

	uint8_t op, d, a, b;

	cpu->ir = cpu->ram[cpu->pc++];

	op = INSN_OP(cpu->ir);
	d = INSN_RD(cpu->ir);
	a = INSN_RA(cpu->ir);
	b = INSN_RB(cpu->ir);

	goto *jmp_RRR[op];

	switch (op) {
	case 0: /* add */
	op_add:
		s16add(&cpu->reg[15], &cpu->reg[d], cpu->reg[a], cpu->reg[b]);
		break;
	case 1: /* sub */
	op_sub:
		s16sub(&cpu->reg[15], &cpu->reg[d], cpu->reg[a], cpu->reg[b]);
		break;
	case 2: /* mul */
	op_mul:
		s16mul(&cpu->reg[15], &cpu->reg[d], cpu->reg[a], cpu->reg[b]);
		break;
	case 3: /* div */
	op_div:
		s16div(&cpu->reg[d], &cpu->reg[15], cpu->reg[a], cpu->reg[b]);
		break;
	case 4: /* cmp */
	op_cmp:
		s16cmp(&cpu->reg[15], cpu->reg[a], cpu->reg[b]);
		break;
	case 5: /* cmplt */
	op_cmplt:
		s16cmplt(&cpu->reg[d], cpu->reg[a], cpu->reg[b]);
		break;
	case 6: /* cmpeq */
	op_cmpeq:
		cpu->reg[d] = cpu->reg[a] == cpu->reg[b];
		break;
	case 7: /* cmpgt */
	op_cmpgt:
		s16cmpgt(&cpu->reg[d], cpu->reg[a], cpu->reg[b]);
		break;
	case 8: /* inv */
	op_inv:
		cpu->reg[d] = (uint16_t) ~cpu->reg[a];
		break;
	case 9: /* and */
	op_and:
		cpu->reg[d] = cpu->reg[a] & cpu->reg[b];
		break;
	case 0xa: /* or */
	op_or:
		cpu->reg[d] = cpu->reg[a] | cpu->reg[b];
		break;
	case 0xb: /* xor */
	op_xor:
		cpu->reg[d] = cpu->reg[a] ^ cpu->reg[b];
		break;
	case 0xc: /* addc */
	op_addc:
		s16addc(&cpu->reg[15], &cpu->reg[d], cpu->reg[a], cpu->reg[b]);
		break;
	case 0xd: /* trap */
	op_trap:
		switch (cpu->reg[d]) {
		case TRAP_EXIT:
			return 0;
		case TRAP_READ:
			trap_read(cpu, cpu->reg[a], cpu->reg[b]);
			break;
		case TRAP_WRITE:
			trap_write(cpu, cpu->reg[a], cpu->reg[b]);
			break;
		}
		break;
	case 0xe: /* EXP format, current unused */
	op_exp:
		break;
	case 0xf: /* RX format */
	op_rx:
		cpu->adr = cpu->ram[cpu->pc++];

		goto *jmp_RX[b];

		switch (b) {
		case 0: /* lea */
		op_lea:
			cpu->reg[d] = cpu->adr + cpu->reg[a];
			break;
		case 1: /* load */
		op_load:
			cpu->reg[d] = cpu->ram[cpu->adr + cpu->reg[a]];
			break;
		case 2: /* store */
		op_store:
			cpu->ram[cpu->adr + cpu->reg[a]] = cpu->reg[d];
			break;
		case 3: /* jump */
		op_jump:
			cpu->pc = cpu->adr + cpu->reg[a];
			break;
		case 4: /* jumpc0 */
		op_jumpc0:
			if (!GET_BIT(cpu->reg[15], d))
				cpu->pc = cpu->adr + cpu->reg[a];
			break;
		case 5: /* jumpc1 */
		op_jumpc1:
			if (GET_BIT(cpu->reg[15], d))
				cpu->pc = cpu->adr + cpu->reg[a];
			break;
		case 6: /* jumpf */
		op_jumpf:
			if (!cpu->reg[d])
				cpu->pc = cpu->adr +  cpu->reg[a];
			break;
		case 7: /* jumpt */
		op_jumpt:
			if (cpu->reg[d])
				cpu->pc = cpu->adr + cpu->reg[a];
			break;
		case 8: /* jal */
		op_jal:
			cpu->reg[d] = cpu->pc;
			cpu->pc = cpu->adr + cpu->reg[a];
			break;
		}
		break;
	}

	/* Enforce R0 = 0 */
	cpu->reg[0] = 0;

	return 1;
}

ssize_t
load_program(const char *path, struct s16cpu *cpu)
{
	FILE *file;
	uint16_t *ptr;
	uint8_t buf[2];

	file = fopen(path, "rb");
	if (!file) {
		perror(path);
		return -1;
	}

	ptr = cpu->ram;
	while (fread(buf, sizeof buf, 1, file) > 0) {
		/* Make sure the program actually fits into RAM */
		if (ptr >= cpu->ram + RAM_WORDS) {
			fprintf(stderr, "Program too big!\n");
			goto err;
		}

		/* Load big endian words from the file in native endianness */
		*ptr++ = buf[0] << 8 | buf[1];
	}

	fclose(file);
	return ptr - cpu->ram;
err:
	fclose(file);
	return -1;
}
