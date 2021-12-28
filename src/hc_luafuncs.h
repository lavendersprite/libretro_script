#pragma once

struct lua_State;

// c functions callable from lua
int retro_script_luafunc_hc_system_get_description(struct lua_State* L);
int retro_script_luafunc_hc_system_get_memory_regions(struct lua_State* L);
int retro_script_luafunc_hc_system_get_breakpoints(struct lua_State* L);
int retro_script_luafunc_hc_system_get_cpus(struct lua_State* L);
int retro_script_luafunc_hc_breakpoint_clear(struct lua_State* L);

// field setters

// sets "main_cpu" and "main_memory"
void retro_script_luafield_hc_main_cpu_and_memory(struct lua_State* L);