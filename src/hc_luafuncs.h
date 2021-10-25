#pragma once

#include <lua.h>

// c functions callable from lua
int retro_script_luafunc_hc_system_get_description(lua_State* L);
int retro_script_luafunc_hc_system_get_memory_regions(lua_State* L);
int retro_script_luafunc_hc_system_get_breakpoints(lua_State* L);
int retro_script_luafunc_hc_system_get_cpus(lua_State* L);

// field setters

// sets "main_cpu" and "main_memory"
void retro_script_luavalue_hc_main_cpu_and_memory(lua_State* L);