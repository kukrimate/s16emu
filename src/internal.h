
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
 * TC math emulator
 */

void s16add(s16word *r, s16word a, s16word b);
void s16sub(s16word *r, s16word a, s16word b);
void s16mul(s16word *r, s16word a, s16word b);
void s16div(s16word *q, s16word *r, s16word a, s16word b);
