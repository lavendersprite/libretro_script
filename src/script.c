#include <lua_5.4.3.h>

#include "script_luafuncs.h"
#include "hc_hooks.h"

#include "libretro_script.h"
#include "script.h"
#include "error.h"
#include "memmap.h"
#include "core.h"
#include "util.h"

// declaration for bitops lib.
#define LUA_BITLIBNAME "bit"
LUALIB_API int luaopen_bit(lua_State *L);

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

// default lua error response
static void lua_print_error_message(retro_script_id_t script_id, int lua_errorcode, const char* errmsg)
{
    printf("libretro_script lua error (code %d): \"%s\"\n", lua_errorcode, errmsg);
}

// lua error response (customizable)
static retro_script_lua_on_error_t lua_on_error = lua_print_error_message;

#define SET_SCRIPT_REF(ref) retro_script_luafunc_set_##ref
#define DEF_SET_SCRIPT_REF(REF, ALLOW_LIST) \
static int SET_SCRIPT_REF(REF)(struct lua_State* L) \
{   \
    script_state_t* script = script_find_lua(L); \
    if (script) \
    {   \
        if (script->refs.REF == LUA_NOREF) \
        { \
            lua_newtable(L); \
            script->refs.REF = luaL_ref(L, LUA_REGISTRYINDEX);\
        } \
        lua_rawgeti(L, LUA_REGISTRYINDEX, script->refs.REF); \
        lua_rotate(L, -2, 1); \
        if (!ALLOW_LIST) \
        { \
            lua_rawseti(L, -2, 1); \
        } \
        else { \
            int idx = lua_rawlen(L, -2) + 1; \
            lua_rawseti(L, -2, idx); \
        } \
        lua_pop(L, 1); \
    }   \
    return 0; \
}   

DEF_SET_SCRIPT_REF(on_run_begin, true);
DEF_SET_SCRIPT_REF(on_run_end, true);

// sets the built-in functions for lua,
// including the libretro-script functions.
static void lua_set_libs(lua_State* L, const char* default_package_path)
{
    // base libs
    luaL_requiref(L, LUA_GNAME, luaopen_base, true);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, true);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, true);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, true);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, true);
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, true);
    luaL_requiref(L, LUA_BITLIBNAME, luaopen_bit, false);
    
    // package gets special attention, as we set the path manually.
    {
        luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, true);
        if (default_package_path && lua_istable(L, -1))
        {
            // set package.path
            lua_pushstring(L, default_package_path);
            lua_setfield(L, -2, "path");
        }
    }
    lua_settop(L, 0);
    
    // register c functions
    #define REGISTER_FUNC(name, func) \
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
        
        REGISTER_FUNC("input_poll", retro_script_luafunc_input_poll);
        REGISTER_FUNC("input_state", retro_script_luafunc_input_state);
        
        retro_script_luafield_constants(L);
        
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
            
            retro_script_luafield_hc_main_cpu_and_memory(L);
            
            lua_setfield(L, -2, "hc");
        }
        
        // set this as package.loaded["retro"]
        luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        lua_rotate(L, -2, 1);
        lua_setfield(L, -2, "retro");
    }
    lua_settop(L, 0);
}

RETRO_SCRIPT_API
void retro_script_on_lua_error(retro_script_lua_on_error_t cb)
{
    lua_on_error = cb;
}

// helper for retro_script_lua_pcall
static void pop_error_and_return_nils(lua_State* L, int retc)
{
    // one error value on stack. Pop it.
    lua_pop(L, 1);
    
    // supplement stack with nils
    for (int i = 0; i < retc; ++i)
    {
        lua_pushnil(L);
    }
}

void retro_script_lua_pcall(lua_State* L, int argc, int retc)
{
    const int result = lua_pcall(L, argc, retc, 0);
    if (result != LUA_OK)
    {
        if (lua_on_error)
        {
            const script_state_t* script = script_find_lua(L);
            const retro_script_id_t script_id = script ? script->id : 0;
            
            // get error string
            char* lua_errmsg = NULL;
            const char* errmsg = NULL;
            
            if (!lua_isstring(L, -1))
            {
                errmsg = "(non-string error)";
            }
            else
            {
                lua_errmsg = retro_script_strdup(lua_tostring(L, -1));
                errmsg = "(insufficient memory to store error string)";
            }
            
            pop_error_and_return_nils(L, retc);
        
            lua_on_error(script_id, result, lua_errmsg ? lua_errmsg : errmsg);
            
            if (lua_errmsg) free(lua_errmsg);
        }
        else
        {
            pop_error_and_return_nils(L, retc);
        }
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
        retro_script_lua_pcall(L, 0, 0);
    }
    else if (lua_istable(L, -1))
    {
        const int len = lua_rawlen(L, -1);
        const int top = lua_gettop(L);
        for (int i = 1; i <= len; ++i)
        {
            lua_rawgeti(L, -1, i);
            retro_script_lua_pcall(L, 0, 0);
            lua_settop(L, top);
        }
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
    
    char* p, *f, *packagepath = NULL;
    retro_script_split_path_file(&p, &f, script_path);
    if (f) free(f);
    if (p)
    {
        size_t plen = strlen(p);
        packagepath = malloc(plen + 1 + strlen("?.lua"));
        memcpy(packagepath, p, plen);
        strcpy(packagepath + plen, "?.lua");
        free(p);
    }
    lua_set_libs(L, p ? packagepath : NULL);
    if (packagepath) free(packagepath);
    
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