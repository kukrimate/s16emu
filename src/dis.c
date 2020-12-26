/*
 * Disassembler frontend
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <map.h>
#include "lib/disasm.h"

// Load big endian file into memory in native endianness
static void
load_full_file(FILE *fp, uint16_t **data, size_t *size)
{
    uint16_t *ptr;
    unsigned char buf[2];

    // Get file size
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp) >> 1;
    fseek(fp, 0, SEEK_SET);

    // Allocate buffer
    ptr = *data = malloc(*size << 1); // NOTE: size * 2
    if (!ptr)
        abort();

    while (fread(buf, sizeof buf, 1, fp) > 0) {
        /* Load big endian words from the file in native endianness */
        *ptr++ = buf[0] << 8 | buf[1];
    }
}

int
main(int argc, char *argv[])
{
    FILE *fp;
    uint16_t *data, *ptr;
    size_t words;
    char buf[4096];

    // Make sure we got a filename argument
    if (argc < 2) {
        fprintf(stderr, "%s BINFILE\n", argv[0]);
        return 1;
    }

    // Open I/O handle on the provided path
    if (!(fp = fopen(argv[1], "r"))) {
        perror(argv[1]);
        return 1;
    }

    // Load file into memory
    load_full_file(fp, &data, &words);
    fclose(fp);

    // Disassemble all instructions
    for (ptr = data; ptr < data + words; ++ptr) {
        ptr = disassemble(buf, sizeof buf, ptr, NULL);
        printf("%s\n", buf);
    }

    // Free buffer and exit
    free(data);
    return 0;
}
