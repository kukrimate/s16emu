/*
 * Tokenizer for s16asm
 */

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dynarr.h"

#include "internal.h"

/* Remove trailing whitespaces from tmp */
static void detrail_token(dynarr *tmp)
{
	char *p;

	p = (char *) tmp->buffer + tmp->elem_cnt;
	while ((char *) tmp->buffer <= --p)
		if (*p == '\r' || *p == '\t' || *p == ' ')
			--tmp->elem_cnt;
}

/*
 * NOTE:
 *  There is a bit of triple pointer magic here, but basically all it does is
 *  allow us to change the pointer to the target next pointer on the tokenize
 *  function's stack
 */
static void append_token(struct s16_token ***next,
	enum s16_token_type type, dynarr *tmp)
{
	detrail_token(tmp);
	if (!tmp->elem_cnt)
		return;

	**next = malloc(sizeof(struct s16_token));
	(**next)->type = type;
	(**next)->data = strndup(tmp->buffer, tmp->elem_cnt);
	tmp->elem_cnt = 0;
	(**next)->next = NULL;

	*next = &(**next)->next;
}

#define INDEX_FIRST 0
#define INDEX_OPCODE 1
#define INDEX_OPERAND 2
#define INDEX_COMMENT 3

static ssize_t tok_first
	(struct s16_token ***next, dynarr *tmp, char cur)
{
	switch (cur) {
	case ';':
		append_token(next, OPCODE, tmp);
		return INDEX_COMMENT;
	case '\n':
		append_token(next, OPCODE, tmp);
		return INDEX_FIRST;
	case ':':
		append_token(next, LABEL, tmp);
		return INDEX_OPCODE;
	case '\r':
	case '\t':
	case ' ':
		if (!tmp->elem_cnt)
			return INDEX_FIRST;
		append_token(next, OPCODE, tmp);
		return INDEX_OPERAND;
	default:
		dynarr_addc(tmp, cur);
		return INDEX_FIRST;
	}
}

static ssize_t tok_opcode
	(struct s16_token ***next, dynarr *tmp, char cur)
{
	switch (cur) {
	case ';':
		append_token(next, OPCODE, tmp);
		return INDEX_COMMENT;
	case '\n':
		append_token(next, OPCODE, tmp);
		return INDEX_FIRST;
	case '\r':
	case '\t':
	case ' ':
		if (!tmp->elem_cnt)
			return INDEX_OPCODE;
		append_token(next, OPCODE, tmp);
		return INDEX_OPERAND;
	default:
		dynarr_addc(tmp, cur);
		return INDEX_OPCODE;
	}
}

static ssize_t tok_operand
	(struct s16_token ***next, dynarr *tmp, char cur)
{
	switch (cur) {
	case ';':
		append_token(next, OPERAND, tmp);
		return INDEX_COMMENT;
	case '\n':
		append_token(next, OPERAND, tmp);
		return INDEX_FIRST;
	case ',':
		append_token(next, OPERAND, tmp);
		return INDEX_OPERAND;
	case '\r':
	case '\t':
	case ' ':
		if (!tmp->elem_cnt)
			return INDEX_OPERAND;
	default:
		dynarr_addc(tmp, cur);
		return INDEX_OPERAND;
	}
}

static ssize_t tok_comment
	(struct s16_token ***next, dynarr *tmp, char cur)
{
	switch (cur) {
	case '\n':
		return INDEX_FIRST;
	default:
		return INDEX_COMMENT;
	}
}

typedef ssize_t (*tok_fun)
	(struct s16_token ***next, dynarr *tmp, char cur);

static tok_fun tok_table[] = {
	tok_first,
	tok_opcode,
	tok_operand,
	tok_comment
};

struct s16_token *tokenize(char *string, size_t length)
{
	struct s16_token *head, **next;
	ssize_t tok_index;

	dynarr tmp;

	head = NULL;
	next = &head; /* First "next" pointer is written to the head itself */
	tok_index = INDEX_FIRST;

	dynarr_alloc(&tmp, sizeof(char));

	while (length--) {
		tok_index = tok_table[tok_index](&next, &tmp, *string++);
		if (-1 == tok_index)
			goto err;
	}

	dynarr_free(&tmp);
	return head;
err:
	dynarr_free(&tmp);
	free_tokens(head);
	return NULL;
}

void free_tokens(struct s16_token *head)
{
	struct s16_token *tmp;

	while (head) {
		tmp = head->next;
		free(head->data);
		free(head);
		head = tmp;
	}
}
