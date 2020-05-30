/*
 * Instruction encoder for s16asm
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "dynarr.h"
#include "htab.h"

#include "lexer.h"
#include "parser.h"

static int assemble_const
	(struct s16_parse_token *ptok, htab *symtab, dynarr *buf)
{
	uint16_t *p;

	p = dynarr_ptr(buf, buf->elem_cnt - 1);

	if (OPERAND_CONSTANT == ptok->type) {
		*p = (uint16_t) ptok->data.l;
	} else if (OPERAND_LABEL == ptok->type) {
		*p = htab_get(symtab, ptok->data.s);
		if (!*p) {
			fprintf(stderr, "Undefined label %s\n",  ptok->data.s);
			return -1;
		}
	} else {
		fprintf(stderr, "Invalid constant or label\n");
		return -1;
	}

	return 0;
}

static int assemble_d
	(struct s16_parse_token *ptok, htab *symtab, dynarr *buf)
{
	uint16_t *p;

	if (OPERAND_REGISTER != ptok->type) {
		fprintf(stderr, "Invalid register\n");
		return -1;
	}

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	*p |= (uint16_t) ptok->data.l << 8;
	return 0;
}

static int assemble_a
	(struct s16_parse_token *ptok, htab *symtab, dynarr *buf)
{
	uint16_t *p;

	if (OPERAND_REGISTER != ptok->type) {
		fprintf(stderr, "Invalid register\n");
		return -1;
	}

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	*p |= (uint16_t) ptok->data.l << 4;
	return 0;
}

static int assemble_b
	(struct s16_parse_token *ptok, htab *symtab, dynarr *buf)
{
	uint16_t *p;

	if (OPERAND_REGISTER != ptok->type) {
		fprintf(stderr, "Invalid register\n");
		return -1;
	}

	p = dynarr_ptr(buf, buf->elem_cnt - 1);
	*p |= (uint16_t) ptok->data.l;
	return 0;
}

static int assemble_ea
	(struct s16_parse_token *ptok, htab *symtab, dynarr *buf)
{
	if (OPERAND_EADDRESS != ptok->type) {
		fprintf(stderr, "Invalid effective address\n");
		return -1;
	}

	if (-1 == assemble_a(ptok->children[1], symtab, buf))
		return -1;
	dynarr_addw(buf, 0); /* NOTE: add 0 to avoid windback for displacement */
	return assemble_const(ptok->children[0], symtab, buf);
}

static int assemble_ascii
	(struct s16_parse_token *ptok, htab *symtab, dynarr *buf)
{
	char *str;

	if (OPERAND_STRING_LITERAL != ptok->type) {
		fprintf(stderr, "Invalid string literal\n");
		return -1;
	}

	/* NOTE: get rid of non-existent opcode the assembler wrote */
	buf->elem_cnt -= 1;
	for (str = ptok->data.s; *str; ++str)
		dynarr_addw(buf, *str);
	dynarr_addw(buf, 0);

	return 0;
}

typedef int (*s16_operand)
	(struct s16_parse_token *ptok, htab *symtab, dynarr *buf);

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
	{ "ascii", 1, 0x0000, 1, { assemble_ascii } },
};

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

static struct s16_opdef *lookup(char *opcode)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(opdefs); ++i) {
		if (!strcmp(opdefs[i].mnemonic, opcode))
			return &opdefs[i];
	}

	return NULL;
}

void assemble(struct s16_parse_token *root, int outfd)
{
	uint16_t address;
	htab labels;
	dynarr code;
	size_t i, j;
	struct s16_opdef *opdef;
	uint8_t tmp;

	address = 0;
	htab_new(&labels, 32);
	dynarr_alloc(&code, sizeof(uint16_t));

	/* First stage */
	for (i = 0; i < root->child_cnt; ++i)
		switch (root->children[i]->type) {
		case LABEL:
			htab_put(&labels, root->children[i]->data.s, address, 0);
			break;
		case OPCODE:
			opdef = lookup(root->children[i]->data.s);
			if (!opdef) {
				fprintf(stderr,
					"Unkonwn instruction %s\n", root->children[i]->data.s);
				goto done;
			}
			address += opdef->length;
			break;
		default:
			break;
		}

	/* Second stage */
	for (i = 0; i < root->child_cnt; ++i)
		if (OPCODE == root->children[i]->type) {
			// printf("%d %s\n", i, root->children[i]->data.s);

			opdef = lookup(root->children[i]->data.s);
			if (root->children[i]->child_cnt != opdef->operand_cnt) {
				fprintf(stderr,
					"Invalid number of operands for %s\n", opdef->mnemonic);
				goto done;
			}

			dynarr_addw(&code, opdef->opcode);

			for (j = 0; j < opdef->operand_cnt; ++j) {
				if (-1 == opdef->operands[j]
						(root->children[i]->children[j], &labels, &code))
					goto done;
			}
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
