#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include "dynarr.h"
#include "htab.h"

/* Enable or disable tracer */
#define trace_print(x, ...) fprintf(stderr, x, __VA_ARGS__)
// #define trace_print(x, ...)

/* Do byteswapping on LE machines */
#ifdef LITTLE_ENDIAN
	#define BEWORD(x) (x << 8 | x >> 8)
#else
	#define BEWORD(x) x
#endif

#define INSN_RD(insn) (insn >> 8 & 0xf)
#define INSN_RA(insn) (insn >> 4 & 0xf)
#define INSN_RB(insn) (insn & 0xf)

#define ccG 15
#define ccg 14
#define ccE 13
#define ccl 12
#define ccL 11

#define EQU_BIT(x, bit, val) \
	if (val) { x |= (1 << bit); } else { x &= ~(1 << bit); }

static void dumpregs(uint16_t *regs)
{
	size_t i;

	for (i = 0; i < 16; ++i) {
		printf("R%ld:  %04x\n", i, regs[i]);
	}
}

/*
 * Execute instruction
 */
void execute(uint16_t *ram, size_t ram_size, htab *symtab)
{
	/* Decode registers */
	uint16_t pc, ir, adr;
	/* ISA registers */
	uint16_t gregs[0xf];

	size_t i;
	uint16_t *trapptr;

	char adrbuf[10];
	char *adrsym;

	/* Reset vector is zero */
	pc = 0;

	/* Zero ISA registers */
	bzero(gregs, sizeof(uint16_t) * 0xf);

	for (;;) {
		trace_print("PC: %04x\t", pc);

		ir = ram[pc++];
		ir = BEWORD(ir);

		switch (ir & 0xf000) {
		case 0x0000: /* add */
			trace_print("add R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[INSN_RD(ir)] = gregs[INSN_RA(ir)] + gregs[INSN_RB(ir)];
			break;
		case 0x1000: /* sub */
			trace_print("sub R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[INSN_RD(ir)] = gregs[INSN_RA(ir)] - gregs[INSN_RB(ir)];
			break;
		case 0x2000: /* mul */
			trace_print("mul R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[INSN_RD(ir)] = gregs[INSN_RA(ir)] * gregs[INSN_RB(ir)];
			break;
		case 0x3000: /* div */
			trace_print("div R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			if (INSN_RD(ir) != 0xf) {
				gregs[INSN_RD(ir)] = gregs[INSN_RA(ir)] / gregs[INSN_RB(ir)];
				gregs[0xf] = gregs[INSN_RA(ir)] % gregs[INSN_RB(ir)];
			} else {
				gregs[0xf] = gregs[INSN_RA(ir)] / gregs[INSN_RB(ir)];
			}
			break;
		case 0x4000: /* cmp */
			trace_print("cmp R%d,R%d\n", INSN_RA(ir), INSN_RB(ir));
			EQU_BIT(gregs[0xf], ccE, gregs[INSN_RA(ir)] == gregs[INSN_RB(ir)]);
			EQU_BIT(gregs[0xf], ccG, gregs[INSN_RA(ir)] > gregs[INSN_RB(ir)]);
			EQU_BIT(gregs[0xf], ccL, gregs[INSN_RA(ir)] < gregs[INSN_RB(ir)]);

			/* FIXME: this assumes two's compliment representation by host */
			EQU_BIT(gregs[0xf], ccg, (int16_t) gregs[INSN_RA(ir)] >
				(int16_t) gregs[INSN_RB(ir)]);
			EQU_BIT(gregs[0xf], ccl, (int16_t) INSN_RA(ir) <
				(int16_t) gregs[INSN_RB(ir)]);
			break;
		case 0x5000: /* cmplt */
			trace_print("cmplt R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[0xf] = gregs[INSN_RA(ir)] < gregs[INSN_RB(ir)];
			break;
		case 0x6000: /* cmpeq */
			trace_print("cmpeq R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[0xf] = gregs[INSN_RA(ir)] == gregs[INSN_RB(ir)];
			break;
		case 0x7000: /* cmpgt */
			trace_print("cmpgt R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[0xf] = gregs[INSN_RA(ir)] > gregs[INSN_RB(ir)];
			break;
		case 0xd000: /* trap */
			trace_print("trap R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));

			/* End of program */
			if (!INSN_RD(ir) && !INSN_RA(ir) && !INSN_RB(ir))
				return;

			/* Trap write */
			if (gregs[INSN_RD(ir)] == 2) {
				trapptr = ram + gregs[INSN_RA(ir)];
				for (i = 0; i < gregs[INSN_RB(ir)]; ++i)
					printf("%c", (int) BEWORD(trapptr[i]));
				if (i > 0)
					printf("\n");
			}

			break;
		case 0xf000: /* RX format */
			adr = ram[pc++];
			adr = BEWORD(adr);

			if (symtab)
				adrsym = htab_get(symtab, adr);
			else
				adrsym = NULL;

			if (!adrsym) {
				snprintf(adrbuf, sizeof(adrbuf), "%04x", adr);
				adrsym = adrbuf;
			}

			switch (INSN_RB(ir)) {
			/* NOTE: doc is silent on the issue,
				but I do iAPX86 style wraparound for address overflows */
			case 0: /* lea */
				trace_print("lea R%d,%s[R%d]\n", INSN_RD(ir), adrsym, INSN_RA(ir));
				gregs[INSN_RD(ir)] = (adr + gregs[INSN_RA(ir)]) % ram_size;
				break;

			case 1: /* load */
				trace_print("load R%d,%s[R%d]\n", INSN_RD(ir), adrsym, INSN_RA(ir));
				gregs[INSN_RD(ir)] = BEWORD(ram[(adr + gregs[INSN_RA(ir)]) % ram_size]);
				break;
			case 2: /* store */
				trace_print("store R%d,%s[R%d]\n", INSN_RD(ir), adrsym, INSN_RA(ir));
				ram[(adr +  gregs[INSN_RA(ir)]) % ram_size] = BEWORD(gregs[INSN_RD(ir)]);
				break;

			case 3: /* jump */
				trace_print("jump R%d,%s[R%d]\n", INSN_RD(ir), adrsym, INSN_RA(ir));
				pc = (adr +  gregs[INSN_RA(ir)]) % ram_size;
				break;
			case 4: /* jumpc0 */
				trace_print("jumpc0 R%d,%s[R%d]\n", INSN_RD(ir), adrsym, INSN_RA(ir));
				if (!(gregs[0xf] & (0x8000 >> INSN_RD(ir))))
					pc = (adr + gregs[INSN_RA(ir)]) % ram_size;
				break;
			case 5: /* jumpc1 */
				trace_print("jumpc1 R%d,%s[R%d]\n", INSN_RD(ir), adrsym, INSN_RA(ir));
				if (gregs[0xf] & (0x8000 >> INSN_RD(ir)))
					pc = (adr + gregs[INSN_RA(ir)]) % ram_size;
				break;
			case 6: /* jumpf */
				trace_print("jumpf R%d,%s[R%d]\n", INSN_RD(ir), adrsym, INSN_RA(ir));
				if (!gregs[INSN_RD(ir)])
					pc = (adr +  gregs[INSN_RA(ir)]) % ram_size;
				break;
			case 7: /* jumpt */
				trace_print("jumpt R%d,%s[R%d]\n", INSN_RD(ir), adrsym, INSN_RA(ir));
				if (gregs[INSN_RD(ir)])
					pc = (adr + gregs[INSN_RA(ir)]) % ram_size;
				break;
			case 8: /* jal */
				trace_print("jal R%d,%s[R%d]\n", INSN_RD(ir), adrsym, INSN_RA(ir));
				gregs[INSN_RD(ir)] = pc;
				pc = (adr + gregs[INSN_RA(ir)]) % ram_size;
				break;
			}
			break;
		}

		/* Enforce R0 = 0 */
		gregs[0] = 0;

		dumpregs(gregs);
		getchar();
	}
}

/*
 * Load program into RAM
 */
ssize_t load(char *path, uint16_t *ram, size_t ram_size)
{
	int fd;
	off_t prog_size;

	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		perror(path);
		return -1;
	}

	prog_size = lseek(fd, 0, SEEK_END);
	if (-1 == prog_size) {
		close(fd);
		perror(path);
		return -1;
	}
	if (prog_size % sizeof(uint16_t)) {
		fprintf(stderr, "Program size needs to be multiple of word size!\n");
		close(fd);
		return -1;
	}
	if (prog_size / sizeof(uint16_t) > ram_size) {
		fprintf(stderr, "Program size must be less then or equal to RAM size!\n");
		close(fd);
		return -1;
	}

	if (-1 == pread(fd, ram, prog_size, 0)) {
		perror(path);
		close(fd);
		return -1;
	}

	close(fd);
	return prog_size / sizeof(uint16_t);
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
	int fd;
	dynarr line;

	ssize_t len;
	char buf[4096], *p;

	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		perror(path);
		return -1;
	}

	dynarr_alloc(&line, sizeof(char));
	htab_new(symtab, 32);

	for (;;) {
		len = read(fd, buf, sizeof(buf));
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

	dynarr_free(&line);
	return 0;
err:
	perror(path);
	close(fd);
	dynarr_free(&line);
	htab_del(symtab, 1);
	return -1;
}

int main(int argc, char *argv[])
{
	uint16_t *ram;
	ssize_t prog_size;

	htab symtab;

	if (argc < 2) {
		fprintf(stderr, "%s PROGRAM [SYMTAB]\n", argv[0]);
		return 1;
	}

	if (argc >= 3 &&
			-1 == load_symtab(argv[2], &symtab)) {
		return 1;
	}

	ram = calloc(0xffff, sizeof(uint16_t));
	if (!ram) /* We can't do much if malloc fails */
		abort();

	prog_size = load(argv[1], ram, 0xffff);
	if (-1 == prog_size) {
		free(ram);
		return 1;
	}
	printf("Loaded %ld word program!\n", prog_size);

	if (argc >= 3) {
		execute(ram, 0xffff, &symtab);
		htab_del(&symtab, 1);
	} else {
		execute(ram, 0xffff, NULL);
	}

	return 0;
}
