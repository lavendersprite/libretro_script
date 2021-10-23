#pragma once

#include <lua.h>

int retro_script_luafunc_hc_system_get_description(lua_State* L);
int retro_script_luafunc_hc_system_get_memory_regions(lua_State* L);
int retro_script_luafunc_hc_system_get_breakpoints(lua_State* L);
int retro_script_luafunc_hc_system_get_cpus(lua_State* L);