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
		outfd = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
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
