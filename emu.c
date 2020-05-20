#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>

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

/*
 * Execute instruction
 */
void execute(uint16_t *ram, size_t ram_size)
{
	/* Decode registers */
	uint16_t pc, ir, adr;
	/* ISA registers */
	uint16_t gregs[0xf];

	size_t i;
	uint16_t *trapptr;

	/* Reset vector is zero */
	pc = 0;

	for (;;) {
		printf("PC: %04x\t", pc);

		ir = ram[pc++];
		ir = BEWORD(ir);

		switch (ir & 0xf000) {
		case 0x0000: /* add */
			printf("add R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[INSN_RD(ir)] = gregs[INSN_RA(ir)] + gregs[INSN_RB(ir)];
			break;
		case 0x1000: /* sub */
			printf("sub R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[INSN_RD(ir)] = gregs[INSN_RA(ir)] - gregs[INSN_RB(ir)];
			break;
		case 0x2000: /* mul */
			printf("mul R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[INSN_RD(ir)] = gregs[INSN_RA(ir)] * gregs[INSN_RB(ir)];
			break;
		case 0x3000: /* div */
			printf("div R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			if (INSN_RD(ir) != 0xf) {
				gregs[INSN_RD(ir)] = gregs[INSN_RA(ir)] / gregs[INSN_RB(ir)];
				gregs[0xf] = gregs[INSN_RA(ir)] % gregs[INSN_RB(ir)];
			} else {
				gregs[0xf] = gregs[INSN_RA(ir)] / gregs[INSN_RB(ir)];
			}
			break;
		case 0x4000: /* cmp */
			printf("cmp R%d,R%d\n", INSN_RA(ir), INSN_RB(ir));
			EQU_BIT(gregs[0xf], ccE, INSN_RA(ir) == INSN_RB(ir));
			EQU_BIT(gregs[0xf], ccG, INSN_RA(ir) > INSN_RB(ir));
			EQU_BIT(gregs[0xf], ccL, INSN_RA(ir) < INSN_RB(ir));

			/* FIXME: this assumes two's compliment representation by host */
			EQU_BIT(gregs[0xf], ccg, (int16_t) INSN_RA(ir) >
				(int16_t) INSN_RB(ir));
			EQU_BIT(gregs[0xf], ccl, (int16_t) INSN_RA(ir) <
				(int16_t) INSN_RB(ir));
			break;
		case 0x5000: /* cmplt */
			printf("cmplt R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[0xf] = gregs[INSN_RA(ir)] < gregs[INSN_RB(ir)];
			break;
		case 0x6000: /* cmpeq */
			printf("cmpeq R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[0xf] = gregs[INSN_RA(ir)] == gregs[INSN_RB(ir)];
			break;
		case 0x7000: /* cmpgt */
			printf("cmpgt R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));
			gregs[0xf] = gregs[INSN_RA(ir)] > gregs[INSN_RB(ir)];
			break;
		case 0xd000: /* trap */
			printf("trap R%d,R%d,R%d\n", INSN_RD(ir), INSN_RA(ir), INSN_RB(ir));

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

			switch (INSN_RB(ir)) {
			/* NOTE: doc is silent on the issue,
				but I do iAPX86 style wraparound for address overflows */
			case 0: /* lea */
				printf("lea R%d,%04x[R%d]\n", INSN_RD(ir), adr, INSN_RA(ir));
				gregs[INSN_RD(ir)] = (adr + INSN_RA(ir)) % ram_size;
				break;
			case 1: /* load */
				printf("load R%d,%04x[R%d]\n", INSN_RD(ir), adr, INSN_RA(ir));
				gregs[INSN_RD(ir)] = ram[(adr + INSN_RA(ir)) % ram_size];
				break;
			case 2: /* store */
				printf("store R%d,%04x[R%d]\n", INSN_RD(ir), adr, INSN_RA(ir));
				ram[(adr + INSN_RA(ir)) % ram_size] = gregs[INSN_RD(ir)];
				break;
			case 3: /* jump */
				printf("jump R%d,%04x[R%d]\n", INSN_RD(ir), adr, INSN_RA(ir));
				pc = (adr + INSN_RA(ir)) % ram_size;
				break;
			case 4: /* jumpc0 (these have incredibly retarded names but oh well) */
				printf("jumpc0 R%d,%04x[R%d]\n", INSN_RD(ir), adr, INSN_RA(ir));
				if (gregs[0xf] & (1 << INSN_RD(ir)))
					pc = (adr + INSN_RA(ir)) % ram_size;
				break;
			case 5: /* jumpc1 */
				printf("jumpc1 R%d,%04x[R%d]\n", INSN_RD(ir), adr, INSN_RA(ir));
				if (!(gregs[0xf] & (1 << INSN_RD(ir))))
					pc = (adr + INSN_RA(ir)) % ram_size;
				break;
			case 6: /* jumpf */
				printf("jumpf R%d,%04x[R%d]\n", INSN_RD(ir), adr, INSN_RA(ir));
				if (!gregs[INSN_RD(ir)])
					pc = (adr + INSN_RA(ir)) % ram_size;
				break;
			case 7: /* jumpt */
				printf("jumpt R%d,%04x[R%d]\n", INSN_RD(ir), adr, INSN_RA(ir));
				if (gregs[INSN_RD(ir)])
					pc = (adr + INSN_RA(ir)) % ram_size;
				break;
			case 8: /* jal */
				printf("jal R%d,%04x[R%d]\n", INSN_RD(ir), adr, INSN_RA(ir));
				gregs[INSN_RD(ir)] = pc;
				pc = (adr + INSN_RA(ir)) % ram_size;
				break;
			}
			break;
		}

		/* Enforce R0 = 0 */
		gregs[0] = 0;
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

int main(int argc, char *argv[])
{
	uint16_t *ram;
	ssize_t prog_size;

	if (argc < 2) {
		fprintf(stderr, "%s PROGRAM [SYMTAB]\n", argv[0]);
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

	execute(ram, 0xffff);
	return 0;
}
