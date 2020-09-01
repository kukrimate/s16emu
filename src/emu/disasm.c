/*
 * Disassembler
 */

#include <stdio.h>
#include <stdint.h>
#include <htab_ui16.h>

#define INSN_OP(insn) (insn >> 12 & 0xf)
#define INSN_RD(insn) (insn >> 8 & 0xf)
#define INSN_RA(insn) (insn >> 4 & 0xf)
#define INSN_RB(insn) (insn & 0xf)

uint16_t *
disassemble(char *str, size_t size, uint16_t *mem, htab_ui16 *symtab)
{
	uint8_t op, d, a, b;

	char adrbuf[10];
	char *adrsym = NULL;

	op = INSN_OP(*mem);
	d = INSN_RD(*mem);
	a = INSN_RA(*mem);
	b = INSN_RB(*mem);

	switch (op) {
	case 0: /* add */
		snprintf(str, size, "add R%d,R%d,R%d", d, a, b);
		break;
	case 1: /* sub */
		snprintf(str, size, "sub R%d,R%d,R%d", d, a, b);
		break;
	case 2: /* mul */
		snprintf(str, size, "mul R%d,R%d,R%d", d, a, b);
		break;
	case 3: /* div */
		snprintf(str, size, "div R%d,R%d,R%d", d, a, b);
		break;
	case 4: /* cmp */
		snprintf(str, size, "cmp R%d,R%d", a, b);
		break;
	case 5: /* cmplt */
		snprintf(str, size, "cmplt R%d,R%d,R%d", d, a, b);
		break;
	case 6: /* cmpeq */
		snprintf(str, size, "cmpeq R%d,R%d,R%d", d, a, b);
		break;
	case 7: /* cmpgt */
		snprintf(str, size, "cmpgt R%d,R%d,R%d", d, a, b);
		break;
	case 8: /* inv */
		snprintf(str, size, "inv R%d,R%d", d, a);
		break;
	case 9: /* and */
		snprintf(str, size, "and R%d,R%d,R%d", d, a, b);
		break;
	case 0xa: /* or */
		snprintf(str, size, "or R%d,R%d,R%d", d, a, b);
		break;
	case 0xb: /* xor */
		snprintf(str, size, "xor R%d,R%d,R%d", d, a, b);
		break;
	case 0xc: /* addc */
		snprintf(str, size, "addc R%d,R%d,R%d", d, a, b);
		break;
	case 0xd: /* trap */
		snprintf(str, size, "trap R%d,R%d,R%d", d, a, b);
		break;
	case 0xe: /* EXP format, current unused */
		break;
	case 0xf: /* RX format */
		++mem;
		if (!symtab || !(adrsym = htab_ui16_get(symtab, *mem))) {
			snprintf(adrbuf, sizeof(adrbuf), "%04x", *mem);
			adrsym = adrbuf;
		}

		switch (b) {
		case 0: /* lea */
			snprintf(str, size, "lea R%d,%s[R%d]", d, adrsym, a);
			break;
		case 1: /* load */
			snprintf(str, size, "load R%d,%s[R%d]", d, adrsym, a);
			break;
		case 2: /* store */
			snprintf(str, size, "store R%d,%s[R%d]", d, adrsym, a);
			break;
		case 3: /* jump */
			snprintf(str, size, "jump R%d,%s[R%d]", d, adrsym, a);
			break;
		case 4: /* jumpc0 */
			snprintf(str, size, "jumpc0 R%d,%s[R%d]", d, adrsym, a);
			break;
		case 5: /* jumpc1 */
			snprintf(str, size, "jumpc1 R%d,%s[R%d]", d, adrsym, a);
			break;
		case 6: /* jumpf */
			snprintf(str, size, "jumpf R%d,%s[R%d]", d, adrsym, a);
			break;
		case 7: /* jumpt */
			snprintf(str, size, "jumpt R%d,%s[R%d]", d, adrsym, a);
			break;
		case 8: /* jal */
			snprintf(str, size, "jal R%d,%s[R%d]", d, adrsym, a);
			break;
		}
		break;
	}

	return mem;
}
