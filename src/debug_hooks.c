#include "debug_hooks.h"
#include "core.h"
#include "hashmap.h"

#include <hcdebug.h>

typedef struct breakpoint_entry
{
    retro_script_hc_breakpoint_userdata userdata;
    retro_script_breakpoint_cb cb;
} breakpoint_entry;

static struct retro_script_hashmap* breakpoint_hashmap = NULL;

ON_INIT()
{
    if (breakpoint_hashmap) retro_script_hashmap_destroy(breakpoint_hashmap);
    breakpoint_hashmap = retro_script_hashmap_create(sizeof(breakpoint_entry));
}

// clear all scripts when a core is unloaded
ON_DEINIT()
{
    if (breakpoint_hashmap) retro_script_hashmap_destroy(breakpoint_hashmap);
}

static void on_breakpoint(unsigned id)
{
    // check if this is one of ours...
    breakpoint_entry* entry = (breakpoint_entry*)(
        retro_script_hashmap_get(breakpoint_hashmap, id)
    );
    
    if (entry)
    {
        entry->cb(entry->userdata, id);
    }
    // otherwise, forward breakpoint callback to frontend
    else if (callbacks.breakpoint_cb)
    {
        callbacks.breakpoint_cb(id);
    }
}

int retro_script_hc_register_breakpoint(retro_script_hc_breakpoint_userdata userdata, unsigned breakpoint_id, retro_script_breakpoint_cb cb)
{
    if (!cb) return 1;
    if (!retro_script_hc_get_debugger()) return 1;
    
    breakpoint_entry* entry = (breakpoint_entry*)(
        retro_script_hashmap_add(breakpoint_hashmap, breakpoint_id)
    );
    
    if (!entry) return 1;
    entry->userdata = userdata;
    entry->cb = cb;
    return 0;
}

int retro_script_hc_unregister_breakpoint(unsigned breakpoint_id)
{
    if (!retro_script_hc_get_debugger()) return 1;
    return !retro_script_hashmap_remove(breakpoint_hashmap, breakpoint_id);
}

static void init_debugger(hc_DebuggerIf* debugger)
{
    *(unsigned*)&debugger->frontend_api_version = HC_API_VERSION;
    *(breakpoint_cb_t*)&debugger->v1.breakpoint_cb = on_breakpoint;
}

hc_DebuggerIf* retro_script_hc_get_debugger()
{
    // obtain debugger if we don't have it yet
    if (!core.hc.debugger_is_init)
    {
        if (!core.hc.set_debugger)
        {
            if (core.retro_get_proc_address)
            {
                core.hc.set_debugger = (hc_Set)core.retro_get_proc_address("hc_get_debugger");
                if (!core.hc.set_debugger)
                {
                    // no debugger available
                    return NULL;
                }
            }
            else
            {
                // no debugger available.
                return NULL;
            }
        }
        
        // initialize debugger and also debug hashmap
        core.hc.debugger_is_init = true;
        init_debugger(&core.hc.debugger);
        core.hc.userdata = core.hc.set_debugger(&core.hc.debugger);
    }
    
    return &core.hc.debugger;
}