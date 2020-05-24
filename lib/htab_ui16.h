#ifndef HTAB_UI16_H
#define HTAB_UI16_H

typedef struct helem_ui16 helem_ui16;
struct helem_ui16 {
	uint16_t key;
	char *val;
	helem_ui16 *nex;
};

typedef struct {
	/* # of used buckets */
	size_t used_count;
	/* # of buckets */
	size_t bucket_count;
	/* buffer */
	helem_ui16 **buffer;
} htab_ui16;


/*
 * Create a new hashtab_ui16le with n preallocated buckets
 */
void htab_ui16_new(htab_ui16 *x, size_t n);

/*
 * Delete a hashtab_ui16le freeing all memory used, if d is set
 * than key/values will also be free'd
 */
void htab_ui16_del(htab_ui16 *x, int d);

/*
 * Insert a key/value pair into a hashtab_ui16le, if d is set
 * than key/values will be free'd before overwriting
 */
void htab_ui16_put(htab_ui16 *x, uint16_t key, char *val, int d);

/*
 * Retrieve a value mapped to a given key,
 * if the key is not in the table NULL is returned
 */
char *htab_ui16_get(htab_ui16 *x, uint16_t key);

#endif
