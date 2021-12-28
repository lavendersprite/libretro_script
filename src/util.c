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

void retro_script_reflist_lua_variable(lua_State* L, int* ref)
{
    if (lua_gettop(L) <= 0)
    {
        luaL_error(L, "cannot reflist variable; empty stack");
        return;
    }
    
    if (*ref == LUA_NOREF)
    {
        // create the list to be ref'd
        lua_newtable(L);
        
        // duplicate
        lua_pushvalue(L, -1);
        
        // set the ref.
        *ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        // retrieve the list
        lua_rawgeti(L, LUA_REGISTRYINDEX, *ref);
    }
    
    if (!lua_istable(L, -1))
    {
        luaL_error(L, "expected reflist, but not a table.");
        return;
    }
    
    // key to store value in list under.
    const int key = lua_rawlen(L, -1) + 1;
    
    // put value at top of lua stack into the list.
    // (pops the value.)
    lua_rotate(L, -2, 1);
    lua_rawseti(L, -2, key);
    
    // pop the list
    lua_pop(L, 1);
}

// returns zero if no value to pop.
int retro_script_reflist_get(lua_State* L, int ref, int i)
{
    if (ref == LUA_NOREF)
    {
        return 0;
    }
    
    // get reflist
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    
    if (!lua_istable(L, -1))
    {
        luaL_error(L, "expected reflist, but not a table.");
        return 0;
    }
    
    // retrieve value from reflist
    if (lua_rawgeti(L, -1, i) == LUA_TNIL)
    {
        // value is nil, so nothing found.
        lua_pop(L, 2);
        return 0;
    }
    else
    {
        // pop list.
        lua_rotate(L, -2, 1);
        lua_pop(L, 1);
        
        // we found something, so return nonzero.
        return 1;
    }
}