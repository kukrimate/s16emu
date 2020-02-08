#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "dynarr.h"

static int readlines(int fd, dynarr *lines)
{
	ssize_t len;
	char buf[4096], *bptr;
	dynarr tmp;

	dynarr_new(&tmp, sizeof(char));

	while (0 < (len = read(fd, buf, sizeof(buf))))
		for (bptr = buf; bptr < buf + len; ++bptr) {
			switch (*bptr) {
			case '\n':
				if (tmp.elem_count) {
					dynarr_addp(lines, strndup(tmp.buffer, tmp.elem_count));
					tmp.elem_count = 0;
				}
				break;
			default:
				dynarr_addc(&tmp, *bptr);
				break;
			}
		}

	if (-1 == len) {
		perror("read");
		goto err;
	}

	if (tmp.elem_count) /* If the last line isn't '\n' terminated */
		dynarr_addp(lines, strndup(tmp.buffer, tmp.elem_count));

	dynarr_del(&tmp);
	return 0;
err:
	dynarr_del(&tmp);
	return -1;
}

static char *strstrip(char *s, size_t n)
{
	char *start, *end;

	for (start = s; start < s + n; ++start)
		if (!isspace(*start))
			break;
	for (end = s + n - 1; end >= start; --end)
		if (!isspace(*end))
			break;

	return strndup(start, end - start + 1);
}

static void tohex(char *l, dynarr *data)
{
	dynarr tmp;

	char *tstr;
	uint16_t val;

	dynarr_new(&tmp, sizeof(char));
	for (; *l; ++l)
		switch (*l) {
		case ',':
			tstr = strstrip(tmp.buffer, tmp.elem_count);
			val = (uint16_t) strtol(tstr, NULL, 16);
			free(tstr);
			tmp.elem_count = 0;
			dynarr_add(data, 1, &val);
			break;
		default:
			dynarr_addc(&tmp, *l);
			break;
		}
	if (tmp.elem_count) {
		tstr = strstrip(tmp.buffer, tmp.elem_count);
		val = (uint16_t) strtol(tstr, NULL, 16);
		free(tstr);
		tmp.elem_count = 0;
		dynarr_add(data, 1, &val);
	}
	dynarr_del(&tmp);
}

static int load(char *path, dynarr *data, dynarr *reloc)
{
	int fd;
	dynarr lines;
	char **l;

	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		perror("open");
		return -1;
	}

	dynarr_new(&lines, sizeof(char *));

	if (-1 == readlines(fd, &lines))
		goto err;
	dynarr_addp(&lines, NULL);

	for (l = dynarr_ptr(&lines, 0); *l; ++l) {
		if (!strncmp(*l, "data", 4)) {
			tohex(*l + 4, data);
		} else if (!strncmp(*l, "relocate", 8)) {
			tohex(*l + 8, reloc);
		} else {
			fprintf(stderr, "Unrecongized keyword -> %s\n", *l);
			goto err;
		}
	}

	dynarr_delall(&lines);
	close(fd);
	return 0;
err:
	dynarr_delall(&lines);
	close(fd);
	return -1;
}

#define REGISTERS 16
#define RAM_WORDS 0xffff

static void run(uint16_t *ram)
{
	uint16_t regs[REGISTERS];

	uint16_t pc, c, disp;
	uint8_t op, d, a, b;

	pc = 0;
	bzero(regs, sizeof(regs));

	for (;;) {
		c = ram[pc];

		op = (c >> 12) & 0xf;
		d = (c >> 8) & 0xf;
		a = (c >> 4) & 0xf;
		b = c & 0xf;

		if (op == 0xf) {
			pc = (pc + 1) % RAM_WORDS;
			disp = ram[pc];
		}

		switch (op) {
		case 0: /* add */
			printf("add R%u,R%u,R%u\n", d, a, b);
			regs[d] = regs[a] + regs[b];
			break;
		case 1: /* sub */
			printf("sub R%u,R%u,R%u\n", d, a, b);
			regs[d] = regs[a] - regs[b];
			break;
		case 2: /* mul */
			printf("mul R%u,R%u,R%u\n", d, a, b);
			regs[d] = regs[a] * regs[b];
			break;
		case 3: /* div */
			printf("div R%u,R%u,R%u\n", d, a, b);
			regs[d] = regs[a] / regs[b];
			regs[15] = regs[a] % regs[b];
			break;
		case 4: /* cmp */
			break;
		case 5: /* cmplt */
			break;
		case 6: /* cmpeq */
			break;
		case 7: /* cmpgt */
			break;
		case 8: /* inv */
			regs[d] = ~regs[a];
			break;
		case 9: /* and */
			regs[d] = regs[a] & regs[b];
			break;
		case 0xa: /* or */
			regs[d] = regs[a] | regs[b];
			break;
		case 0xb: /* xor */
			regs[d] = regs[a] ^ regs[b];
			break;
		case 0xc: /* nop */
			printf("nop");
			break;
		case 0xd: /* trap */
			printf("trap R%u,R%u,R%u\n", d, a, b);
			if (!d && !a && !b)
				return;
			break;
		case 0xe:
			fprintf(stderr, "EXP formats are not supported as of now\n");
			exit(1);
			break;
		case 0xf:
			switch (b) {
			case 0: /* lea */
				printf("lea R%u,$%x[R%u]\n", d, disp, a);
				regs[d] = regs[a] + disp;
				break;
			case 1: /* load */
				printf("load R%u,$%x[R%u]\n", d, disp, a);
				regs[d] = ram[regs[a] + disp];
				break;
			case 2: /* store */
				printf("store R%u,$%x[R%u]\n", d, disp, a);
				ram[regs[a] + disp] = regs[d];
				break;
			case 3: /* jump */
				pc = regs[a] + disp;
				continue;
			}
			break;
		}

		pc = (pc + 1) % RAM_WORDS;
	}
}

int main(int argc, char *argv[])
{
	dynarr data, reloc;
	uint16_t *ram;

	if (argc < 2) {
		fprintf(stderr, "%s PROGRAM\n", argv[0]);
		return 1;
	}

	dynarr_new(&data, sizeof(uint16_t));
	dynarr_new(&reloc, sizeof(uint16_t));

	if (-1 == load(argv[1], &data, &reloc))
		goto err;
	if (data.elem_count > RAM_WORDS) {
		fprintf(stderr, "Your fucking program is too big\n");
		goto err;
	}

	ram = calloc(RAM_WORDS, sizeof(uint16_t));
	/* Load programs at the bottom of memory for now */
	memcpy(ram, data.buffer, data.elem_count * sizeof(uint16_t));
	run(ram);

	printf("result: %u\n", ram[0x31]);
	free(ram);

	dynarr_del(&data);
	dynarr_del(&reloc);
	return 0;
err:
	dynarr_del(&data);
	dynarr_del(&reloc);
	return 1;
}
