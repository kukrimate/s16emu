#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>

#include "dynarr.h"
#include "htab.h"

#include "internal.h"

/* Enable tracing */
static _Bool dotrace = 0;

static void trace_print(char *fmt, ...)
{
	va_list ap;

	if (!dotrace)
		return;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

/* Enable stupidly-simple "debugger" */
static _Bool dodebug = 0;

/*
 * Traps
 */

#define TRAP_EXIT  0
#define TRAP_READ  1
#define TRAP_WRITE 2

static void trap_read(struct s16emu *emu, uint16_t a, uint16_t b)
{
	if ((uint32_t) a + b > RAM_WORDS) {
		fprintf(stderr, "WARN: out of bounds trap read detected!!\n");
		return;
	}

	while (b--)
		emu->ram[a++] = getchar();
}

static void trap_write(struct s16emu *emu, uint16_t a, uint16_t b)
{
	if ((uint32_t) a + b > RAM_WORDS) {
		fprintf(stderr, "WARN: out of bounds trap write detected!!\n");
		return;
	}

	while (b--)
		printf("%c", (int) emu->ram[a++]);
}

/*
 * Execute instruction with debugging features
 */
void execute(struct s16emu *emu, htab *symtab)
{
	uint8_t op, d, a, b;

	char adrbuf[10];
	char *adrsym;

	for (;;) {
		trace_print("PC: %04x\t", emu->pc);

		emu->ir = emu->ram[emu->pc++];

		op = INSN_OP(emu->ir);
		d = INSN_RD(emu->ir);
		a = INSN_RA(emu->ir);
		b = INSN_RB(emu->ir);

		switch (op) {
		case 0: /* add */
			trace_print("add R%d,R%d,R%d\n", d, a, b);
			s16add(&emu->reg[15], &emu->reg[d], emu->reg[a], emu->reg[b]);
			break;
		case 1: /* sub */
			trace_print("sub R%d,R%d,R%d\n", d, a, b);
			s16sub(&emu->reg[15], &emu->reg[d], emu->reg[a], emu->reg[b]);
			break;
		case 2: /* mul */
			trace_print("mul R%d,R%d,R%d\n", d, a, b);
			s16mul(&emu->reg[15], &emu->reg[d], emu->reg[a], emu->reg[b]);
			break;
		case 3: /* div */
			trace_print("div R%d,R%d,R%d\n", d, a, b);
			s16div(&emu->reg[d], &emu->reg[15], emu->reg[a], emu->reg[b]);
			break;
		case 4: /* cmp */
			trace_print("cmp R%d,R%d\n", a, b);
			s16cmp(&emu->reg[15], emu->reg[a], emu->reg[b]);
			break;
		case 5: /* cmplt */
			trace_print("cmplt R%d,R%d,R%d\n", d, a, b);
			s16cmplt(&emu->reg[d], emu->reg[a], emu->reg[b]);
			break;
		case 6: /* cmpeq */
			trace_print("cmpeq R%d,R%d,R%d\n", d, a, b);
			emu->reg[d] = emu->reg[a] == emu->reg[b];
			break;
		case 7: /* cmpgt */
			trace_print("cmpgt R%d,R%d,R%d\n", d, a, b);
			s16cmpgt(&emu->reg[d], emu->reg[a], emu->reg[b]);
			break;
		case 8: /* inv */
			trace_print("inv R%d,R%d\n", d, a);
			emu->reg[d] = (uint16_t) ~emu->reg[a];
			break;
		case 9: /* and */
			trace_print("and R%d,R%d,R%d\n", d, a, b);
			emu->reg[d] = emu->reg[a] & emu->reg[b];
			break;
		case 0xa: /* or */
			trace_print("or R%d,R%d,R%d\n", d, a, b);
			emu->reg[d] = emu->reg[a] | emu->reg[b];
			break;
		case 0xb: /* xor */
			trace_print("xor R%d,R%d,R%d\n", d, a, b);
			emu->reg[d] = emu->reg[a] ^ emu->reg[b];
			break;
		case 0xc: /* addc */
			trace_print("addc R%d,R%d,R%d\n", d, a, b);
			s16addc(&emu->reg[15], &emu->reg[d], emu->reg[a], emu->reg[b]);
			break;
		case 0xd: /* trap */
			trace_print("trap R%d,R%d,R%d\n", d, a, b);
			switch (emu->reg[d]) {
			case TRAP_EXIT:
				return;
			case TRAP_READ:
				trap_read(emu, emu->reg[a], emu->reg[b]);
				break;
			case TRAP_WRITE:
				trap_write(emu, emu->reg[a], emu->reg[b]);
				break;
			}
			break;
		case 0xf: /* RX format */
			emu->adr = emu->ram[emu->pc++];

			if (symtab)
				adrsym = htab_get(symtab, emu->adr);
			else
				adrsym = NULL;
			if (!adrsym) {
				snprintf(adrbuf, sizeof(adrbuf), "%04x", emu->adr);
				adrsym = adrbuf;
			}

			switch (b) {
			case 0: /* lea */
				trace_print("lea R%d,%s[R%d]\n", d, adrsym, a);
				emu->reg[d] = emu->adr + emu->reg[a];
				break;
			case 1: /* load */
				trace_print("load R%d,%s[R%d]\n", d, adrsym, a);
				emu->reg[d] = emu->ram[emu->adr + emu->reg[a]];
				break;
			case 2: /* store */
				trace_print("store R%d,%s[R%d]\n", d, adrsym, a);
				emu->ram[emu->adr + emu->reg[a]] = emu->reg[d];
				break;
			case 3: /* jump */
				trace_print("jump R%d,%s[R%d]\n", d, adrsym, a);
				emu->pc = emu->adr + emu->reg[a];
				break;
			case 4: /* jumpc0 */
				trace_print("jumpc0 R%d,%s[R%d]\n", d, adrsym, a);
				if (!GET_BIT(emu->reg[15], d))
					emu->pc = emu->adr + emu->reg[a];
				break;
			case 5: /* jumpc1 */
				trace_print("jumpc1 R%d,%s[R%d]\n", d, adrsym, a);
				if (GET_BIT(emu->reg[15], d))
					emu->pc = emu->adr + emu->reg[a];
				break;
			case 6: /* jumpf */
				trace_print("jumpf R%d,%s[R%d]\n", d, adrsym, a);
				if (!emu->reg[d])
					emu->pc = emu->adr +  emu->reg[a];
				break;
			case 7: /* jumpt */
				trace_print("jumpt R%d,%s[R%d]\n", d, adrsym, a);
				if (emu->reg[d])
					emu->pc = emu->adr + emu->reg[a];
				break;
			case 8: /* jal */
				trace_print("jal R%d,%s[R%d]\n", d, adrsym, a);
				emu->reg[d] = emu->pc;
				emu->pc = emu->adr + emu->reg[a];
				break;
			}
			break;
		}

		/* Enforce R0 = 0 */
		emu->reg[0] = 0;

		if (dodebug) {
			for (size_t i = 0; i < REG_COUNT; ++i) {
				trace_print("R%ld\t: %04x\n", i, emu->reg[i]);
			}
			getchar();
		}
	}
}

/*
 * Load program into RAM
 */
ssize_t load(char *path, uint16_t *ram, size_t ram_words)
{
	FILE *file;
	uint16_t *ptr;
	uint8_t buf[2];

	file = fopen(path, "rb");
	if (!file) {
		perror(path);
		return -1;
	}

	ptr = ram;
	while (0 < fread(buf, 1, sizeof(buf), file)) {
		/* Make sure the program actually fits into RAM */
		if (ptr >= ram + ram_words) {
			fprintf(stderr, "Program too big!\n");
			goto err;
		}

		/* Load big endian words from the file in native endianness */
		*ptr++ = buf[0] << 8 | buf[1];
	}

	fclose(file);
	return ptr - ram;
err:
	fclose(file);
	return -1;
}

/*
 * Add a single symbol to the table
 */
int addsym(char *line, htab *symtab)
{
	char *sym, *p;
	uint16_t addr;

	p = strchr(line, ':');

	if (!p) {
		fprintf(stderr, "Invalid symbol table entry %s\n", line);
		return -1;
	}

	sym  = strndup(line, p - line);
	addr = strtol(p+1, NULL, 10);

	htab_put(symtab, addr, sym, 1);
	return 0;
}

/*
 * Load symbol table into a hash-table in memory
 */
int load_symtab(char *path, htab *symtab)
{
	FILE *file;
	dynarr line;

	ssize_t len;
	char buf[4096], *p;

	file = fopen(path, "rb");
	if (!file) {
		perror(path);
		return -1;
	}

	dynarr_alloc(&line, sizeof(char));
	htab_new(symtab, 32);

	for (;;) {
		len = fread(buf, 1, sizeof(buf), file);
		if (-1 == len)
			goto err;

		if (!len) {
			if (line.elem_cnt > 0) {
				dynarr_addc(&line, 0);
				addsym(line.buffer, symtab);
				line.elem_cnt = 0;
			}
			break;
		}

		for (p = buf; p < buf + len; ++p) {
			if (*p == '\n') {
				if (line.elem_cnt > 0) {
					dynarr_addc(&line, 0);
					addsym(line.buffer, symtab);
					line.elem_cnt = 0;
				}
			} else {
				dynarr_addc(&line, *p);
			}
		}
	}

	fclose(file);
	dynarr_free(&line);
	return 0;
err:
	perror(path);
	fclose(file);
	dynarr_free(&line);
	htab_del(symtab, 1);
	return -1;
}

int main(int argc, char *argv[])
{
	int opt;
	char *arg_sym;

	struct s16emu *emu;
	ssize_t prog_size;

	htab symtab;

	arg_sym = NULL;

	while (-1 != (opt = getopt(argc, argv, "htds:")))
		switch (opt) {
		case 's':
			arg_sym = optarg;
			break;
		case 't':
			dotrace = 1;
			break;
		case 'd':
			dotrace = 1; /* -d implies -t */
			dodebug = 1;
			break;
		case 'h':
		default:
			goto print_usage;
		}

	if (optind >= argc)
		goto print_usage;

	if (arg_sym && -1 == load_symtab(arg_sym, &symtab)) {
		return 1;
	}

	emu = calloc(1, sizeof(struct s16emu));
	if (!emu) /* We can't do much if malloc fails */
		abort();

	prog_size = load(argv[optind], emu->ram, RAM_WORDS);
	if (-1 == prog_size) {
		free(emu);
		return 1;
	}
	printf("Loaded %u word program!\n", (unsigned) prog_size);

	execute(emu, arg_sym ? &symtab : NULL);

	if (arg_sym)
		htab_del(&symtab, 1);
	free(emu);
	return 0;

print_usage:
	fprintf(stderr, "Usage: %s [-h] [-t] [-d] [-s SYM] BIN\n", argv[0]);
	return 1;
}
