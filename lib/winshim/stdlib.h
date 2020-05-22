#ifndef WINSHIM_STDLIB_H
#define WINSHIM_STDLIB_H

#include_next <stdlib.h>

void *reallocarray(void *ptr, size_t nmemb, size_t size);

#endif
