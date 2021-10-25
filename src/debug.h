#pragma once

/* A collection of functions to help debug libretro-script
 */

#ifdef RETRO_SCRIPT_DEBUG

// ALWAYS_VISIBLE prevents compiler from optimizing out the function,
// so that it remains available in debug mode.
#if defined(__GNUC__) || defined(__clang__) || defined(__MINGW32__) || defined(__MINGW64__)
    #define ALWAYS_VISIBLE \
        __attribute__((noinline)) \
        __attribute__((__visibility__("default"))) \
        __attribute__((externally_visible))
#elif defined(_MSC_VER)
    #define ALWAYS_VISIBLE __declspec(noinline) 
#else
    #define ALWAYS_VISIBLE
#endif

struct lua_State;
ALWAYS_VISIBLE int retro_script_debug_lua_print_stack(struct lua_State* L);
ALWAYS_VISIBLE int retro_script_debug_lua_inspect(struct lua_State* L, int idx);
ALWAYS_VISIBLE void retro_script_debug_force_include();
#endif