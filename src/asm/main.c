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
	if (!head) {
		fprintf(stderr, "Invalid syntax!\n");
		close(infd);
		close(outfd);
		return 1;
	}
	assemble(head, outfd);

	free_tokens(head);
	close(infd);
	return 0;

print_usage:
	fprintf(stderr, "Usage %s [-o OUT] FILE\n", argv[0]);
	return 1;
}
