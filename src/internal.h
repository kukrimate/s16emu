/*
 * Emulator context
 */

#define REG_COUNT 16
#define RAM_WORDS 0x10000 /* 64K words */

struct s16emu {
	/* Decode registers */
	uint16_t pc, ir, adr;
	/* General purpose registers */
	uint16_t reg[REG_COUNT];
	/* RAM */
	uint16_t ram[RAM_WORDS];
};

/*
 * Instruction decoding macros
 */

#define INSN_OP(insn) (insn >> 12 & 0xf)
#define INSN_RD(insn) (insn >> 8 & 0xf)
#define INSN_RA(insn) (insn >> 4 & 0xf)
#define INSN_RB(insn) (insn & 0xf)

/*
 * Flags stored in R15
 */

#define BIT_ccG 0 /* Unsigned greater than */
#define BIT_ccg 1 /* Signed greater than */
#define BIT_ccE 2 /* Equal to */
#define BIT_ccl 3 /* Signed less than */
#define BIT_ccL 4 /* Unsigned less than */
#define BIT_ccV 5 /* Unsigned overflow */
#define BIT_ccv 6 /* Signed overflow */
#define BIT_ccC 7 /* Carry prop */

#define SET_BIT(x, bit, val) \
	if (val) { x |= (0x8000 >> bit); } else { x &= ~(0x8000 >> bit); }

#define GET_BIT(x, bit) \
	((x & (0x8000 >> bit)) > 0)

/*
 * Two's complement arithmetic emulator
 */

void s16add(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b);
void s16sub(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b);
void s16mul(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b);
void s16div(uint16_t *q, uint16_t *r, uint16_t a, uint16_t b);
void s16cmp(uint16_t *f, uint16_t a, uint16_t b);
void s16cmplt(uint16_t *d, uint16_t a, uint16_t b);
void s16cmpgt(uint16_t *d, uint16_t a, uint16_t b);
