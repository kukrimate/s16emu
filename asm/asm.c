/*
 * Flexible Sigma16 assembler, if you want to assemble a Sigma16 file from
 *  the CS1S course at UofG you are probably looking for "tools/stdasm.py"
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "dynarr.h"
#include "htab.h"

#include "internal.h"

enum s16_token_type {
	LABEL,   /* Start of line, ends with ':' */
	OPCODE,  /* Start of line or after a label, ends with a ' ', '\t' or EOL */
	OPERAND, /* After an opcode, ends with ',' or EOL */
	COMMENT  /* Starts with ';', ends with EOL */
};

/*
 * The "tokenize" function creates a linked-list of these tokens
 */

struct s16_token {
	enum s16_token_type type;
	char *data;
	struct s16_token *next;
};

/*
 * NOTE:
 *  There is a bit of triple pointer magic here, but basically all it does is
 *  allow us to change the pointer to the target next pointer on the tokenize
 *  function's stack
 */
void append_token(struct s16_token ***next,
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

struct s16_token *tokenize(char *string, size_t length)
{
	struct s16_token *head, **next;
	enum s16_token_type type;
	char *ptr;
	dynarr tmp;

	head = NULL;
	next = &head; /* First "next" pointer is written to the head itself */

	type = LABEL;
	ptr = string;

	dynarr_alloc(&tmp, sizeof(char));

	for (; ptr < string + length; ++ptr)
		switch (*ptr) {
		case ';':
			type = COMMENT;
			break;
		case ':':
			if (LABEL == type) {
				append_token(&next, type, &tmp);
				type = OPCODE;
			}
			break;
		case ' ':
		case '\t':
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
			if (OPERAND == type)
				append_token(&next, type, &tmp);
			break;
		case '\n':
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

void assemble(struct s16_token *head, int outfd)
{
	uint16_t address;
	htab labels;
	dynarr code;
	struct s16_token *cur;
	struct s16_opdef *opdef;
	size_t i;
	uint8_t tmp;

	address = 0;
	htab_new(&labels, 32);
	dynarr_alloc(&code, sizeof(uint16_t));

	/* First stage */
	for (cur = head; cur; cur = cur->next)
		switch (cur->type) {
		case LABEL:
			htab_put(&labels, cur->data, address, 0);
			break;
		case OPCODE:
			opdef = lookup_opdef(cur->data);
			if (!opdef) {
				fprintf(stderr, "Unkonwn instruction %s\n", cur->data);
				goto done;
			}
			address += opdef->length;
			break;
		default:
			break;
		}

	/* Second stage */
	for (cur = head; cur; cur = cur->next)
		switch (cur->type) {
		case OPCODE:
			opdef = lookup_opdef(cur->data);
			opdef->write_base(cur->data, &code, &labels);
			for (i = 0; i < opdef->operands; ++i) {
				cur = cur->next;
				opdef->write_operands[i](cur->data, &code, &labels);
			}
			break;
		case OPERAND:
			fprintf(stderr, "Unexpected operand %s\n", cur->data);
			goto done;
		default:
			break;
		}

	for (i = 0; i < code.elem_cnt; ++i) {
		tmp = dynarr_getw(&code, i) >> 8;
		write(outfd, &tmp, 1);
		tmp = dynarr_getw(&code, i);
		write(outfd, &tmp, 1);
	}

done:
	htab_del(&labels, 0);
	dynarr_free(&code);
}

int main(int argc, char *argv[])
{
	int opt, infd, outfd;
	char *infile, *outfile;

	char *str;
	size_t len;

	struct s16_token *head;

	infile = NULL;
	outfile = NULL;

	/* Parse arguments */
	while (-1 != (opt = getopt(argc, argv, "ho:")))
		switch (opt) {
		case 'o':
			outfile = optarg;
			break;
		default:
		case 'h':
			goto print_usage;
		}

	if (optind >= argc)
		goto print_usage;

	infile = argv[optind];

	/* Open input file */
	infd = open(infile, O_RDONLY);
	if (-1 == infd) {
		perror(infile);
		return 1;
	}
	len = lseek(infd, 0, SEEK_END);
	str = mmap(NULL, len, PROT_READ, MAP_PRIVATE, infd, 0);

	/* Open output file */
	if (outfile) {
		outfd = open(outfile, O_RDWR);
		if (-1 == outfd) {
			perror(outfile);
			close(infd);
			return 1;
		}
	} else {
		outfd = STDOUT_FILENO;
	}

	/* Tokenize and assemble */
	head = tokenize(str, len);
	assemble(head, outfd);

	free_tokens(head);
	close(infd);
	return 0;

print_usage:
	fprintf(stderr, "Usage %s [-o OUT] FILE\n", argv[0]);
	return 1;
}
