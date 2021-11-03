#pragma once

#include <lua_5.4.3.h>

#include "hc_luafuncs.h"

int retro_script_luafunc_memory_read_char(lua_State* L);
int retro_script_luafunc_memory_read_byte(lua_State* L);
int retro_script_luafunc_memory_write_char(lua_State* L);
int retro_script_luafunc_memory_write_byte(lua_State* L);

#define DECLARE_LUAFUNCS_MEMORY_ACCESS(type, ctype, luatype) \
    DECLARE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
    DECLARE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, be)
    
#define DECLARE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
int retro_script_luafunc_memory_read_##type##_##le(lua_State* L); \
int retro_script_luafunc_memory_write_##type##_##le(lua_State* L);

DECLARE_LUAFUNCS_MEMORY_ACCESS(int16, int16_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(uint16, uint16_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(int32, int32_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(uint32, uint32_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(int64, int64_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(uint64, uint64_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(float32, float, number);
DECLARE_LUAFUNCS_MEMORY_ACCESS(float64, double, number);