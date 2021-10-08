#include "libretro_script.h"
#include "script.h"
#include "error.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int set_ref(lua_State* L)
{
    int n = lua_gettop(L);
    if (n > 0 && lua_isfunction(L, -1))
    {
        // copy the function 
        lua_pushvalue(L, -1);
        return luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        return LUA_NOREF;
    }
}

#define SET_SCRIPT_REF(ref) set_##ref
#define DEF_SET_SCRIPT_REF(REF) \
static int SET_SCRIPT_REF(REF)(struct lua_State* L) \
{   \
    script_state_t* script = script_find_lua(L); \
    if (script) \
    {   \
        script->refs.REF = set_ref(L);   \
    }   \
    return 0; \
}   

DEF_SET_SCRIPT_REF(on_run_begin);
DEF_SET_SCRIPT_REF(on_run_end);

static void lua_set_libs(lua_State* L)
{
    // base libs
    const luaL_Reg lib = { "base", luaopen_base };
    lib.func(L);
    lua_settop(L, 0);
    
    // register c functions
    #define REGISTER_SCRIPT_SET_REF(ref) \
        lua_register(L, "retro_" #ref, SET_SCRIPT_REF(ref));
    
    REGISTER_SCRIPT_SET_REF(on_run_begin);
    REGISTER_SCRIPT_SET_REF(on_run_end);
}

void retro_script_execute_cb(script_state_t* script, int ref)
{
    if (!script || ref == LUA_NOREF || ref == LUA_REFNIL)
    {
        return;
    }
    
    lua_State* L = script->L;
    
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (lua_isfunction(L, -1))
    {
        lua_call(L, 0, 0);
    }
    else
    {
        lua_pop(L, 1);
    }
}

RETRO_SCRIPT_API retro_script_id_t retro_script_load(const char* script_path)
{
    script_state_t* script_state = script_alloc(script_path);
    if (!script_state)
    {
        set_error_nofree("Unable to allocate script");
        return 0;
    }
    lua_State* L = script_state->L;
    
    lua_set_libs(L);
    int result = luaL_dofile(L, script_path);
    if (result)
    {
        set_error(lua_tostring(L, -1));
        lua_pop(L, 1);
        script_free(script_state->id);
        return 0;
    }
    else
    {
        return script_state->id;
    }
}