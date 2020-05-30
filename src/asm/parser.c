/*
 * Syntactical analyzer
 */

#include <stdint.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"

#include <stdio.h>

static struct s16_parse_token *allocroot()
{
	struct s16_parse_token *token;

	token = malloc(sizeof(struct s16_parse_token));
	token->type = ROOT;
	token->child_cnt = 0;
	token->children = NULL;
	return token;
}

static struct s16_parse_token *alloctoken
	(enum s16_parse_type type, union s16_lex_data data)
{
	struct s16_parse_token *token;

	token = malloc(sizeof(struct s16_parse_token));
	token->type = type;
	token->data = data;
	token->child_cnt = 0;
	token->children = NULL;
	return token;
}

static struct s16_parse_token *appendchild
	(struct s16_parse_token *parent, enum s16_parse_type type,
		union s16_lex_data data)
{
	parent->children = reallocarray
		(parent->children, ++parent->child_cnt, sizeof(parent->children[0]));
	parent->children[parent->child_cnt - 1] = alloctoken(type, data);
	return parent->children[parent->child_cnt - 1];
}

struct s16_parse_token *genast(struct s16_lex_token *tok, long *line)
{
	struct s16_parse_token *root, *opcode, *eaddress;

	root = allocroot();
	*line = 1;

	while (tok) {
		/* Parse label if any */
		if (IS_IDENTIFIER(tok) && IS_PUNCTUATOR(tok->next, ':')) {
			appendchild(root, LABEL, tok->data);
			tok = tok->next->next;
		}

		/* Opcode comes next */
		if (IS_IDENTIFIER(tok)) {
			opcode = appendchild(root, OPCODE, tok->data);
			tok = tok->next;

			while (!IS_PUNCTUATOR(tok, '\n')) {
				switch (tok->type) {
				case IDENTIFIER:
					if (IS_PUNCTUATOR(tok->next, '[')) {
						eaddress = appendchild(opcode, OPERAND_EADDRESS, datanull);
						appendchild(eaddress, OPERAND_LABEL, tok->data);
						tok = tok->next->next;
						if (!IS_REGISTER(tok) || !IS_PUNCTUATOR(tok->next, ']'))
							goto err;
						appendchild(eaddress, OPERAND_REGISTER, tok->data);
						tok = tok->next->next;
					} else {
						appendchild(opcode, OPERAND_LABEL, tok->data);
						tok = tok->next;
					}
					break;
				case CONSTANT:
					if (IS_PUNCTUATOR(tok->next, '[')) {
						eaddress = appendchild(opcode, OPERAND_EADDRESS, datanull);
						appendchild(eaddress, OPERAND_CONSTANT, tok->data);
						tok = tok->next->next;
						if (!IS_REGISTER(tok) || !IS_PUNCTUATOR(tok->next, ']'))
							goto err;
						appendchild(eaddress, OPERAND_REGISTER, tok->data);
						tok = tok->next->next;
					} else {
						appendchild(opcode, OPERAND_CONSTANT, tok->data);
						tok = tok->next;
					}
					break;
				case REGISTER:
					appendchild(opcode, OPERAND_REGISTER, tok->data);
					tok = tok->next;
					break;
				case STRING_LITERAL:
					appendchild(opcode, OPERAND_STRING_LITERAL, tok->data);
					tok = tok->next;
					break;
				default:
					goto err;
				}

				if (IS_PUNCTUATOR(tok, ','))
					tok = tok->next;
				else if (!IS_PUNCTUATOR(tok, '\n'))
					goto err;
			}
		}

		/* Skip all newlines */
		if (IS_PUNCTUATOR(tok, '\n'))
			while (IS_PUNCTUATOR(tok, '\n')) {
				tok = tok->next;
				++*line;
			}
		else
			goto err;
	}

	return root;

err:
	freeast(root);
	return NULL;
}

void freeast(struct s16_parse_token *root)
{
	size_t i;

	for (i = 0; i < root->child_cnt; ++i) {
		freeast(root->children[i]);
	}

	if (root->children)
		free(root->children);
	free(root);
}
