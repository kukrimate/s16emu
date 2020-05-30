#ifndef LEXER_H
#define LEXER_H

enum s16_token_type {
	IDENTIFIER,     /* Label, opcode or register */
	PUNCTUATOR,     /* Any of : , + - [ ] LF */
	CONSTANT,       /* Numeric or character constant */
	STRING_LITERAL, /* String literal */
};

union s16_token_data {
	char *str;     /* IDENTIFIER and STRING_LITERAL */
	char ch;       /* PUCTUATOR */
	long constant; /* CONSTANT */
};

struct s16_token {
	enum s16_token_type type;
	union s16_token_data data;
	struct s16_token *next;
};

struct s16_token *tokenize(char *str, long *line);
void free_tokens(struct s16_token *head);

#endif
