#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "script_luafuncs.h"
#include "debug_hooks.h"

#include "libretro_script.h"
#include "script.h"
#include "error.h"
#include "memmap.h"
#include "core.h"

// clear all scripts when a core is loaded
ON_INIT()
{
    script_clear_all();
}

// clear all scripts when a core is unloaded
ON_DEINIT()
{
    script_clear_all();
}

static int set_ref(lua_State* L)
{
    int n = lua_gettop(L);
    if (n > 0 && lua_isfunction(L, -1))
    {
        // copy the function to the registry
        lua_pushvalue(L, -1);
        return luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        return LUA_NOREF;
    }
}

#define SET_SCRIPT_REF(ref) retro_script_luafunc_set_##ref
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

// sets the built-in functions for lua,
// including the libretro-script functions.
static void lua_set_libs(lua_State* L)
{
    // base libs
    const luaL_Reg lib = { "base", luaopen_base };
    lib.func(L);
    lua_settop(L, 0);
    
        // register c functions
    #define REGISTER_FUNC(name, func) \
        lua_pushvalue(L, -1); \
        lua_pushcfunction(L, func); \
        lua_setfield(L, -2, name)
    #define REGISTER_SCRIPT_SET_REF(ref) \
        REGISTER_FUNC(#ref, SET_SCRIPT_REF(ref))
    
    #define REGISTER_MEMORY_ACCESS_ENDIAN(type, le) \
        REGISTER_FUNC("read_" #type "_" #le, retro_script_luafunc_memory_read_##type##_##le); \
        REGISTER_FUNC("write_" #type "_" #le, retro_script_luafunc_memory_write_##type##_##le); \
    
    #define REGISTER_MEMORY_ACCESS(type) \
        REGISTER_MEMORY_ACCESS_ENDIAN(type, le); \
        REGISTER_MEMORY_ACCESS_ENDIAN(type, be);
    
    // create a global 'retro' 
    {
        lua_newtable(L);
        
        REGISTER_FUNC("read_char", retro_script_luafunc_memory_read_char);
        REGISTER_FUNC("write_char", retro_script_luafunc_memory_write_char);
        REGISTER_FUNC("read_byte", retro_script_luafunc_memory_read_byte);
        REGISTER_FUNC("write_byte", retro_script_luafunc_memory_write_byte);
        
        REGISTER_MEMORY_ACCESS(int16);
        REGISTER_MEMORY_ACCESS(uint16);
        REGISTER_MEMORY_ACCESS(int32);
        REGISTER_MEMORY_ACCESS(uint32);
        REGISTER_MEMORY_ACCESS(int64);
        REGISTER_MEMORY_ACCESS(uint64);
        REGISTER_MEMORY_ACCESS(float32);
        REGISTER_MEMORY_ACCESS(float64);
        
        REGISTER_SCRIPT_SET_REF(on_run_begin);
        REGISTER_SCRIPT_SET_REF(on_run_end);
        
        // retro.hc
        if (retro_script_hc_get_debugger())
        {
            lua_newtable(L);
            
            REGISTER_FUNC("system_get_description", retro_script_luafunc_hc_system_get_description);
            REGISTER_FUNC("system_get_memory_regions", retro_script_luafunc_hc_system_get_memory_regions);
            REGISTER_FUNC("system_get_breakpoints", retro_script_luafunc_hc_system_get_breakpoints);
            REGISTER_FUNC("system_get_cpus", retro_script_luafunc_hc_system_get_cpus);
            
            lua_setfield(L, -2, "hc");
        }
        
        lua_setglobal(L, "retro");
    }
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

RETRO_SCRIPT_API retro_script_id_t retro_script_load_lua(const char* script_path)
{
    return retro_script_load_lua_special(script_path, NULL);
}

RETRO_SCRIPT_API retro_script_id_t retro_script_load_lua_special(const char* script_path, retro_script_setup_lua_t frontend_setup)
{
    script_state_t* script_state = script_alloc(script_path);
    if (!script_state)
    {
        set_error_nofree("Unable to allocate script");
        return 0;
    }
    lua_State* L = script_state->L;
    
    lua_set_libs(L);
    
    int no_error = 1; // becomes 0 if error.
    
    // core can modify lua state.
    if (core.retro_get_proc_address)
    {
        retro_script_setup_lua_t core_setup = (retro_script_setup_lua_t)core.retro_get_proc_address("retro_script_setup_lua");
        if (core_setup)
        {
            lua_getglobal(L, "retro");
            no_error = core_setup(L);
            lua_pop(L, 1);
        }
    }
    
    // front-end can modify lua state
    if (no_error && frontend_setup)
    {
        lua_getglobal(L, "retro");
        no_error = frontend_setup(L);
        lua_pop(L, 1);
    }
    
    if (no_error) no_error = luaL_dofile(L, script_path);
    if (no_error)
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