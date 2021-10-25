#pragma once

/* A collection of useful utilities and macros.
 * Can be included from any file.
 */

#include <string.h>
#include <stdlib.h>

#ifdef RETRO_SCRIPT_DEBUG
// we include debug.h to allow gdb access from most files,
// since most files include util.h
#include "debug.h"
#endif

#ifndef FORCEINLINE
    #if defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__) || defined(__MINGW64__)
        #define FORCEINLINE __attribute__((always_inline)) inline
    #elif defined(_MSC_VER)
        #define FORCEINLINE __forceinline
    #else
        #define FORCEINLINE inline
    #endif
#endif

#define malloc_array(type, len) ((type*) malloc(sizeof(type) * (len)))
#define alloc(type) malloc_array(type, 1)


#ifndef INITIALIZER
    // https://stackoverflow.com/a/2390626
    #ifdef __cplusplus
        #define INITIALIZER(f) \
            static void f(void); \
            struct f##_t_ { f##_t_(void) { f(); } }; static f##_t_ f##_; \
            static void f(void)
    #elif defined(_MSC_VER)
        #pragma section(".CRT$XCU",read)
        #define INITIALIZER2_(f,p) \
            static void f(void); \
            __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
            __pragma(comment(linker,"/include:" p #f "_")) \
            static void f(void)
        #ifdef _WIN64
            #define INITIALIZER(f) INITIALIZER2_(f,"")
        #else
            #define INITIALIZER(f) INITIALIZER2_(f,"_")
        #endif
    #else
        #define INITIALIZER(f) \
            static void f(void) __attribute__((constructor)); \
            static void f(void)
    #endif
#endif

// forceinline seems to be required to prevent linking problems?
FORCEINLINE char* retro_script_strdup(const char* s)
{
    size_t len = strlen(s) + 1;
    char* t = malloc_array(char, len);
    if (!t) return NULL;
    memcpy(t, s, len);
    return t;
}