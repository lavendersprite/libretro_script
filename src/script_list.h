#pragma once

#include "libretro_script.h"
#include "script.h"
#include <lua_5.4.3.h>

// allocates a new script, but does not initialize it.
// returns NULL only if not enough memory to allocate.
script_state_t* script_alloc();

// retrieves the script with the given index.
// returns NULL if no such script.
script_state_t* script_find(retro_script_id_t);

// retrieves the script with the given Lua state
// returns NULL if no such script.
script_state_t* script_find_lua(lua_State* L);

// allows iterating over all scripts
// returns NULL if no scripts.
script_state_t* script_first();

// returns true if no script at given index.
// frees the path and lua state as well.
bool script_free(retro_script_id_t);

// removes all scripts.
void script_clear_all();

#define SCRIPT_ITERATE(script_state) for (script_state_t* script_state = script_first(); script_state; script_state = script_state->next)