/*
 * Flexible Sigma16 instruction encoders
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dynarr.h"
#include "htab.h"

#include "internal.h"

/* For pseudo instructions with no base value */
static void write_stub(char *str, dynarr *buf, htab *symtab)  {}

/* RRR instructions */
static void write_add(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0x0000); }
static void write_sub(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0x1000); }
static void write_mul(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0x2000); }
static void write_div(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0x3000); }
static void write_cmp(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0x4000); }
static void write_cmplt(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0x5000); }
static void write_cmpeq(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0x6000); }
static void write_cmpgt(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0x7000); }
static void write_inv(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0x8000); }
static void write_and(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0x9000); }
static void write_or(char *str, dynarr *buf, htab *symtab)    { dynarr_addw(buf, 0xa000); }
static void write_xor(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0xb000); }
static void write_addc(char *str, dynarr *buf, htab *symtab)  { dynarr_addw(buf, 0xc000); }
static void write_trap(char *str, dynarr *buf, htab *symtab)  { dynarr_addw(buf, 0xd000); }

/* RX instructions */
static void write_lea(char *str, dynarr *buf, htab *symtab)    { dynarr_addw(buf, 0xf000); }
static void write_load(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0xf001); }
static void write_store(char *str, dynarr *buf, htab *symtab)  { dynarr_addw(buf, 0xf002); }
static void write_jump(char *str, dynarr *buf, htab *symtab)   { dynarr_addw(buf, 0xf003); }
static void write_jumpc0(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0xf004); }
static void write_jumpc1(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0xf005); }
static void write_jumpf(char *str, dynarr *buf, htab *symtab)  { dynarr_addw(buf, 0xf006); }
static void write_jumpt(char *str, dynarr *buf, htab *symtab)  { dynarr_addw(buf, 0xf007); }
static void write_jal(char *str, dynarr *buf, htab *symtab)    { dynarr_addw(buf, 0xf008); }

/* RX jump aliases */
static void write_jumplt(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0xf305); }
static void write_jumple(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0xf104); }
static void write_jumpne(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0xf204); }
static void write_jumpeq(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0xf205); }
static void write_jumpge(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0xf304); }
static void write_jumpgt(char *str, dynarr *buf, htab *symtab) { dynarr_addw(buf, 0xf105); }

static void write_const(char *str, dynarr *buf, htab *symtab)
{
	uint16_t w;

	w = strtol(str, NULL, 10);
	dynarr_addw(buf, w);
}

#include <stdio.h>

static void write_d(char *str, dynarr *buf, htab *symtab)
{
	uint16_t *p, t;

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	t = strtol(str + 1, NULL, 10) & 0xf;
	*p |= t << 8;
}

static void write_a(char *str, dynarr *buf, htab *symtab)
{
	uint16_t *p, t;

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	t = strtol(str + 1, NULL, 10) & 0xf;
	*p |= t << 4;
}

static void write_b(char *str, dynarr *buf, htab *symtab)
{
	uint16_t *p, t;

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	t = strtol(str + 1, NULL, 10) & 0xf;
	*p |= t;
}

static void write_ea(char *str, dynarr *buf, htab *symtab)
{
	char *a_start, *a_end, *a, *disp;

	a_start = strchr(str, '[');
	a_end = strchr(str, ']');
	a = strndup(a_start + 1, a_end - a_start - 1);
	disp = strndup(str, a_start - str);

	write_a(a, buf, symtab);
	if (htab_get(symtab, disp) > 0)
		dynarr_addw(buf, htab_get(symtab, disp));
	else
		write_const(disp, buf, symtab);

	free(a);
	free(disp);
}

static struct s16_opdef opdefs[] = {
	/* RRR instructions */
	{ "add"  , 1, write_add  , 3, { write_d, write_a, write_b } },
	{ "sub"  , 1, write_sub  , 3, { write_d, write_a, write_b } },
	{ "mul"  , 1, write_mul  , 3, { write_d, write_a, write_b } },
	{ "div"  , 1, write_div  , 3, { write_d, write_a, write_b } },
	{ "cmp"  , 1, write_cmp  , 2, { write_a, write_b } },
	{ "cmplt", 1, write_cmplt, 3, { write_d, write_a, write_b } },
	{ "cmpeq", 1, write_cmpeq, 3, { write_d, write_a, write_b } },
	{ "cmpgt", 1, write_cmpgt, 3, { write_d, write_a, write_b } },
	{ "inv"  , 1, write_inv  , 2, { write_d, write_a } },
	{ "and"  , 1, write_and  , 3, { write_d, write_a, write_b } },
	{ "or"   , 1, write_or   , 3, { write_d, write_a, write_b } },
	{ "xor"  , 1, write_xor  , 3, { write_d, write_a, write_b } },
	{ "addc" , 1, write_addc , 3, { write_d, write_a, write_b } },
	{ "trap" , 1, write_trap , 3, { write_d, write_a, write_b } },

	/* RX instructions */
	{ "lea"   , 2, write_lea   , 2, { write_d, write_ea } },
	{ "load"  , 2, write_load  , 2, { write_d, write_ea } },
	{ "store" , 2, write_store , 2, { write_d, write_ea } },
	{ "jump"  , 2, write_jump  , 1, { write_ea } },
	{ "jumpc0", 2, write_jumpc0, 2, { write_d, write_ea } },
	{ "jumpc1", 2, write_jumpc1, 2, { write_d, write_ea } },
	{ "jumpf" , 2, write_jumpf , 2, { write_d, write_ea } },
	{ "jumpt" , 2, write_jumpt , 2, { write_d, write_ea } },
	{ "jal"   , 2, write_jal   , 2, { write_d, write_ea } },

	/* RX jump aliases */
	{ "jumplt", 2, write_jumplt, 1, { write_ea } },
	{ "jumple", 2, write_jumple, 1, { write_ea } },
	{ "jumpne", 2, write_jumpne, 1, { write_ea } },
	{ "jumpeq", 2, write_jumpeq, 1, { write_ea } },
	{ "jumpge", 2, write_jumpge, 1, { write_ea } },
	{ "jumpgt", 2, write_jumpgt, 1, { write_ea } },

	/* Assembler commands */
	{ "data", 1, write_stub, 1, { write_const } },
};

struct s16_opdef *lookup_opdef(char *opcode)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(opdefs); ++i) {
		if (!strcmp(opdefs[i].opcode, opcode))
			return &opdefs[i];
	}

	return NULL;
}
