#include <lua_5.4.3.h>
#include "util.h"

int retro_script_lua_rawgetfield(lua_State* L, int index, const char* field)
{
    lua_pushstring(L, field);
    return lua_rawget(L, (index < 0) ? index - 1 : index);
}

void retro_script_lua_rawsetfield(lua_State* L, int index, const char* field)
{
    lua_pushstring(L, field);
    lua_rotate(L, -2, 1);
    lua_rawset(L, (index < 0) ? index - 1 : index);
}