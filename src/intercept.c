#include "libretro_script.h"
#include "script.h"
#include <stdio.h>

// convenience
#define DECLT(x) RETRO_SCRIPT_DECLT(x)
#define INTERCEPT(name) RETRO_SCRIPT_DECLT(name) retro_script_intercept_##name(RETRO_SCRIPT_DECLT(name) f)
#define INTERCEPT_HANDLER(name) _interceptor_##name

static struct
{
    DECLT(retro_set_environment) retro_set_environment;
    DECLT(retro_get_memory_data) retro_get_memory_data;
    DECLT(retro_get_memory_size) retro_get_memory_size;
    DECLT(retro_run) retro_run;
} core;

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

INTERCEPT(retro_set_environment) { return (core.retro_set_environment = f), f; }
INTERCEPT(retro_get_memory_data) { return (core.retro_get_memory_data = f), f; }
INTERCEPT(retro_get_memory_size) { return (core.retro_get_memory_size = f), f; }
INTERCEPT(retro_run)             { return (core.retro_run = f), INTERCEPT_HANDLER(retro_run); }