#pragma once

// we use hcdebug.h (hackable-console), a libretro debug interface,
// to get access to some additional features.

#include <hcdebug.h>

hc_DebuggerIf* retro_script_hc_get_debugger();
// for efficiency reasons in use case, we provide three userdata slots.
// (usage: <lua context, memory, lua callback ref>)

#define RETRO_SCRIPT_HC_BREAKPOINT_USERDATA_COUNT 2

typedef struct retro_script_hc_breakpoint_userdata
{
    union
    {
        void* ptr;
        uint64_t u64;
    } values[RETRO_SCRIPT_HC_BREAKPOINT_USERDATA_COUNT];
} retro_script_hc_breakpoint_userdata;

typedef void (*retro_script_breakpoint_cb)(retro_script_hc_breakpoint_userdata, hc_SubscriptionID breakpoint_id, hc_Event const*);
int retro_script_hc_register_breakpoint(retro_script_hc_breakpoint_userdata const*, hc_SubscriptionID breakpoint_id, retro_script_breakpoint_cb); // returns 1 if failure
int retro_script_hc_unregister_breakpoint(hc_SubscriptionID breakpoint_id); // returns 1 if failure