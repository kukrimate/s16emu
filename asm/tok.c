/*
 * Tokenizer for s16asm
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dynarr.h"

#include "internal.h"

/*
 * NOTE:
 *  There is a bit of triple pointer magic here, but basically all it does is
 *  allow us to change the pointer to the target next pointer on the tokenize
 *  function's stack
 */
static void append_token(struct s16_token ***next,
	enum s16_token_type type, dynarr *tmp)
{
	if (!tmp->elem_cnt)
		return;

	**next = malloc(sizeof(struct s16_token));
	(**next)->type = type;
	(**next)->data = strndup(tmp->buffer, tmp->elem_cnt);
	tmp->elem_cnt = 0;
	(**next)->next = NULL;

	*next = &(**next)->next;
}

struct s16_token *tokenize(char *string, size_t length)
{
	struct s16_token *head, **next;
	enum s16_token_type type;
	char *ptr;
	_Bool literal;
	dynarr tmp;

	head = NULL;
	next = &head; /* First "next" pointer is written to the head itself */

	type = LABEL;
	ptr = string;
	literal = 0;

	dynarr_alloc(&tmp, sizeof(char));

	for (; ptr < string + length; ++ptr)
		switch (*ptr) {
		case '"':
			literal = !literal;
			dynarr_addc(&tmp, *ptr);
			break;
		case ';':
			if (literal) {
				dynarr_addc(&tmp, *ptr);
				break;
			}

			type = COMMENT;
			break;
		case ':':
			if (literal) {
				dynarr_addc(&tmp, *ptr);
				break;
			}

			if (LABEL == type) {
				append_token(&next, type, &tmp);
				type = OPCODE;
			}
			break;
		case ' ':
		case '\t':
			if (literal) {
				dynarr_addc(&tmp, *ptr);
				break;
			}

			/* Leading spaces can't cause state transitions */
			if (!tmp.elem_cnt)
				continue;

			/* Terminating ':' missing, LABEL means opcode here */
			if (LABEL == type || OPCODE == type) {
				append_token(&next, OPCODE, &tmp);
				type = OPERAND;
			}
			break;
		case ',':
			if (literal) {
				dynarr_addc(&tmp, *ptr);
				break;
			}

			if (OPERAND == type)
				append_token(&next, type, &tmp);
			break;
		case '\n':
			/* Pretend literal lasting to LF is fine,
		 		backend will diagnose it anyways */
			if (literal)
				literal = 0;

			/* Terminating ':' missing, LABEL means opcode here */
			if (LABEL == type || OPCODE == type)
				append_token(&next, OPCODE, &tmp);
			else
				append_token(&next, type, &tmp);
			type = LABEL;
			break;
		case '\r': /* Ignore CR to avoid dirty tokens from CRLF files */
			continue;
		default:
			dynarr_addc(&tmp, *ptr);
		}

	dynarr_free(&tmp);
	return head;
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
