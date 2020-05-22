/*
 * MinGW-w64 portablity shims
 */

#include <errno.h>
#include <stdlib.h>

/*
 * reallocarray shim with overflow checking
 */
void *reallocarray(void *ptr, size_t nmemb, size_t size)
{
	size_t total;

	total = nmemb * size;

	if (total < nmemb || total < size) {
		errno = ENOMEM;
		return NULL;
	}

	return realloc(ptr, total);
}

/*
 * IDK why, but MinGW-w64 does not have strndup :(
 */
char *strndup(char *s, size_t n)
{
	char *d, *p;

	p = d = malloc(n + 1);
	if (!d)
		return NULL;

	for (; n && *s; ++p, ++s, --n) {
		*p = *s;
	}
	*p = 0;

	return d;
}
