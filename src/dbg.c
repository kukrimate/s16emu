/*
 * Debugger
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ncurses.h>
#include <vec.h>
#include <map.h>
#include "lib/cpu.h"
#include "lib/disasm.h"

vec_gen(char, c)

/*
 * Add a single symbol to the table
 */
static int
addsym(const char *line, struct rsymmap *rsymtab)
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
static int
load_symtab(const char *path, struct rsymmap *rsymtab)
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

int
main(int argc, char *argv[])
{
	int opt;
	const char *symtab_path = NULL, *prog_path;
	struct s16cpu cpu;
	struct rsymmap rsymtab;

	/* Parse command line */
	while ((opt = getopt(argc, argv, "hs:")) != -1)
		switch (opt) {
		case 's':
			symtab_path = optarg;
			break;
		case 'h':
		default:
			goto print_usage;
		}

	if (optind >= argc)
		goto print_usage;
	prog_path = argv[optind];

	/* Make sure all registers and RAM is zeroed */
	memset(&cpu, 0, sizeof cpu);
	/* Initialize symbol table */
	rsymmap_init(&rsymtab);

	/* Load program into the CPU's RAM */
	if (load_program(prog_path, &cpu) < 0) {
		perror(prog_path);
		return 1;
	}
	printf("Program binary: %s\n", prog_path);

	/* Load symbol table if specified */
	if (symtab_path && load_symtab(symtab_path, &rsymtab) < 0)
		perror(symtab_path);

	/* Start debugger */
	execute_debug(&cpu, &rsymtab);

	/* Free symbol table and exit */
	rsymmap_free(&rsymtab);
	return 0;

print_usage:
	fprintf(stderr, "Usage: %s [-s SYMTAB] PROG\n", argv[0]);
	return 1;
}