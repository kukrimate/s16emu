/*
 * Flexible Sigma16 instruction encoders
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "dynarr.h"
#include "htab.h"

#include "internal.h"

static int parse_uint(char *str, size_t base, uint16_t *out)
{
	char *p;
	size_t pos, digit, tmp;

	if (!*str) /* Reject empty strings */
		return -1;

	/* We want to look at the last digit first */
	for (p = str; *p; ++p);

	pos = 1;
	tmp = 0;

	/* Compute integer value */
	while (--p >= str) {
		if ('0' <= *p && *p <= '9')
			digit = *p - '0';
		else if ('a' <= *p && *p <= 'z')
			digit = *p - 'a' + 9;
		else if ('A' <= *p && *p <= 'Z')
			digit = *p - 'A' + 9;
		else
			return -1;

		if (digit >= base)
			return -1;

		tmp += digit * pos;
		pos *= base;
	}

	if (tmp > UINT16_MAX) /* Result won't fit */
		return -1;

	*out = (uint16_t) tmp;
	return 0;
}

static int parse_int(char *str, uint16_t *out)
{
	_Bool sgn;

	sgn = 0;

	if ('-' == *str) {
		sgn = 1;
		++str;
	} else if ('+' == *str) {
		++str;
	}

	if (-1 == parse_uint(str, 10, out))
		return -1;

	if (sgn)
		*out = (uint16_t) ~*out + 1;

	return 0;
}

static int assemble_const(char *str, htab *symtab, dynarr *buf)
{
	uint16_t *p;

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	*p = htab_get(symtab, str);
	if (*p > 0)
		return 0;

	if (!strncmp(str, "0o", 2))
		return parse_uint(str + 2, 8, p);
	else if (!strncmp(str, "0b", 2))
		return parse_uint(str + 2, 2, p);
	else if (!strncmp(str, "0x", 2))
		return parse_uint(str + 2, 16, p);
	else
		return parse_int(str, p);
}

static int assemble_d(char *str, htab *symtab, dynarr *buf)
{
	uint16_t r, *p;

	if (*str != 'R' && *str != 'r')
		return -1;
	if (-1 == parse_uint(str + 1, 10, &r) || r > 15)
		return -1;

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	*p |= (uint16_t) r << 8;
	return 0;
}

static int assemble_a(char *str, htab *symtab, dynarr *buf)
{
	uint16_t r, *p;

	if (*str != 'R' && *str != 'r')
		return -1;
	if (-1 == parse_uint(str + 1, 10, &r) || r > 15)
		return -1;

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	*p |= (uint16_t) r << 4;
	return 0;
}

static int assemble_b(char *str, htab *symtab, dynarr *buf)
{
	uint16_t r, *p;

	if (*str != 'R' && *str != 'r')
		return -1;
	if (-1 == parse_uint(str + 1, 10, &r) || r > 15)
		return -1;

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	*p |= (uint16_t) r;
	return 0;
}

static int assemble_ea(char *str, htab *symtab, dynarr *buf)
{
	char *a_start, *a_end, *a, *disp;

	a_start = strchr(str, '[');
	a_end = strchr(str, ']');
	a = strndup(a_start + 1, a_end - a_start - 1);
	disp = strndup(str, a_start - str);

	assemble_a(a, symtab, buf);

 	/* NOTE: add 0 to avoid windback for displacement */
	dynarr_addw(buf, 0);
	assemble_const(disp, symtab, buf);

	free(a);
	free(disp);
}

typedef int (*s16_operand)(char *str, htab *symtab, dynarr *buf);

struct s16_opdef {
	char *mnemonic;
	/* Length in words */
	size_t length;
	/* Opcode to be written to the first byte */
	uint16_t opcode;
	/* Number of operands */
	size_t operand_cnt;
	/* What each operand to be assembled as */
	s16_operand operands[3];
};

static struct s16_opdef opdefs[] = {
	/* RRR instructions */
	{ "add"  , 1, 0x0000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "sub"  , 1, 0x1000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "mul"  , 1, 0x2000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "div"  , 1, 0x3000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "cmp"  , 1, 0x4000, 2, { assemble_a, assemble_b } },
	{ "cmplt", 1, 0x5000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "cmpeq", 1, 0x6000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "cmpgt", 1, 0x7000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "inv"  , 1, 0x8000, 2, { assemble_d, assemble_a } },
	{ "and"  , 1, 0x9000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "or"   , 1, 0xa000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "xor"  , 1, 0xb000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "addc" , 1, 0xc000, 3, { assemble_d, assemble_a, assemble_b } },
	{ "trap" , 1, 0xd000, 3, { assemble_d, assemble_a, assemble_b } },

	/* RX instructions */
	{ "lea"   , 2, 0xf000, 2, { assemble_d, assemble_ea } },
	{ "load"  , 2, 0xf001, 2, { assemble_d, assemble_ea } },
	{ "store" , 2, 0xf002, 2, { assemble_d, assemble_ea } },
	{ "jump"  , 2, 0xf003, 1, { assemble_ea } },
	{ "jumpc0", 2, 0xf004, 2, { assemble_d, assemble_ea } },
	{ "jumpc1", 2, 0xf005, 2, { assemble_d, assemble_ea } },
	{ "jumpf" , 2, 0xf006, 2, { assemble_d, assemble_ea } },
	{ "jumpt" , 2, 0xf007, 2, { assemble_d, assemble_ea } },
	{ "jal"   , 2, 0xf008, 2, { assemble_d, assemble_ea } },

	/* RX jump aliases */
	{ "jumplt", 2, 0xf305, 1, { assemble_ea } },
	{ "jumple", 2, 0xf104, 1, { assemble_ea } },
	{ "jumpne", 2, 0xf204, 1, { assemble_ea } },
	{ "jumpeq", 2, 0xf205, 1, { assemble_ea } },
	{ "jumpge", 2, 0xf304, 1, { assemble_ea } },
	{ "jumpgt", 2, 0xf105, 1, { assemble_ea } },

	/* Assembler commands */
	{ "data", 1, 0x0000, 1, { assemble_const } },
};

static struct s16_opdef *lookup(char *opcode)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(opdefs); ++i) {
		if (!strcmp(opdefs[i].mnemonic, opcode))
			return &opdefs[i];
	}

	return NULL;
}

void assemble(struct s16_token *head, int outfd)
{
	uint16_t address;
	htab labels;
	dynarr code;
	struct s16_token *cur;
	struct s16_opdef *opdef;
	size_t i;
	uint8_t tmp;

	address = 0;
	htab_new(&labels, 32);
	dynarr_alloc(&code, sizeof(uint16_t));

	/* First stage */
	for (cur = head; cur; cur = cur->next)
		switch (cur->type) {
		case LABEL:
			htab_put(&labels, cur->data, address, 0);
			break;
		case OPCODE:
			opdef = lookup(cur->data);
			if (!opdef) {
				fprintf(stderr, "Unkonwn instruction %s\n", cur->data);
				goto done;
			}
			address += opdef->length;
			break;
		default:
			break;
		}

	/* Second stage */
	for (cur = head; cur; cur = cur->next)
		switch (cur->type) {
		case OPCODE:
			opdef = lookup(cur->data);
			dynarr_addw(&code, opdef->opcode);

			for (i = 0; i < opdef->operand_cnt; ++i) {
				cur = cur->next;
				if (-1 == opdef->operands[i](cur->data, &labels, &code)) {
					fprintf(stderr, "Invalid operand %s for %s\n",
						cur->data, opdef->mnemonic);
					goto done;
				}
			}
			break;
		case OPERAND:
			fprintf(stderr, "Unexpected operand %s\n", cur->data);
			goto done;
		default:
			break;
		}

	/* Convert to big-endian and write to output */
	for (i = 0; i < code.elem_cnt; ++i) {
		tmp = dynarr_getw(&code, i) >> 8;
		write(outfd, &tmp, 1);
		tmp = dynarr_getw(&code, i);
		write(outfd, &tmp, 1);
	}

done:
	htab_del(&labels, 0);
	dynarr_free(&code);
}
