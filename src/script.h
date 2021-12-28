#pragma once

struct lua_State;

typedef struct script_state
{
    struct lua_State* L;
    retro_script_id_t id;
    struct script_state* next;
    
    // lua references.
    // unless otherwise stated, these are 'reflists.'
    // see: retro_script_reflist_lua_variable
    struct {
        int on_run_begin;
        int on_run_end;
    } refs;
} script_state_t;

void retro_script_execute_cb(script_state_t*, int ref);
int retro_script_lua_pcall(struct lua_State*, int argc, int retc);
void retro_script_on_uncaught_error(struct lua_State* L, int status);