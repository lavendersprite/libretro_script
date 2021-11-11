#include <hcdebug.h>
#include "libretro_script.h"

#include "script.h"
#include "memmap.h"
#include "hc_hooks.h"
#include "core.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

// convenience
#define DECLT(x) RETRO_SCRIPT_DECLT(x)
#define INTERCEPT(name) RETRO_SCRIPT_DECLT(name) retro_script_intercept_##name(RETRO_SCRIPT_DECLT(name) f)
#define INTERCEPT_HANDLER(name) _interceptor_##name

struct core_t core;
struct frontend_callbacks_t frontend_callbacks;

enum
{
    RS_INIT, // retro-script init'd, but core not yet init'd
    CORE_INIT, // core init'd
    CORE_DEINIT, // core deinit'd (may be skipped if frontend does not deinit core)
    RS_DEINIT // retro-script de-init'd 
} state = RS_DEINIT;

// how many core_on_init/deinit can be registered
#define MAX_INIT_FUNCTIONS 8

static size_t retro_script_core_init_count = 0;
static size_t retro_script_core_deinit_count = 0;
static core_init_cb_t core_on_init_fn[MAX_INIT_FUNCTIONS];
static core_init_cb_t core_on_deinit_fn[MAX_INIT_FUNCTIONS];

// registers functions to run on init
void retro_script_register_on_init(core_init_cb_t cb)
{
    assert(retro_script_core_init_count < MAX_INIT_FUNCTIONS);
    assert(cb);
    core_on_init_fn[retro_script_core_init_count++] = cb;
}
void retro_script_register_on_deinit(core_init_cb_t cb)
{
    assert(retro_script_core_deinit_count < MAX_INIT_FUNCTIONS);
    assert(cb);
    core_on_deinit_fn[retro_script_core_deinit_count++] = cb;
}

uint32_t retro_script_init()
{
    #ifdef RETRO_SCRIPT_DEBUG
    retro_script_debug_force_include();
    #endif
    
    if (state == RS_DEINIT)
    {
        memset(&core, 0, sizeof(core));
        memset(&frontend_callbacks, 0, sizeof(frontend_callbacks));
        retro_script_clear_memory_map();
    }
    else
    {
        // de-init before init'ing
        retro_script_deinit();
    }
    
    // run init functions
    for (size_t i = 0; i < retro_script_core_init_count; ++i)
    {
        core_on_init_fn[i]();
    }
    
    state = RS_INIT;
    return RETRO_SCRIPT_API_VERSION;
}

void retro_script_deinit()
{
    if (state != RS_DEINIT)
    {
        for (size_t i = 0; i < retro_script_core_deinit_count; ++i)
        {
            core_on_deinit_fn[i]();
        }
        retro_script_clear_memory_map();
    }
    
    state = RS_DEINIT;
}

static void INTERCEPT_HANDLER(retro_run)()
{
    SCRIPT_ITERATE(script_state)
    {
        retro_script_execute_cb(script_state, script_state->refs.on_run_begin);
    }
    core.retro_run();
    SCRIPT_ITERATE(script_state)
    {
        retro_script_execute_cb(script_state, script_state->refs.on_run_end);
    }
}

static bool retro_environment(unsigned int cmd, void* data)
{
    bool result = false;
    
    switch (cmd)
    {
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
        result = retro_script_set_memory_map((struct retro_memory_map*)data);
        break;
    case RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK:
        core.retro_get_proc_address = ((struct retro_get_proc_address_interface*)data)->get_proc_address;
        result = !!core.retro_get_proc_address;
        break;
    }
    
    return frontend_callbacks.retro_environment(cmd, data) || result;
}

static void INTERCEPT_HANDLER(retro_set_environment)(retro_environment_t cb)
{
    frontend_callbacks.retro_environment = cb;
    core.retro_set_environment(retro_environment);
}

static void INTERCEPT_HANDLER(retro_init)()
{
    assert(state == RS_INIT);
    core.retro_init();
    state = CORE_INIT;
}

static void INTERCEPT_HANDLER(retro_set_input_poll)(retro_input_poll_t cb)
{
    frontend_callbacks.retro_input_poll = cb;
    core.retro_set_input_poll(cb);
}

static void INTERCEPT_HANDLER(retro_set_input_state)(retro_input_state_t cb)
{
    frontend_callbacks.retro_input_state = cb;
    core.retro_set_input_state(cb);
}

INTERCEPT(retro_set_environment) { return (core.retro_set_environment = f), INTERCEPT_HANDLER(retro_set_environment); }
INTERCEPT(retro_get_memory_data) { return (core.retro_get_memory_data = f), f; }
INTERCEPT(retro_get_memory_size) { return (core.retro_get_memory_size = f), f; }
INTERCEPT(retro_init)            { return (core.retro_init = f), INTERCEPT_HANDLER(retro_init); }
INTERCEPT(retro_deinit)          { return (core.retro_deinit = f), f; }
INTERCEPT(retro_run)             { return (core.retro_run = f), INTERCEPT_HANDLER(retro_run); }
INTERCEPT(retro_set_input_poll)  { return (core.retro_set_input_poll = f), INTERCEPT_HANDLER(retro_set_input_poll); }
INTERCEPT(retro_set_input_state) { return (core.retro_set_input_state = f), INTERCEPT_HANDLER(retro_set_input_state); }