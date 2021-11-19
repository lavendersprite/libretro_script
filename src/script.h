#pragma once

#include "script_list.h"

void retro_script_execute_cb(script_state_t*, int ref);
int retro_script_lua_pcall(lua_State*, int argc, int retc);
void retro_script_on_uncaught_error(lua_State* L, int status);