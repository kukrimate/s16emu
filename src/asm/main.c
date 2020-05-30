/*
 * Flexible Sigma16 assembler, if you want to assemble a Sigma16 file from
 *  the CS1S course at UofG you are probably looking for "tools/stdasm.py"
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "lexer.h"

static char *readfile(char *path)
{
	int fd;
	off_t off;
	char *str;
	ssize_t len;

	fd = open(path, O_RDONLY);
	if (-1 == fd)
		return NULL;

	if (-1 == (off = lseek(fd, 0, SEEK_END)) ||
			NULL == (str = malloc(off + 1)))
		goto err_close;

	if (-1 == (len = pread(fd, str, off, 0)))
		goto err_free;
	str[len] = 0;

	close(fd);
	return str;

err_free:
	free(str);
err_close:
	close(fd);
	return NULL;
}

int main(int argc, char *argv[])
{
	int opt, outfd;
	char *outfile;
	char *str;
	long line;
	struct s16_token *head;

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

	/* Open input file */
	str = readfile(argv[optind]);
	if (!str) {
		perror(argv[optind]);
		return 1;
	}

	/* Open output file */
	if (outfile) {
		outfd = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, 0644);
		if (-1 == outfd) {
			perror(outfile);
			return 1;
		}
	} else {
		outfd = STDOUT_FILENO;
	}

	/* Tokenize and assemble */
	head = tokenize(str, &line);
	if (!head) {
		fprintf(stderr, "Error on line %ld\n", line);
		close(outfd);
		return 1;
	}
	// assemble(head, outfd);
	free_tokens(head);
	return 0;

print_usage:
	fprintf(stderr, "Usage %s [-o OUT] FILE\n", argv[0]);
	return 1;
}
