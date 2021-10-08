#pragma once

#include <string.h>
#include <stdlib.h>

#define malloc_array(type, len) ((type*) malloc(sizeof(type) * len))
#define alloc(type) malloc_array(type, 1)

inline char* _strdup(const char* s)
{
    size_t len = strlen(s) + 1;
    char* t = malloc_array(char, len);
    if (!t) return NULL;
    memcpy(t, s, len);
    return t;
}