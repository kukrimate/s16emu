/*
 * Flexible Sigma16 assembler internal definitions
 */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

/* Maximum number of operands */
#define S16_MAX_OPERANDS 3

/* Assemble callback */
typedef void (*s16_write)(char *str, dynarr *buf, htab *symtab);

struct s16_opdef {
	/* Opcode */
	char *opcode;
	/* Encoded length in words */
	size_t length;
	/* Write base instruction */
	s16_write write_base;
	/* Number of operands */
	size_t operands;
	/* Write operands */
	s16_write write_operands[S16_MAX_OPERANDS];
};

struct s16_opdef *lookup_opdef(char *opcode);
