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
		if (*p == '\t' || *p == ' ')
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

#define INDEX_LABEL 0
#define INDEX_OPCODE 1
#define INDEX_OPERAND 2
#define INDEX_COMMENT 3

static ssize_t tok_label
	(struct s16_token ***next, dynarr *tmp, char last, char cur)
{
	switch (cur) {
	case ';':
		append_token(next, LABEL, tmp);
		return INDEX_COMMENT;
	case '\n':
		append_token(next, LABEL, tmp);
		return INDEX_LABEL;
	case ':':
		append_token(next, LABEL, tmp);
		return INDEX_OPCODE;
	case '\t':
	case ' ':
		if (!tmp->elem_cnt)
			return INDEX_LABEL;
		/* Has to end with : to be a label -> this was actually an opcode */
		append_token(next, OPCODE, tmp);
		return INDEX_OPERAND;
	default:
		dynarr_addc(tmp, cur);
		return INDEX_LABEL;
	}
}

static ssize_t tok_opcode
	(struct s16_token ***next, dynarr *tmp, char last, char cur)
{
	switch (cur) {
	case ';':
		append_token(next, OPCODE, tmp);
		return INDEX_COMMENT;
	case '\n':
		append_token(next, OPCODE, tmp);
		return INDEX_LABEL;
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
	(struct s16_token ***next, dynarr *tmp, char last, char cur)
{
	switch (cur) {
	case ';':
		append_token(next, OPERAND, tmp);
		return INDEX_COMMENT;
	case '\n':
		append_token(next, OPERAND, tmp);
		return INDEX_LABEL;
	case ',':
		append_token(next, OPERAND, tmp);
		return INDEX_OPERAND;
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
	(struct s16_token ***next, dynarr *tmp, char last, char cur)
{
	switch (cur) {
	case '\n':
		// append_token(next, COMMENT, tmp);
		tmp->elem_cnt = 0;
		return INDEX_LABEL;
	default:
		dynarr_addc(tmp, cur);
		return INDEX_COMMENT;
	}
}

typedef ssize_t (*tok_fun)
	(struct s16_token ***next, dynarr *tmp, char last, char cur);

static tok_fun tok_table[] = {
	tok_label,
	tok_opcode,
	tok_operand,
	tok_comment
};

struct s16_token *tokenize(char *string, size_t length)
{
	struct s16_token *head, **next;
	ssize_t tok_index;

	dynarr tmp;
	char last, *ptr;

	head = NULL;
	next = &head; /* First "next" pointer is written to the head itself */
	tok_index = INDEX_LABEL;

	dynarr_alloc(&tmp, sizeof(char));
	last = 0;

	for (ptr = string; ptr < string + length; ++ptr) {
		if ('\r' == *ptr)
			continue;

		tok_index = tok_table[tok_index](&next, &tmp, last, *ptr);
		if (-1 == tok_index)
			goto err;

		last = *ptr;
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
