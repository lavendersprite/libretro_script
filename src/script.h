#pragma once

#include "script_list.h"

void retro_script_execute_cb(script_state_t*, int ref);
void retro_script_lua_pcall(lua_State*, int argc, int retc);