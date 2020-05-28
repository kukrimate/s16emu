/*
 * Tokenizer for s16asm
 */

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dynarr.h"

#include "internal.h"

#include <stdio.h>

/*
 * NOTE:
 *  There is a bit of triple pointer magic here, but basically all it does is
 *  allow us to change the pointer to the target next pointer on the tokenize
 *  function's stack
 */
static void append_token
	(struct s16_token ***next, enum s16_token_type type, dynarr *tmp)
{
	**next = malloc(sizeof(struct s16_token));
	(**next)->type = type;
	(**next)->data = strndup(tmp->buffer, tmp->elem_cnt);
	tmp->elem_cnt = 0;
	(**next)->next = NULL;

	printf("%d:%s\n", (**next)->type, (**next)->data);

	*next = &(**next)->next;
}

static void nextchar(char **string, size_t *length)
{
	++*string;
	--*length;
}

#define INDEX_FIRST_LEAD    0
#define INDEX_FIRST_DATA    1
#define INDEX_FIRST_TRAIL   2

#define INDEX_OPCODE_LEAD   3
#define INDEX_OPCODE_DATA   4
#define INDEX_OPCODE_TRAIL  5

#define INDEX_OPERAND_LEAD  6
#define INDEX_OPERAND_DATA  7
#define INDEX_OPERAND_TRAIL 8

#define INDEX_COMMENT       9

static ssize_t tok_first_lead
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case ';':
		nextchar(string, length);
		return INDEX_COMMENT;
	case '\n':
		nextchar(string, length);
		return INDEX_FIRST_LEAD;
	case '\r':
	case '\t':
	case ' ':
		nextchar(string, length);
		return INDEX_FIRST_LEAD;
	default:
		return INDEX_FIRST_DATA;
	}
}

static ssize_t tok_first_data
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case ';':
	case '\n':
	case ':':
	case '\r':
	case '\t':
	case ' ':
		return INDEX_FIRST_TRAIL;
	default:
		dynarr_addc(tmp, **string);
		nextchar(string, length);
		return INDEX_FIRST_DATA;
	}
}

static ssize_t tok_first_trail
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case ';':
		nextchar(string, length);
		append_token(next, OPCODE, tmp);
		return INDEX_COMMENT;
	case '\n':
		nextchar(string, length);
		append_token(next, OPCODE, tmp);
		return INDEX_FIRST_LEAD;
	case ':':
		nextchar(string, length);
		append_token(next, LABEL, tmp);
		return INDEX_OPCODE_LEAD;
	case '\r':
	case '\t':
	case ' ':
		nextchar(string, length);
		return INDEX_FIRST_TRAIL;
	default:
		append_token(next, OPCODE, tmp);
		return INDEX_OPERAND_LEAD;
	}
}

static ssize_t tok_opcode_lead
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case ';':
		nextchar(string, length);
		return INDEX_COMMENT;
	case '\n':
		nextchar(string, length);
		return INDEX_FIRST_LEAD;
	case '\r':
	case '\t':
	case ' ':
		nextchar(string, length);
		return INDEX_OPCODE_LEAD;
	default:
		return INDEX_OPCODE_DATA;
	}
}

static ssize_t tok_opcode_data
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case ';':
	case '\n':
	case '\r':
	case '\t':
	case ' ':
		return INDEX_OPCODE_TRAIL;
	default:
		dynarr_addc(tmp, **string);
		nextchar(string, length);
		return INDEX_OPCODE_DATA;
	}
}

static ssize_t tok_opcode_trail
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case ';':
		nextchar(string, length);
		append_token(next, OPCODE, tmp);
		return INDEX_COMMENT;
	case '\n':
		nextchar(string, length);
		append_token(next, OPCODE, tmp);
		return INDEX_FIRST_LEAD;
	case '\r':
	case '\t':
	case ' ':
		nextchar(string, length);
		return INDEX_OPCODE_TRAIL;
	default:
		append_token(next, OPCODE, tmp);
		return INDEX_OPERAND_LEAD;
	}
}

static ssize_t tok_operand_lead
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case ';':
		nextchar(string, length);
		return INDEX_COMMENT;
	case '\n':
		nextchar(string, length);
		return INDEX_FIRST_LEAD;
	case '\r':
	case '\t':
	case ' ':
		nextchar(string, length);
		return INDEX_OPERAND_LEAD;
	default:
		return INDEX_OPERAND_DATA;
	}
}

static ssize_t tok_operand_data
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case ';':
	case '\n':
	case ',':
	case '\r':
	case '\t':
	case ' ':
		return INDEX_OPERAND_TRAIL;
	default:
		dynarr_addc(tmp, **string);
		nextchar(string, length);
		return INDEX_OPERAND_DATA;
	}
}

static ssize_t tok_operand_trail
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case ';':
		nextchar(string, length);
		append_token(next, OPERAND, tmp);
		return INDEX_COMMENT;
	case '\n':
		nextchar(string, length);
		append_token(next, OPERAND, tmp);
		return INDEX_FIRST_LEAD;
	case ',':
		nextchar(string, length);
		append_token(next, OPERAND, tmp);
		return INDEX_OPERAND_LEAD;
	case '\r':
	case '\t':
	case ' ':
		nextchar(string, length);
		return INDEX_OPERAND_TRAIL;
	default: /* Can't have space inside operand */
		return -1;
	}
}

static ssize_t tok_comment
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length)
{
	switch (**string) {
	case '\n':
		nextchar(string, length);
		return INDEX_FIRST_LEAD;
	default:
		nextchar(string, length);
		return INDEX_COMMENT;
	}
}

typedef ssize_t (*tok_fun)
	(struct s16_token ***next, dynarr *tmp, char **string, size_t *length);

static tok_fun tok_table[] = {
	tok_first_lead,
	tok_first_data,
	tok_first_trail,
	tok_opcode_lead,
	tok_opcode_data,
	tok_opcode_trail,
	tok_operand_lead,
	tok_operand_data,
	tok_operand_trail,
	tok_comment
};

struct s16_token *tokenize(char *string, size_t length)
{
	struct s16_token *head, **next;
	ssize_t tok_index;
	dynarr tmp;

	head = NULL;
	next = &head; /* First "next" pointer is written to the head itself */

	tok_index = INDEX_FIRST_LEAD;
	dynarr_alloc(&tmp, sizeof(char));

	while (length) {
		tok_index = tok_table[tok_index](&next, &tmp, &string, &length);
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
