/*
 * Lexical analyzer
 */

#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "dynarr.h"
#include "lexer.h"

static char *registers[] = { \
	"0", "1", "2", "3", "4", "5", "6" , "7",
	"8", "9", "10", "11", "12", "13", "14", "15", NULL };

static _Bool isregister(char *str)
{
	char **reg;

	switch (*str++) {
	case 'r':
	case 'R':
		break;
	default:
		return 0;
	}

	for (reg = registers; *reg; ++reg) {
		if (!strncmp(*reg, str, strlen(*reg)))
			return 1;
	}

	return 0;
}

static char *getidentifier(char *str, char **endptr)
{
	char *ptr;

	for (ptr = str; *ptr; ++ptr)
		if ('_' != *ptr && !isalnum(*ptr))
			break;

	if (endptr)
		*endptr = ptr;

	return strndup(str, ptr - str);
}

static long getdigit(char digit)
{
	/* Decimal digits are always continous */
	if ('0' <= digit && digit <= '9')
		return digit - '0';

	/* Letters need not be continues, e.g. EBCDIC */
	switch (digit) {
	case 'a':
	case 'A':
		return 10;
	case 'b':
	case 'B':
		return 11;
	case 'c':
	case 'C':
		return 12;
	case 'd':
	case 'D':
		return 13;
	case 'e':
	case 'E':
		return 14;
	case 'f':
	case 'F':
		return 15;
	}

	return -1;
}

static long getlong(char *str, char **endptr, int base)
{
	char *ptr;
	long result, tmp;

	result = 0;

	/* Find the last valid digit */
	for (ptr = str; *ptr; ++ptr) {
		tmp = getdigit(*ptr);
		if (-1 == tmp || tmp >= base)
			break;
	}

	/* Store endptr if requested */
	if (endptr)
		*endptr = ptr;

	/* Calculate conversion */
	tmp = 1;
	while (--ptr >= str) {
		result += getdigit(*ptr) * tmp;
		tmp *= base;
	}

	return result;
}

static long getconst(char *str, char **endptr)
{
	int base;
	_Bool sign;

	base = 10; /* Base 10 is default */
	sign = 0;  /* Default to positive */

	if ('-' == *str) {
		sign = 1;
		++str;
	} else if ('+' == *str) {
		++str;
	} else if ('0' == *str) { /* Base prefix */
		switch (*++str) {
		case 'x':
		case 'X': /* Hexadecimal */
			++str;
			base = 16;
			break;
		case 'b':
		case 'B': /* Binary */
			++str;
			base = 2;
			break;
		case 'o':
		case 'O': /* New style octal */
			++str;
		default: /* Old style octal */
			base = 8;
			break;
		}
	}

	return sign ? -getlong(str, endptr, base) : getlong(str, endptr, base);
}

static int getcharacter(char *str, char **endptr)
{
	char ch;
	long tmp;

	if ('\\' != *str)
		ch = *str++;
	else
		switch (*++str) {
		case 'a':
			++str;
			ch = '\a';
			break;
		case 'b':
			++str;
			ch = '\b';
			break;
		case 'f':
			++str;
			ch = '\f';
			break;
		case 'n':
			++str;
			ch = '\n';
			break;
		case 'r':
			++str;
			ch = '\r';
			break;
		case 't':
			++str;
			ch = '\t';
			break;
		case 'v':
			++str;
			ch = '\v';
			break;
		case '\\':
		case '\'':
		case '\"':
		case '\?':
			ch = *str++;
			break;
		case 'x':
			++str;
			tmp = getlong(str, &str, 16);
			if (-1 == tmp || tmp > CHAR_MAX)
				return -1;
			ch = tmp;
			break;
		default:
			tmp = getlong(str, &str, 8);
			if (-1 == tmp || tmp > CHAR_MAX)
				return -1;
			ch = tmp;
			break;
		}

	if (endptr)
		*endptr = str;

	return ch;
}

static char *getstrliteral(char *str, char **endptr)
{
	dynarr tmp;
	int ch;

	if ('"' != *str++)
		return NULL;

	dynarr_alloc(&tmp, sizeof(char));

	while (*str && *str != '"') {
		ch = getcharacter(str, &str);
		if (-1 == ch)
			break;
		dynarr_addc(&tmp, ch);
	}

	if ('"' != *str++) {
		dynarr_free(&tmp);
		return NULL;
	}

	if (endptr)
		*endptr = str;

	dynarr_addc(&tmp, 0); /* NUL-terminate buffer */
	return tmp.buffer;
}

static void append_token(
	struct s16_lex_token ***next,
	enum s16_lex_type type,
	union s16_lex_data data)
{
	**next = malloc(sizeof(struct s16_lex_token));
	(**next)->type = type;
	(**next)->data = data;
	(**next)->next = NULL;

	*next = &(**next)->next;
}

struct s16_lex_token *tokenize(char *str, long *line)
{
	struct s16_lex_token *head, **next;
	long tmp;
	char *stmp;

	head = NULL;
	next = &head; /* First "next" pointer is written to the head itself */

	*line = 1;

	while (*str)
		switch (*str) {
		/* Whitespace */
		case ' ':
		case '\t':
		case '\v':
		case '\f':
			++str;
			break;

		/* Comment */
		case ';':
			while (*str && '\n' != *str)
				++str;
			break;

		/* Punctuators */
		case ':':
		case ',':
		case '[':
		case ']':
			append_token(&next, PUNCTUATOR, datachar(*str++));
			break;

		/* Newlines */
		case '\r':
			if ('\n' == *++str)
				++str;
			append_token(&next, PUNCTUATOR, datachar('\n'));
			++*line;
			break;
		case '\n':
			append_token(&next, PUNCTUATOR, datachar(*str++));
			++*line;
			break;

		/* Character constant */
		case '\'':
			tmp = getcharacter(str, &str);
			if (-1 == tmp || '\'' != *str++)
				goto err;
			append_token(&next, CONSTANT, datalong(tmp));
			break;

		/* String literal */
		case '\"':
			stmp = getstrliteral(str, &str);
			if (!stmp)
				goto err;
			append_token(&next, STRING_LITERAL, datastr(stmp));
			break;

		default:
			/* Register */
			if (isregister(str))
				append_token(&next, REGISTER,
					datalong(getlong(++str, &str, 10)));

			/* Identifier */
			else if ('_' == *str || isalpha(*str))
				append_token(&next, IDENTIFIER,
					datastr(getidentifier(str, &str)));

			/* Constant */
			else if ('-' == *str || '+' == *str || isdigit(*str))
				append_token(&next, CONSTANT,
					datalong(getconst(str, &str)));

			/* Invalid token */
			else
				goto err;
		}

	return head;

err:
	freetokens(head);
	return NULL;
}

void freetokens(struct s16_lex_token *head)
{
	struct s16_lex_token *tmp;

	while (head) {
		tmp = head->next;
		if (IDENTIFIER == head->type
				|| STRING_LITERAL == head->type)
			free(head->data.s);
		free(head);
		head = tmp;
	}
}
