#ifndef ALU_H
#define ALU_H

#define BIT_ccG 0 /* Unsigned greater than */
#define BIT_ccg 1 /* Signed greater than */
#define BIT_ccE 2 /* Equal to */
#define BIT_ccl 3 /* Signed less than */
#define BIT_ccL 4 /* Unsigned less than */
#define BIT_ccV 5 /* Unsigned overflow */
#define BIT_ccv 6 /* Signed overflow */
#define BIT_ccC 7 /* Carry prop */

#define SET_BIT(x, bit, val) \
	if (val) { x |= (0x8000 >> bit); } else { x &= ~(0x8000 >> bit); }

#define GET_BIT(x, bit) \
	((x & (0x8000 >> bit)) > 0)

void s16add(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b);
void s16sub(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b);
void s16mul(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b);
void s16div(uint16_t *q, uint16_t *r, uint16_t a, uint16_t b);
void s16cmp(uint16_t *f, uint16_t a, uint16_t b);
void s16cmplt(uint16_t *d, uint16_t a, uint16_t b);
void s16cmpgt(uint16_t *d, uint16_t a, uint16_t b);
void s16addc(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b);

#endif
