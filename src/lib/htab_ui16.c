/*
 * Hash table using DJB2 and chaining
 * Auhor: Mate Kukri
 * License: ISC
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "htab_ui16.h"

// static size_t djb2_hash(char *str)
// {
// 	size_t r;

// 	r = 5381;
// 	for (; *str; ++str)
// 		r = (r << 5) + r + *str;
// 	return r;
// }

/*
 * Put key/value paris into a hashtab_ui16le *without* checking for key overwrites
 */
static void htab_ui16_putuniq(htab_ui16 *x, uint16_t key, char *val)
{
	helem_ui16 **e;

	e = x->buffer + key % x->bucket_count;
	if (!*e)
		++x->used_count;
	else
		for (; *e; e = &(*e)->nex);

	*e = malloc(sizeof(helem_ui16));
	if (!*e)
		abort();
	(*e)->key = key;
	(*e)->val = val;
	(*e)->nex = NULL;
}

/*
 * "Grow" a hashtab_ui16le by creating a new one and overwriting it
 */
static void htab_ui16_grow(htab_ui16 *x, size_t n)
{
	htab_ui16 y;
	helem_ui16 **b, *e, *t;

	htab_ui16_new(&y, n);
	for (b = x->buffer; b < x->buffer + x->bucket_count; ++b)
		for (e = *b; e; ) {
			htab_ui16_putuniq(&y, e->key, e->val);
			t = e->nex;
			free(e);
			e = t;
		}
	free(x->buffer);
	*x = y;
}


void htab_ui16_new(htab_ui16 *x, size_t n)
{
	x->used_count   = 0;
	x->bucket_count = n;
	x->buffer       = calloc(n, sizeof(void *));
	if (!x->buffer)
		abort();
}

void htab_ui16_del(htab_ui16 *x, int d)
{
	helem_ui16 **b, *e, *t;

	for (b = x->buffer; b < x->buffer + x->bucket_count; ++b)
		for (e = *b; e; ) {
			if (d) {
				free(e->val);
			}
			t = e->nex;
			free(e);
			e = t;
		}
	free(x->buffer);
}


void htab_ui16_put(htab_ui16 *x, uint16_t key, char *val, int d)
{
	helem_ui16 **e;

	if (x->used_count > x->bucket_count * 3 / 4)
		htab_ui16_grow(x, x->bucket_count * 2);

	e = x->buffer + key % x->bucket_count;

	if (!*e)
		++x->used_count;
	else
		for (; *e; e = &(*e)->nex)
			if ((*e)->key == key) {
				if (d) {
					free((*e)->val);
				}
				goto insert;
			}

	*e = malloc(sizeof(helem_ui16));
	if (!*e)
		abort();
	(*e)->nex = NULL;

insert:
	(*e)->key = key;
	(*e)->val = val;
}

char *htab_ui16_get(htab_ui16 *x, uint16_t key)
{
	helem_ui16 **e;

	e = x->buffer + key % x->bucket_count;
	for (; *e; e = &(*e)->nex)
		if ((*e)->key == key)
			return (*e)->val;

	return NULL;
}
