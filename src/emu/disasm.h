#ifndef DISASM_H
#define DISASM_H

/* Reverse symbol table type */
map_gen(uint16_t, char *, shash, scmp, rsym)

/*
 * Disassemble one instruction
 * Returns a pointer to the next instruction
 */
uint16_t *
disassemble(char *str, size_t size, uint16_t *mem, struct rsymmap *rsymtab);

#endif
