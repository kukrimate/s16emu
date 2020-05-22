#ifndef WINSHIM_STRING_H
#define WINSHIM_STRING_H

#include_next <string.h>

char *strndup(char *s, size_t n);

#endif
