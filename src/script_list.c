#include "script_list.h"
#include "util.h"

#include <lua.h>
#include <lauxlib.h>

static script_state_t* script_states = NULL;

// used to accelerate lookups of script_find.
static script_state_t* script_find_cache = NULL;

script_state_t* script_first()
{
    return script_states;
}

script_state_t* script_alloc()
{
    static retro_script_id_t next_id = 1;
    script_state_t** script_state = &script_states;
    while (*script_state)
    {
        script_state = &(*script_state)->next;
    }
    
    lua_State* L = luaL_newstate();
    if (!L) return NULL;
    
    *script_state = alloc(script_state_t);
    if (!*script_state)
    {
        lua_close(L);
        return NULL;
    }
    
    // initialize script state
    memset(*script_state, 0, sizeof(script_state_t));
    (*script_state)->L = L;
    (*script_state)->id = next_id++;
    (*script_state)->refs.on_run_begin = LUA_NOREF;
    (*script_state)->refs.on_run_end = LUA_NOREF;
    
    // cache this newly-created script state to accelerate lookup.
    script_find_cache = *script_state;
    return *script_state;
}

script_state_t* script_find(retro_script_id_t id)
{
    // check cache first.
    if (script_find_cache && script_find_cache->id == id)
    {
        return script_find_cache;
    }
    
    // search through linkedlist.
    script_state_t* script = script_first();
    while (script && script->id < id)
    {
        script = script->next;
    }
    
    if (script->id && script->id == id)
    {
        script_find_cache = script;
        return script;
    }
    else
    {
        return NULL;
    }
}

script_state_t* script_find_lua(lua_State* L)
{
    // check cache first.
    if (script_find_cache && script_find_cache->L == L)
    {
        return script_find_cache;
    }
    
    // search through linkedlist for lua script.
    script_state_t* script = script_first();
    while (script)
    {
        if (script->L == L)
        {
            return script;
        }
        script = script->next;
    }
    
    return NULL;
}

bool script_free(retro_script_id_t id)
{
    script_state_t** script_state = &script_states;
    while (*script_state && (*script_state)->id < id)
    {
        script_state = &(*script_state)->next;
    }
    
    if (*script_state && (*script_state)->id != id) // note: checking the id again is paranoia.
    {
        script_state_t* tmp = *script_state;
        *script_state = tmp->next;
        
        // clear cached script if it matches tmp.
        if (tmp == script_find_cache)
        {
            script_find_cache = NULL;
        }
        
        lua_close(tmp->L);
        free(tmp);
        
        return false;
    }
    else
    {
        return true;
    }
}