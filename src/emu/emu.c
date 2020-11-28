#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <vec.h>
#include <map.h>
#include "cpu.h"
#include "disasm.h"

vec_gen(char, c)

/*
 * Add a single symbol to the table
 */
static
int
addsym(char *line, struct rsymmap *rsymtab)
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

	rsymmap_put(rsymtab, addr, sym);
	return 0;
}

/*
 * Load symbol table into a hash-table in memory
 */
static
int
load_symtab(char *path, struct rsymmap *rsymtab)
{
	FILE *file;
	struct cvec line;

	ssize_t len;
	char buf[4096], *p;

	file = fopen(path, "rb");
	if (!file) {
		perror(path);
		return -1;
	}

	cvec_init(&line);

	for (;;) {
		len = fread(buf, 1, sizeof(buf), file);
		if (-1 == len) {
			perror(path);
			goto err_close;
		}

		if (!len) {
			if (line.n > 0) {
				cvec_add(&line, 0);
				addsym(line.arr, rsymtab);
				line.n = 0;
			}
			break;
		}

		for (p = buf; p < buf + len; ++p) {
			if (*p == '\n') {
				if (line.n > 0) {
					cvec_add(&line, 0);
					addsym(line.arr, rsymtab);
					line.n = 0;
				}
			} else {
				cvec_add(&line, *p);
			}
		}
	}

	fclose(file);
	cvec_free(&line);
	return 0;
err_close:
	fclose(file);
	cvec_free(&line);
	return -1;
}

/*
 * Load program into RAM
 */
static
ssize_t
load(char *path, uint16_t *ram, size_t ram_words)
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
	while (fread(buf, 1, sizeof(buf), file) > 0) {
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

struct winbox {
	WINDOW *border, *content;
};

static
void
winbox_create(struct winbox *self, int height, int width, int y, int x)
{
	self->border = newwin(height, width, y, x);
	box(self->border, 0, 0);
	wrefresh(self->border);
	self->content = newwin(height - 2, width - 2, y + 1, x + 1);
	scrollok(self->content, 1);
	wrefresh(self->content);
}

static
void
winbox_refresh(struct winbox *self)
{
	wrefresh(self->border);
	wrefresh(self->content);
}

static
void
regs_refresh(struct winbox *regs, struct s16cpu *cpu)
{
	size_t reg_idx;

	wmove(regs->content, 0, 0);
	wprintw(regs->content, "PC:\t%04x", cpu->pc);
	wprintw(regs->content, "\nIR:\t%04x", cpu->ir);
	wprintw(regs->content, "\nADR:\t%04x", cpu->adr);
	for (reg_idx = 0; reg_idx < REG_COUNT; ++reg_idx)
		wprintw(regs->content, "\nR%d:\t%04x",
				reg_idx, cpu->reg[reg_idx]);
	wrefresh(regs->content);
}

static
void
execute_debug(struct s16cpu *cpu, struct rsymmap *symtab)
{
	int height, width;

	struct winbox regs;

	struct winbox disasm;
	char disbuf[50];

	size_t cmdline_height;
	struct winbox cmdline;
	char lastcmd[100], cmd[100];

	initscr();
	refresh();

	getmaxyx(stdscr, height, width);
	cmdline_height = height / 5;

	winbox_create(&regs, height - cmdline_height, 15, 0, 0);
	winbox_create(&disasm, height - cmdline_height, width - 16, 0, 16);
	regs_refresh(&regs, cpu);
	winbox_create(&cmdline, cmdline_height, width, height - cmdline_height, 0);
	strcpy(lastcmd, "invalid");

	for (;;) {
		disassemble(disbuf, sizeof(disbuf), &cpu->ram[cpu->pc], symtab);
		wprintw(disasm.content, "%04x: %s", cpu->pc, disbuf);
		winbox_refresh(&disasm);

read_cmd:
		wprintw(cmdline.content, "> ");
		winbox_refresh(&cmdline);
		wgetnstr(cmdline.content, cmd, sizeof(cmd));
parse_cmd:
		if (!strncmp("n", cmd, 1)) {
			;
		} else if (!strncmp("q", cmd, 1)) {
			break;
		} else if (!strcmp("", cmd)) {
			memcpy(cmd, lastcmd, sizeof(cmd));
			goto parse_cmd;
		} else {
			wprintw(cmdline.content, "?\n");
			winbox_refresh(&cmdline);
			goto read_cmd;
		}
		memcpy(lastcmd, cmd, sizeof(lastcmd));

		if (!execute(cpu))
			break;

		regs_refresh(&regs, cpu);
		wprintw(disasm.content, "\n");
	}

	endwin();
}

enum {
	EXECUTE_FAST,
	EXECUTE_TRACE,
	EXECUTE_DEBUG
};

int
main(int argc, char *argv[])
{
	int opt, execute_type;

	char *arg_sym;
	struct rsymmap rsymtab;

	struct s16cpu *cpu;
	ssize_t prog_size;

	char disbuf[50];

	execute_type = EXECUTE_FAST;
	arg_sym = NULL;

	while (-1 != (opt = getopt(argc, argv, "htds:")))
		switch (opt) {
		case 't':
			execute_type = EXECUTE_TRACE;
			break;
		case 'd':
			execute_type = EXECUTE_DEBUG;
			break;
		case 's':
			arg_sym = optarg;
			break;
		case 'h':
		default:
			goto print_usage;
		}

	if (optind >= argc)
		goto print_usage;

	rsymmap_init(&rsymtab);
	if (arg_sym && -1 == load_symtab(arg_sym, &rsymtab)) {
		rsymmap_free(&rsymtab);
		return 1;
	}

	if (!(cpu = calloc(1, sizeof(struct s16cpu))))
		abort();

	prog_size = load(argv[optind], cpu->ram, RAM_WORDS);
	if (-1 == prog_size) {
		rsymmap_free(&rsymtab);
		free(cpu);
		return 1;
	}

	switch (execute_type) {
	case EXECUTE_FAST:
		while (execute(cpu))
			;
		break;
	case EXECUTE_TRACE:
		do {
			disassemble(disbuf, sizeof(disbuf),
				&cpu->ram[cpu->pc], &rsymtab);
			printf("%s\n", disbuf);
		} while (execute(cpu));
		break;
	case EXECUTE_DEBUG:
		execute_debug(cpu, &rsymtab);
		break;
	}

	rsymmap_free(&rsymtab);
	free(cpu);
	return 0;

print_usage:
	fprintf(stderr, "Usage: %s [-h] [-t | -d] [-s SYM] BIN\n", argv[0]);
	return 1;
}
