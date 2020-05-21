
/*
 * Sigma16 word
 */
typedef uint16_t s16word;

#define S16_WORDSIZE sizeof(s16word)
#define S16_MAXBIT 0x8000

/*
 * Emulator context
 */

#define REG_COUNT 16
#define RAM_WORDS 0x10000 /* 64K words */

struct s16emu {
	/* Decode registers */
	s16word pc, ir, adr;
	/* General purpose registers */
	s16word reg[REG_COUNT];
	/* RAM */
	s16word ram[RAM_WORDS];
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

/*
 * TC math emulator
 */

void s16add(s16word *r, s16word a, s16word b);
void s16sub(s16word *r, s16word a, s16word b);
void s16mul(s16word *r, s16word a, s16word b);
void s16div(s16word *q, s16word *r, s16word a, s16word b);
