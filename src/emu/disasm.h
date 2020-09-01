#ifndef DISASM_H
#define DISASM_H

/*
 * Disassemble one instruction
 * Returns a pointer to the next instruction
 */
uint16_t *
disassemble(char *str, size_t size, uint16_t *mem, htab_ui16 *symtab);

#endif
