#ifndef LEXER_H
#define LEXER_H

enum s16_lex_type {
	IDENTIFIER,     /* Label or opcode */
	PUNCTUATOR,     /* Any of : , [ ] LF */
	REGISTER,       /* Register */
	CONSTANT,       /* Numeric or character constant */
	STRING_LITERAL, /* String literal */
};

union s16_lex_data {
	char *s; /* IDENTIFIER and STRING_LITERAL */
	char  c; /* PUCTUATOR */
	long  l; /* REGISTER and CONSTANT */
};

#define datanull    ((union s16_lex_data) { .s = NULL })
#define datastr(x)  ((union s16_lex_data) { .s = x })
#define datachar(x) ((union s16_lex_data) { .c = x })
#define datalong(x) ((union s16_lex_data) { .l = x })

struct s16_lex_token {
	enum s16_lex_type type;
	union s16_lex_data data;
	struct s16_lex_token *next;
};

#define IS_IDENTIFIER(tok) \
	(tok && IDENTIFIER == (tok)->type)
#define IS_REGISTER(tok) \
	(tok && REGISTER == (tok)->type)
#define IS_CONSTANT(tok) \
	(tok && CONSTANT == (tok)->type)
#define IS_STRING_LITERAL(tok) \
	(tok && STRING_LITERAL == (tok)->type)
#define IS_PUNCTUATOR(tok, x) \
	(tok && PUNCTUATOR == (tok)->type && x == (tok)->data.c)

/*
 * Tokenize a NUL-terminted input string
 *  On error NULL is returned and *line is set to the offending line
 */
struct s16_lex_token *tokenize(char *str, long *line);

/*
 * Free a linked list of tokens
 */
void freetokens(struct s16_lex_token *head);

#endif
