/*
 * Emulate Sigma16 TC arihmatic without relying on the host being TC as well
 * NOTE: this needs a C99 compiler to not rely on undefined behaviour
 */
#include <stdint.h>
#include "internal.h"

#define S16_NEG(x) ((s16word) ~x)

/*
 * Add two words
 */
void s16add(s16word *r, s16word a, s16word b)
{
	*r = a + b;
}

/*
 * Calculate "a - b" by computing the TC of b and adding
 */
void s16sub(s16word *r, s16word a, s16word b)
{
	*r = a + (s16word) ~b + 1;
}

/*
 * Convert a Sigma16 TC word into a signed integer
 */
static int s16wordtoint(s16word x)
{
	if (x & S16_MAXBIT) {
		return -(int) ((s16word) ~x + 1);
	} else {
		return (int) x;
	}
}

/*
 * Convert a signed integer to Sigma16 TC word
 */
static s16word intotos16word(int x)
{

	if (x < 0) {
		return (s16word) ~(s16word) -x + 1;
	} else {
		return (s16word) x;
	}
}

/*
 * Calculate "a * b" treating both a and b as TC Sigma16 words
 */
void s16mul(s16word *r, s16word a, s16word b)
{
	*r = intotos16word(s16wordtoint(a) * s16wordtoint(b));
}

/*
 * Calculate "a / b" and "a % b" treating both a and b as TC Sigma16 words,
 * "a / b" will be floored, "a % b" will be the same sign as the divisor
 */
void s16div(s16word *q, s16word *r, s16word a, s16word b)
{
	int i_a, i_b, i_q;

	i_a = s16wordtoint(a);
	i_b = s16wordtoint(b);

	i_q	= i_a / i_b;

	/* NOTE: For the following code to run correctly C99 behaviour is required,
		e.g. integer division *must* be truncated (earlier standards allowed
		for floor division) */

	if (i_q < 0 && i_a % i_b) /* Emulate floor division */
		--i_q;

	*q = intotos16word(i_q);
	*r = intotos16word(a - b * i_q);
}
