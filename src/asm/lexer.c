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
	long result, tmp1, tmp2;

	result = 0;

	/* Find the last valid digit */
	for (ptr = str; *ptr; ++ptr) {
		tmp1 = getdigit(*ptr);
		if (-1 == tmp1 || tmp1 >= base)
			break;
	}

	/* Store endptr if requested */
	if (endptr)
		*endptr = ptr;

	/* Calculate conversion */
	tmp1 = 1;
	while (--ptr >= str) {
		tmp2 = result + getdigit(*ptr) * tmp1;
		if (tmp2 < result) /* Check for overflow */
			return -1;
		result = tmp2;
		tmp1 *= base;
	}

	return result;
}

static long getconst(char *str, char **endptr)
{
	int base;

	if ('0' == *str)
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
	else
		base = 10;

	return getlong(str, endptr, base);
}

static int getescape(char *str, char **endptr)
{
	char ch;
	long tmp;

	if ('\\' != *str)
		return -1;

	/* Support for most common C escape sequences */
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

	while (*str) {
		switch (*str) {
		case '"':
			++str;
			goto end;
		case '\\':
			ch = getescape(str, &str);
			break;
		default:
			ch = *str++;
		}

		if (-1 == ch)
			break;

		dynarr_addc(&tmp, ch);
	}

	dynarr_free(&tmp);
	return NULL;

end:
	if (endptr)
		*endptr = str;

	dynarr_addc(&tmp, 0); /* NUL-terminate buffer */
	return tmp.buffer;
}

static union s16_token_data dataconsant(long constant)
{
	return (union s16_token_data) { .constant = constant };
}

static union s16_token_data datachar(char ch)
{
	return (union s16_token_data) { .ch = ch };
}

static union s16_token_data datastr(char *str)
{
	return (union s16_token_data) { .str = str };
}

static void append_token(
	struct s16_token ***next,
	enum s16_token_type type,
	union s16_token_data data)
{
	**next = malloc(sizeof(struct s16_token));
	(**next)->type = type;
	(**next)->data = data;
	(**next)->next = NULL;

	*next = &(**next)->next;
}

struct s16_token *tokenize(char *str, long *line)
{
	struct s16_token *head, **next;
	int tmp;
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
		case '+':
		case '-':
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
			switch (*++str) {
			case '\\':
				tmp = getescape(str, &str);
				break;
			default:
				tmp = *str++;
			}

			if (-1 == tmp || '\'' != *str++)
				goto err;

			append_token(&next, CONSTANT, dataconsant(tmp));
			break;

		/* String literal */
		case '\"':
			stmp = getstrliteral(str, &str);
			if (!stmp)
				goto err;
			append_token(&next, STRING_LITERAL, datastr(stmp));
			break;

		default:
			/* Identifier */
			if ('_' == *str || isalpha(*str))
				append_token(&next, IDENTIFIER,
					datastr(getidentifier(str, &str)));

			/* Constant */
			else if (isdigit(*str))
				append_token(&next, CONSTANT,
					dataconsant(getconst(str, &str)));

			/* Invalid token */
			else
				goto err;
		}

	return head;

err:
	free_tokens(head);
	return NULL;
}

void free_tokens(struct s16_token *head)
{
	struct s16_token *tmp;

	while (head) {
		tmp = head->next;
		if (IDENTIFIER == head->type
				|| STRING_LITERAL == head->type)
			free(head->data.str);
		free(head);
		head = tmp;
	}
}
