/*
 * Emulate two's complement arithmatic, relying only on C99 defined behaviour
 */

#include <stdint.h>
#include "internal.h"

#define S16_SIGN(x) (x & 0x8000)

/*
 * Convert a two's complement word into a signed integer
 */
static int32_t tosigint(uint16_t x)
{
	if (S16_SIGN(x)) {
		return -(int32_t) ((uint16_t) ~x + 1);
	} else {
		return (int32_t) x;
	}
}

/*
 * Convert a signed integer to two's complement word
 */
static uint16_t fromsigint(int32_t x)
{
	if (x < 0) {
		return (uint16_t) ~(uint16_t) -x + 1;
	} else {
		return (uint16_t) x;
	}
}

/*
 * Add two words
 */
void s16add(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b)
{
	*d = a + b;

	/* Unsigned overflow + carry propagation */
	SET_BIT(*f, BIT_ccV, *d < a || *d < b);
	SET_BIT(*f, BIT_ccC, *d < a || *d < b);

	/* Signed overflow */
	SET_BIT(*f, BIT_ccv,
		!(S16_SIGN(a) ^ S16_SIGN(b)) && (S16_SIGN(a) ^ S16_SIGN(*d)));
}

/*
 * Calculate "a - b" by computing the two's complement of b and adding
 */
void s16sub(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b)
{
	s16add(f, d, a, (uint16_t) ~b + 1);
}

/*
 * Calculate "a * b" treating both a and b as two's complement words
 */
void s16mul(uint16_t *f, uint16_t *d, uint16_t a, uint16_t b)
{
	int32_t tmp;

	tmp = tosigint(a) * tosigint(b);
	*d = fromsigint(tmp);

	/* Signed overflow bit */
	SET_BIT(*f, BIT_ccv, tmp & 0xffff0000);
}

/*
 * Calculate "a / b" and "a % b" treating both a and b as two's complement words
 *  "a / b" will be floored, "a % b" will be the same sign as the divisor
 */
void s16div(uint16_t *q, uint16_t *r, uint16_t a, uint16_t b)
{
	int32_t i_a, i_b, i_q;

	i_a = tosigint(a);
	i_b = tosigint(b);

	i_q	= i_a / i_b;

	if (i_q < 0 && i_a % i_b) /* Emulate floor division */
		--i_q;

	*q = fromsigint(i_q);
	if (r) /* NOTE: we need to support not caclulating a % b */
		*r = fromsigint(i_a - i_b * i_q);
}

/*
 * Compare a and b, set flags accordingly
 */
void s16cmp(uint16_t *f, uint16_t a, uint16_t b)
{
	int32_t i_a, i_b;

	i_a = tosigint(a);
	i_b = tosigint(b);

	/* Unsigned comparison */
	SET_BIT(*f, BIT_ccE, a == b);
	SET_BIT(*f, BIT_ccG, a > b);
	SET_BIT(*f, BIT_ccL, a < b);

	/* Signed comparison */
	SET_BIT(*f, BIT_ccg, i_a > i_b);
	SET_BIT(*f, BIT_ccl, i_a < i_b);
}

/*
 * Compare a and b, treating both as two's complement integers
 *  set d to 1 if a is less than b
 */
void s16cmplt(uint16_t *d, uint16_t a, uint16_t b)
{
	int32_t i_a, i_b;

	i_a = tosigint(a);
	i_b = tosigint(b);

	*d = i_a < i_b;
}

/*
 * Compare a and b, treating both as two's complement integers
 *  set d to 1 if a is greater than b
 */
void s16cmpgt(uint16_t *d, uint16_t a, uint16_t b)
{
	int32_t i_a, i_b;

	i_a = tosigint(a);
	i_b = tosigint(b);

	*d = i_a > i_b;
}
