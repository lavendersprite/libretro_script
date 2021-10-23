#include "script_luafuncs.h"
#include "memmap.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

int retro_script_luafunc_memory_read_char(lua_State* L)
{
    int n = lua_gettop(L);
    if (n > 0 && lua_isinteger(L, -1))
    {
        lua_Integer addr = lua_tointeger(L, -1);
        if (addr < 0) return 0; // invalid usage
        
        char out;
        if (retro_script_memory_read_char(addr, &out))
        {
            lua_pushinteger(L, out);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

int retro_script_luafunc_memory_read_byte(lua_State* L)
{
    int n = lua_gettop(L);
    if (n >= 1 && lua_isinteger(L, -1))
    {
        lua_Integer addr = lua_tointeger(L, -1);
        if (addr < 0) return 0; // invalid usage
        
        unsigned char out;
        if (retro_script_memory_read_byte(addr, &out))
        {
            lua_pushinteger(L, out);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

int retro_script_luafunc_memory_write_char(lua_State* L)
{
    int n = lua_gettop(L);
    if (n >= 2 && lua_isinteger(L, -1) && lua_isinteger(L, -2))
    {
        lua_Integer addr = lua_tointeger(L, -2);
        if (addr < 0) return 0; // invalid usage
        
        char in = lua_tointeger(L, -1);
        lua_pushinteger(L,
            retro_script_memory_write_char(addr, in)
        );
        
        return 1;
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

int retro_script_luafunc_memory_write_byte(lua_State* L)
{
    int n = lua_gettop(L);
    if (n >= 2 && lua_isinteger(L, -1) && lua_isinteger(L, -2))
    {
        lua_Integer addr = lua_tointeger(L, -2);
        if (addr < 0) return 0; // invalid usage
        
        unsigned char in = lua_tointeger(L, -1);
        lua_pushinteger(L,
            retro_script_memory_write_byte(addr, in)
        );
        
        return 1;
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

#define DEFINE_LUAFUNCS_MEMORY_ACCESS(type, ctype, luatype) \
    DEFINE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
    DEFINE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, be)

#define DEFINE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
int retro_script_luafunc_memory_read_##type##_##le(lua_State* L) \
{ \
    int n = lua_gettop(L); \
    if (n >= 1 && lua_isinteger(L, -1)) \
    { \
        lua_Integer addr = lua_tointeger(L, -1); \
        if (addr < 0) return 0; \
        ctype out; \
        if (retro_script_memory_read_##type##_##le(addr, &out)) \
        { \
            lua_push##luatype(L, out); \
            return 1; \
        } \
    } \
    return 0; \
} \
int retro_script_luafunc_memory_write_##type##_##le(lua_State* L) \
{ \
    int n = lua_gettop(L); \
    if (n >= 2 && lua_isinteger(L, -2) && lua_is##luatype(L, -1)) \
    { \
        lua_Integer addr = lua_tointeger(L, -2); \
        if (addr < 0) return 0; \
        ctype in = lua_to##luatype(L, -1); \
        lua_pushinteger(L, \
            retro_script_memory_write_##type##_##le(addr, in) \
        ); \
        return 1; \
    } \
    return 0; \
}

DEFINE_LUAFUNCS_MEMORY_ACCESS(int16, int16_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(uint16, uint16_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(int32, int32_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(uint32, uint32_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(int64, int64_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(uint64, uint64_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(float32, float, number);
DEFINE_LUAFUNCS_MEMORY_ACCESS(float64, double, number);