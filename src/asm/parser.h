#ifndef PARSER_H
#define PARSER_H

enum s16_parse_type {
	ROOT, /* Root of the AST */
	LABEL,
	OPCODE,
	OPERAND_LABEL,
	OPERAND_CONSTANT,
	OPERAND_REGISTER,
	OPERAND_EADDRESS,
	OPERAND_STRING_LITERAL,
};

struct s16_parse_token {
	/* Type of token */
	enum s16_parse_type type;
	/* Token data (optional) */
	union s16_lex_data data;
	/* Child count */
	size_t child_cnt;
	/* Children */
	struct s16_parse_token **children;
};

/*
 * Generate an AST out of a list of lexer tokens
 */
struct s16_parse_token *genast(struct s16_lex_token *tok, long *line);

/*
 * Free an AST
 */
void freeast(struct s16_parse_token *root);

#endif
