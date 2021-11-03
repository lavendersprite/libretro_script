// these functions are for helping to debug libretro_script.

#ifdef RETRO_SCRIPT_DEBUG
#include "util.h"

#include <lua_5.4.3.h>

typedef void (*retro_script_debug_func_t)();

#ifdef __GNUC__
__attribute__((externally_visible))
__attribute__((__visibility__("default")))
#endif
volatile retro_script_debug_func_t retro_script_debug_funcs[] = {
    (retro_script_debug_func_t)retro_script_debug_lua_print_stack,
    (retro_script_debug_func_t)retro_script_debug_lua_inspect
};

volatile retro_script_debug_func_t retro_script_debug_force_visible_fn;

static void print_indent(int indent)
{
    for (int i = 0; i < indent; ++i)
    {
        printf("  ");
    }
}

#define STACKPRINT(fmt, ...) do {if (i >= 0) printf("%d. " fmt "\n", i, __VA_ARGS__); else printf(fmt "\n", __VA_ARGS__);} while (0)
static void lua_inspect(struct lua_State* L, int i, int verbose, int indent)
{
    switch (lua_type(L, i))
    {
    case LUA_TNUMBER:
        STACKPRINT("%f", lua_tonumber(L, i));
        break;
    case LUA_TSTRING:
        STACKPRINT("\"%s\"", lua_tostring(L, i));
        break;
    case LUA_TLIGHTUSERDATA:
        STACKPRINT("lightuserdata %p", lua_touserdata(L, i));
        break;
    case LUA_TUSERDATA:
        STACKPRINT("userdata %p", lua_touserdata(L, i));
        break;
    case LUA_TTABLE:
        STACKPRINT("%s", "table");
        if (verbose)
        {
            int first = 1;
            printf("{");
            
            lua_pushnil(L); 
            while (lua_next(L, i))
            {
                if (first)
                {
                    printf("\n");
                    first = 0;
                }
                print_indent(indent);
                if (lua_isstring(L, -2))
                {
                    printf("  %s: ", lua_tostring(L, -2));
                }
                else if (lua_isinteger(L, -2))
                {
                    printf("  %lld: ", lua_tointeger(L, -2));
                }
                else
                {
                    printf("  [?]: ");
                }
                lua_inspect(L, -1, 0, 1);
                lua_pop(L, 1);
            }
            if (first) printf(" ");
            printf("}\n");
        }
        break;
    default:
        STACKPRINT("unknown type \"%s\"", lua_typename(L, lua_type(L, i)));
    }
}

int retro_script_debug_lua_print_stack(lua_State* L)
{
    for (size_t i = 1; i <= lua_gettop(L); ++i)
    {
        lua_inspect(L, i, 0, 0);
    }
    return lua_gettop(L);
}

int retro_script_debug_lua_inspect(lua_State* L, int idx)
{
    lua_inspect(L, idx, 1, 0);
    return 1;
}

void retro_script_debug_force_include()
{
    for (volatile size_t i = 0; i < sizeof(retro_script_debug_funcs) / sizeof(retro_script_debug_funcs[0]); ++i)
    {
        //printf("debug: %p\n", retro_script_debug_funcs[i]);
        retro_script_debug_force_visible_fn = retro_script_debug_funcs[i];
        asm("");
    }
}
#endif