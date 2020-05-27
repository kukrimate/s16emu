/*
 * Flexible Sigma16 assembler internal definitions
 */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

/*
 * Tokenizer
 */

enum s16_token_type {
	LABEL,   /* Start of line, ends with ':' */
	OPCODE,  /* Start of line or after a label, ends with a ' ', '\t' or EOL */
	OPERAND, /* After an opcode, ends with ',' or EOL */
	COMMENT  /* Starts with ';', ends with EOL */
};

/*
 * The "tokenize" function creates a linked-list of these tokens
 */

struct s16_token {
	enum s16_token_type type;
	char *data;
	struct s16_token *next;
};

struct s16_token *tokenize(char *string, size_t length);
void free_tokens(struct s16_token *head);

/*
 * Instruction encoder
 */
void assemble(struct s16_token *head, int outfd);
