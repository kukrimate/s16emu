/*
 * Flexible Sigma16 assembler internal definitions
 */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))


/*
 * Instruction encoder
 */
void assemble(struct s16_token *head, int outfd);
