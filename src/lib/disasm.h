#ifndef DISASM_H
#define DISASM_H

/* Reverse symbol table type */
MAP_GEN(uint16_t, char *, IHASH, ICOMPARE, rsym)

/*
 * Disassemble one instruction
 * Returns a pointer to the next instruction
 */
uint16_t *
disassemble(char *str, size_t size, uint16_t *mem, rsymmap *rsymtab);

#endif
