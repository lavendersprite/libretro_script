#include "hc_luafuncs.h"
#include "hc_hooks.h"
#include "hashmap.h"
#include "core.h"
#include "hc_registers.h"
#include "script.h"

#include <libretro.h>
#include <hcdebug.h>

#include <lua_5.4.3.h>

#define debugger (retro_script_hc_get_debugger())
#define system (debugger->v1.system)

#define USERDATA_FIELD "_ud"

#define nargs(L) (lua_gettop(L))

// caches tables representing hc objects like cpus and memory regions
// TODO: this should be per-script.
static struct retro_script_hashmap* lua_table_cache = NULL;

ON_INIT()
{
    if (lua_table_cache) retro_script_hashmap_destroy(lua_table_cache);
    lua_table_cache = retro_script_hashmap_create(sizeof(lua_Integer));
}

ON_DEINIT()
{
    if (lua_table_cache) retro_script_hashmap_destroy(lua_table_cache);
}

// retrieves a unique persistent lua table for the given pointer
// returns 0 if table already existed.
static bool lua_table_for_data(lua_State* L, void const* ptr)
{
    uintptr_t v = (uintptr_t)ptr;
    lua_Integer* refptr = retro_script_hashmap_get(lua_table_cache, v);
    if (refptr == NULL)
    {
        // add a hashmap entry
        refptr = retro_script_hashmap_add(lua_table_cache, v);
        
        // create lua table. Duplicate it so a copy stays on the stack afterward.
        lua_newtable(L);
        lua_pushvalue(L, -1);
        
        // set the hashmap entry to a ref for this table.
        *refptr = luaL_ref(L, LUA_REGISTRYINDEX);
        return 1;
    }
    else
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, *refptr);
        return 0;
    }
}

static void assert_argc_range(lua_State* L, int lower, int upper)
{
    const int argc = nargs(L);
    if (argc < lower || argc >= upper)
    {
        luaL_error(L, "Invalid number of arguments (%d); expected %d-%d args.", argc, lower, upper);
    }
}

static void assert_argc(lua_State* L, int count)
{
    const int argc = nargs(L);
    if (argc != count)
    {
        luaL_error(L, "Invalid number of arguments (%d); expected %d", argc, count);
    }
}

// retrieves userdata field from deepest elt on stack.
// luaL_error is called on failure.
static void* get_userdata_from_self(lua_State* L)
{
    if (lua_gettop(L) <= 0) goto FAIL;
    if (!lua_istable(L, 1)) goto FAIL;
    
    if (lua_rawgetfield(L, 1, USERDATA_FIELD) != LUA_TLIGHTUSERDATA)
    {
        lua_pop(L, 1);
        goto FAIL;
    }
    else
    {
        void* v = lua_touserdata(L, -1);
        lua_pop(L, 1);
        return v;
    }
    
FAIL:
    luaL_error(L, "invalid 'self' argument.");
    return NULL;
}

static inline void poke_range(hc_Memory const* mem, uint64_t start, size_t count, const void* vdata, bool reverse)
{
    const char* data = (const char*)vdata;
    for (size_t i = 0; i < count; ++i)
    {
        size_t index = reverse ? (count - i - 1) : i;
        mem->v1.poke(start + index, *data);
        data++;
    }
}

static inline void peek_range(hc_Memory const* mem, uint64_t start, size_t count, void* vdata, bool reverse)
{
    char* data = (char*)vdata;
    for (size_t i = 0; i < count; ++i)
    {
        size_t index = reverse ? (count - i - 1) : i;
        *data = mem->v1.peek(start + index);
        data++;
    }
}

// endianness
static const int be = 0;
static const int le = 1;

#define SYS_IS_BIGENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

#define HC_MEMORY_ACCESS_PREAMBLE(argc) \
    assert_argc(L, argc); \
    hc_Memory const* mem = (hc_Memory const*)get_userdata_from_self(L); \
    lua_Integer address = lua_tointeger(L, 2); \

#define DEFINE_HC_MEMORY_READ(lua_type, ctype, le) \
static int hc_memory_read_##ctype##_##le(lua_State* L) \
{ \
    HC_MEMORY_ACCESS_PREAMBLE(2); \
    ctype v; \
    peek_range(mem, address, sizeof(v), &v, le == SYS_IS_BIGENDIAN); \
    lua_push##lua_type(L, v); \
    return 1; \
}

#define DEFINE_HC_MEMORY_WRITE(lua_type, ctype, le) \
static int hc_memory_write_##ctype##_##le(lua_State* L) \
{ \
    HC_MEMORY_ACCESS_PREAMBLE(3); \
    ctype v = lua_to##lua_type(L, 3); \
    poke_range(mem, address, sizeof(v), &v, le == SYS_IS_BIGENDIAN); \
    return 0; \
}

#define DEFINE_HC_MEMORY_ACCESS(ctype, lua_type) \
    DEFINE_HC_MEMORY_READ(lua_type, ctype, le) \
    DEFINE_HC_MEMORY_READ(lua_type, ctype, be) \
    DEFINE_HC_MEMORY_WRITE(lua_type, ctype, le) \
    DEFINE_HC_MEMORY_WRITE(lua_type, ctype, be) \

static int hc_memory_read_char(lua_State* L)
{
    HC_MEMORY_ACCESS_PREAMBLE(2);
    char v = mem->v1.peek(address);
    lua_pushinteger(L, v);
    return 1;
}

static int hc_memory_read_byte(lua_State* L)
{
    HC_MEMORY_ACCESS_PREAMBLE(2);
    uint8_t v = mem->v1.peek(address);
    lua_pushinteger(L, v);
    return 1;
}

static int hc_memory_write_char(lua_State* L)
{
    HC_MEMORY_ACCESS_PREAMBLE(3);
    lua_Integer v = lua_tointeger(L, 3);
    mem->v1.poke(address, v);
    return 0;
}

static int hc_memory_write_byte(lua_State* L)
{
    HC_MEMORY_ACCESS_PREAMBLE(3);
    lua_Integer v = lua_tointeger(L, 3);
    mem->v1.poke(address, v);
    return 0;
}

DEFINE_HC_MEMORY_ACCESS(int16_t, integer);
DEFINE_HC_MEMORY_ACCESS(uint16_t, integer);
DEFINE_HC_MEMORY_ACCESS(int32_t, integer);
DEFINE_HC_MEMORY_ACCESS(uint32_t, integer);
DEFINE_HC_MEMORY_ACCESS(int64_t, integer);
DEFINE_HC_MEMORY_ACCESS(uint64_t, integer);
DEFINE_HC_MEMORY_ACCESS(float, number);
DEFINE_HC_MEMORY_ACCESS(double, number);

// precondition: the top el't of the lua stack is a lua callback function
// postcondition: the top el't of the lua stack is the breakpoint id
// returns breakpoint id or -1
static hc_SubscriptionID breakpoint_register(lua_State* L, hc_Subscription const* s, retro_script_breakpoint_cb cb)
{
    lua_pushvalue(L, -1);
    uintptr_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    const hc_SubscriptionID id = debugger->v1.subscribe(s);
    if (id < 0) return -1;
    
    retro_script_hc_breakpoint_userdata u;
    u.values[0].ptr = L;
    u.values[1].u64 = ref;
    retro_script_hc_register_breakpoint(&u, id, cb);
    
    lua_pushinteger(L, id);
    return id;
}

static void pcall_function_from_ref(lua_State* L, lua_Integer ref, const int argc, const int retc)
{
    const int top = lua_gettop(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (lua_isfunction(L, -1))
    {
        int result = retro_script_lua_pcall(L, argc, retc);
        retro_script_on_uncaught_error(L, result);
        if (result != LUA_OK) goto return_nils;
    }
    else
    {
    return_nils:
        lua_settop(L, top);
        for (int i = 0; i < retc; ++i)
        {
            lua_pushnil(L);
        }
    }
}

static void on_step_event(retro_script_hc_breakpoint_userdata u, hc_SubscriptionID id, hc_Event const* e)
{
    // step events only fire once. Unregister this event.
    retro_script_hc_unregister_breakpoint(id);
    
    lua_State* L = (lua_State*)u.values[0].ptr;
    lua_Integer ref = u.values[1].u64;
    
    // TODO: arguments.
    const int argc = 0;
    
    pcall_function_from_ref(L, ref, argc, 0);
}

static int cpu_step_into(lua_State* L)
{
    assert_argc(L, 2);
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu || !debugger->v1.subscribe) return 0;
    
    hc_Subscription s;
    s.type = HC_EVENT_EXECUTION;
    s.execution.cpu = cpu;
    s.execution.type = HC_STEP_SKIP_INTERRUPT;
    s.execution.address_range_begin = 0;
    s.execution.address_range_end = -1;
    
    return breakpoint_register(L, &s, on_step_event) >= 0;
}

static int cpu_step_over(lua_State* L)
{
    assert_argc(L, 2);
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu || !debugger->v1.subscribe) return 0;
    
    hc_Subscription s;
    s.type = HC_EVENT_EXECUTION;
    s.execution.cpu = cpu;
    s.execution.type = HC_STEP_CURRENT_SUBROUTINE;
    s.execution.address_range_begin = 0;
    s.execution.address_range_end = -1;
    
    return breakpoint_register(L, &s, on_step_event) >= 0;
}

static int cpu_step_out(lua_State* L)
{
    assert_argc(L, 2);
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu || !debugger->v1.subscribe) return 0;
    
    hc_Subscription s;
    s.type = HC_EVENT_RETURN;
    s.execution_return.cpu = cpu;
    
    return breakpoint_register(L, &s, on_step_event) >= 0;
}

static int memory_peek(lua_State* L)
{
    assert_argc(L, 2);
    
    hc_Memory const* memory = (hc_Memory const*)get_userdata_from_self(L);
    if (!memory || !memory->v1.peek) return 0;
    
    uint64_t address = lua_tointeger(L, 2);
    uint8_t value = memory->v1.peek(address);
    
    lua_pushinteger(L, value);
    return 1;
}

static int memory_poke(lua_State* L)
{
    assert_argc(L, 3);
    
    hc_Memory const* memory = (hc_Memory const*)get_userdata_from_self(L);
    if (!memory || !memory->v1.poke) return 0;
    
    uint64_t address = lua_tointeger(L, 2);
    
    uint8_t value = lua_tointeger(L, 3);
    
    memory->v1.poke(address, value);
    return 0;
}

// invoked on cpu exec breakpoint trigger
static void on_breakpoint(retro_script_hc_breakpoint_userdata u, hc_SubscriptionID breakpoint_id, hc_Event const* e)
{
    lua_State* L = (lua_State*)u.values[0].ptr;
    lua_Integer ref = u.values[1].u64;
    
    const int argc = 0;
    
    pcall_function_from_ref(L, ref, argc, 0);
}

// invoked on cpu exec breakpoint trigger
static void on_cpu_exec(retro_script_hc_breakpoint_userdata u, hc_SubscriptionID breakpoint_id, hc_Event const* e)
{
    lua_State* L = (lua_State*)u.values[0].ptr;
    lua_Integer ref = u.values[1].u64;
    
    // TODO: arguments.
    const int argc = 1;
    lua_pushinteger(L, e->execution.address);
    
    pcall_function_from_ref(L, ref, argc, 0);
}

// invoked on watchpoint trigger
static void on_memory_access(retro_script_hc_breakpoint_userdata u, hc_SubscriptionID breakpoint_id, hc_Event const* e)
{
    lua_State* L = (lua_State*)u.values[0].ptr;
    uintptr_t ref = (uintptr_t)u.values[1].u64;
    
    // TODO: arguments.
    const int argc = 3;
    lua_pushinteger(L, e->memory.address);
    lua_pushinteger(L, e->memory.operation);
    lua_pushinteger(L, e->memory.value);
    
    pcall_function_from_ref(L, ref, argc, 0);
}

// invoked on register breakpoint trigger
static void on_register_breakpoint(retro_script_hc_breakpoint_userdata u, hc_SubscriptionID breakpoint_id, hc_Event const* e)
{
    lua_State* L = (lua_State*)u.values[0].ptr;
    uintptr_t ref = (uintptr_t)u.values[1].u64;
    
    // TODO: arguments.
    const int argc = 2;
    lua_pushinteger(L, e->reg.reg);
    lua_pushinteger(L, e->reg.new_value);
    
    pcall_function_from_ref(L, ref, argc, 0);
}

// lua args: self
//      ret: value
static int get_register(lua_State* L)
{
    assert_argc(L, 1);
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu) return 0;
    
    lua_Integer idx;
    if (lua_rawgetfield(L, 1, "_idx") == LUA_TNUMBER)
    {
        idx = lua_tointeger(L, -1);
    }
    else
    {
        lua_pop(L, 1);
        return 0;
    }
    
    if (!cpu->v1.get_register) return 0;
    
    lua_pushinteger(L, cpu->v1.get_register(idx));
    
    return 1;
}

// lua args: self, value
static int set_register(lua_State* L)
{
    assert_argc(L, 2);
    if (!lua_isinteger(L, 2)) return 0;
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu) return 0;
    
    lua_Integer idx;
    if (lua_rawgetfield(L, 1, "_idx") == LUA_TNUMBER)
    {
        idx = lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    else
    {
        lua_pop(L, 1);
        return 0;
    }
    
    if (!cpu->v1.set_register) return 0;
    
    cpu->v1.set_register(
        idx,
        lua_tointeger(L, 2)
    );
    
    return 0;
}

// lua args: self, callback
static int set_register_breakpoint(lua_State* L)
{
    assert_argc(L, 2);
    if (!lua_isfunction(L, 2)) return 0;
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu) return 0;
    
    lua_Integer idx;
    if (lua_rawgetfield(L, 0, "_idx") == LUA_TNUMBER)
    {
        idx = lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    else
    {
        lua_pop(L, 1);
        return 0;
    }
    
    hc_Subscription s;
    {
        s.type = HC_EVENT_REG;
        s.reg.cpu = cpu;
        s.reg.reg = idx;
    }
    
    return breakpoint_register(L, &s, on_register_breakpoint) >= 0;
}

// lua args: self, address, length, [read/write string], callback
//      ret: breakpoint id
static int memory_set_watchpoint(lua_State* L)
{
    assert_argc_range(L, 4, 5);
    if (!lua_isfunction(L, -1)) return 0;
    
    hc_Memory const* memory = (hc_Memory const*)get_userdata_from_self(L);
    if (!memory) return 0;
    
    uint64_t address = lua_tointeger(L, 2);
    uint64_t length = lua_tointeger(L, 3);
    const char* mode = lua_isstring(L, 4) ? lua_tostring(L, 4) : NULL;
    const bool watch_read = mode ? !!strchr(mode, 'r') : 0;
    const bool watch_write = mode ? !!strchr(mode, 'w') : 1;
    
    hc_Subscription s;
    {
        s.type = HC_EVENT_MEMORY;
        s.memory.memory = memory;
        s.memory.address_range_begin = address;
        s.memory.address_range_end = address + length;
        s.memory.operation = 0;
        if (watch_read) s.memory.operation |= HC_MEMORY_READ;
        if (watch_write) s.memory.operation |= HC_MEMORY_WRITE;
    }
    
    return breakpoint_register(L, &s, on_memory_access) >= 0;
}

// args: self, callback
//  lua return: breakpoint id
static int breakpoint_enable(lua_State* L)
{
    assert_argc(L, 2);
    
    hc_GenericBreakpoint const* breakpoint = (hc_GenericBreakpoint const*)get_userdata_from_self(L);
    if (!breakpoint) return 0;
    
    hc_Subscription s;
    {
        s.type = HC_EVENT_GENERIC;
        s.generic.breakpoint = breakpoint;
    }
    
    return breakpoint_register(L, &s, on_breakpoint) >= 0;
}

// args: self, address, callback
//  ret: breakpoint id
static int cpu_set_exec_breakpoint(lua_State* L)
{
    assert_argc(L, 3);
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu || !debugger->v1.subscribe) return 0;
    
    uint64_t address = lua_tointeger(L, 2);
    
    hc_Subscription s;
    {
        s.type = HC_EVENT_EXECUTION;
        s.execution.cpu = cpu;
        s.execution.type = HC_STEP;
        s.execution.address_range_begin = address;
        s.execution.address_range_end = address + 1;
    }
    
    return breakpoint_register(L, &s, on_cpu_exec) >= 0;
}

static int push_breakpoint(lua_State* L, hc_GenericBreakpoint const* breakpoint)
{
    if (lua_table_for_data(L, breakpoint))
    {
        lua_pushlightuserdata(L, (void*)breakpoint);
        lua_rawsetfield(L, -2, USERDATA_FIELD);
        
        if (breakpoint->v1.description)
        {
            lua_pushstring(L, breakpoint->v1.description);
            lua_rawsetfield(L, -2, "description");
        }

        lua_pushcfunction(L, breakpoint_enable);
        lua_rawsetfield(L, -2, "enable");
    }
    
    return 1;
}

static int push_memory_region(lua_State* L, hc_Memory const* mem)
{
    if (lua_table_for_data(L, mem))
    {
        lua_pushlightuserdata(L, (void*)mem);
        lua_rawsetfield(L, -2, USERDATA_FIELD);
        
        if (mem->v1.id)
        {
            lua_pushstring(L, mem->v1.id);
            lua_rawsetfield(L, -2, "id");
        }
        
        if (mem->v1.description)
        {
            lua_pushstring(L, mem->v1.description);
            lua_rawsetfield(L, -2, "id");
        }
        
        lua_pushinteger(L, mem->v1.alignment);
        lua_rawsetfield(L, -2, "alignment");
        
        lua_pushinteger(L, mem->v1.base_address);
        lua_rawsetfield(L, -2, "base_address");
        
        lua_pushinteger(L, mem->v1.size);
        lua_rawsetfield(L, -2, "size");
        
        #define FUNCFIELD(name, f) lua_pushcfunction(L, f); lua_rawsetfield(L, -2, #name);
        #define MEMFIELD(read, name, type) \
            FUNCFIELD(read##_##name##_le, hc_memory_##read##_##type##_le); \
            FUNCFIELD(read##_##name##_be, hc_memory_##read##_##type##_be)
        
        if (mem->v1.peek)
        {
            lua_pushcfunction(L, memory_peek);
            lua_rawsetfield(L, -2, "peek");
            FUNCFIELD(read_byte, hc_memory_read_byte);
            FUNCFIELD(read_char, hc_memory_read_char);
            MEMFIELD(read, int16, int16_t);
            MEMFIELD(read, uint16, uint16_t);
            MEMFIELD(read, int32, int32_t);
            MEMFIELD(read, uint32, uint32_t);
            MEMFIELD(read, int64, int64_t);
            MEMFIELD(read, uint64, uint64_t);
            MEMFIELD(read, float32, float);
            MEMFIELD(read, float64, double);
        }
        
        if (mem->v1.poke)
        {
            lua_pushcfunction(L, memory_poke);
            lua_rawsetfield(L, -2, "poke");
            FUNCFIELD(read_byte, hc_memory_write_byte);
            FUNCFIELD(read_char, hc_memory_write_char);
            MEMFIELD(write, int16, int16_t);
            MEMFIELD(write, uint16, uint16_t);
            MEMFIELD(write, int32, int32_t);
            MEMFIELD(write, uint32, uint32_t);
            MEMFIELD(write, int64, int64_t);
            MEMFIELD(write, uint64, uint64_t);
            MEMFIELD(write, float32, float);
            MEMFIELD(write, float64, double);
        }
        
        lua_pushcfunction(L, memory_set_watchpoint);
        lua_rawsetfield(L, -2, "set_watchpoint");
        
        lua_createtable(L, mem->v1.num_break_points, 0);
        for (size_t i = 0; i < mem->v1.num_break_points; ++i)
        {
            if (mem->v1.break_points[i])
            {
                (void)push_breakpoint(L, mem->v1.break_points[i]);
                lua_rawseti(L, -2, i + 1);
            }
        }
        lua_rawsetfield(L, -2, "breakpoints");
    }
    
    return 1;
}

static int push_cpu(lua_State* L, hc_Cpu const* cpu)
{
    if (lua_table_for_data(L, cpu))
    {
        lua_pushlightuserdata(L, (void*)cpu);
        lua_rawsetfield(L, -2, USERDATA_FIELD);
                
        if (cpu->v1.description)
        {
            lua_pushstring(L, cpu->v1.description);
            lua_rawsetfield(L, -2, "description");
        }
        
        lua_pushinteger(L, cpu->v1.type);
        lua_rawsetfield(L, -2, "type");
        
        const char* cpu_name = retro_script_hc_get_cpu_name(cpu->v1.type);
        if (cpu_name)
        {
            lua_pushstring(L, cpu_name);
            lua_rawsetfield(L, -2, "name");
        }
        
        // registers
        const int num_registers = retro_script_hc_get_cpu_register_count(cpu->v1.type);
        if (num_registers >= 0)
        {
            lua_newtable(L);
            for (size_t i = 0; i < num_registers; ++i)
            {
                lua_newtable(L);
                
                lua_pushlightuserdata(L, (void*)cpu);
                lua_rawsetfield(L, -2, USERDATA_FIELD);
                
                lua_pushinteger(L, i);
                lua_rawsetfield(L, -2, "_idx");
                
                const char* name = retro_script_hc_get_cpu_register_name(cpu->v1.type, i);
                if (name)
                {
                    lua_pushstring(L, name);
                    lua_rawsetfield(L, -2, "name");
                    
                    // register name as key for register, e.g. registers.X
                    if (*name)
                    {
                        lua_pushvalue(L, -1);
                        lua_rawsetfield(L, -3, name);
                    }
                }
                    
                if (cpu->v1.get_register)
                {
                    lua_pushcfunction(L, get_register);
                    lua_rawsetfield(L, -2, "get");
                }
                
                if (cpu->v1.set_register)
                {
                    lua_pushcfunction(L, set_register);
                    lua_rawsetfield(L, -2, "set");
                }
                
                lua_pushcfunction(L, set_register_breakpoint);
                lua_rawsetfield(L, -2, "watch");
                
                lua_rawseti(L, -2, i + 1);
            }
            
            lua_rawsetfield(L, -2, "registers");
        }
        
        lua_pushboolean(L, cpu->v1.is_main);
        lua_rawsetfield(L, -2, "is_main");
        
        if (cpu->v1.memory_region)
        {
            push_memory_region(L, cpu->v1.memory_region);
            lua_rawsetfield(L, -2, "memory");
        }
        
        lua_pushcfunction(L, cpu_step_into);
        lua_rawsetfield(L, -2, "step_into");
        
        lua_pushcfunction(L, cpu_step_over);
        lua_rawsetfield(L, -2, "step_over");
        
        lua_pushcfunction(L, cpu_step_out);
        lua_rawsetfield(L, -2, "step_out");
        
        lua_pushcfunction(L, cpu_set_exec_breakpoint);
        lua_rawsetfield(L, -2, "set_exec_breakpoint");
        
        lua_createtable(L, cpu->v1.num_break_points, 0);
        for (size_t i = 0; i < cpu->v1.num_break_points; ++i)
        {
            if (cpu->v1.break_points[i])
            {
                (void)push_breakpoint(L, cpu->v1.break_points[i]);
                lua_rawseti(L, -2, i + 1);
            }
        }
        lua_rawsetfield(L, -2, "breakpoints");
    }
        
    return 1;
}

int retro_script_luafunc_hc_system_get_description(lua_State* L)
{
    lua_pushstring(L, system->v1.description);
    return 1;
}

int retro_script_luafunc_hc_system_get_memory_regions(lua_State* L)
{
    lua_createtable(L, system->v1.num_memory_regions, 0);
    for (size_t i = 0; i < system->v1.num_memory_regions; ++i)
    {
        if (system->v1.memory_regions[i])
        {
            (void)push_memory_region(L, system->v1.memory_regions[i]);
            lua_rawseti(L, -2, i + 1);
        }
    }
    return 1;
}

int retro_script_luafunc_hc_system_get_breakpoints(lua_State* L)
{
    lua_createtable(L, system->v1.num_break_points, 0);
    for (size_t i = 0; i < system->v1.num_break_points; ++i)
    {
        if (system->v1.break_points[i])
        {
            (void)push_breakpoint(L, system->v1.break_points[i]);
            lua_rawseti(L, -2, i + 1);
        }
    }
    return 1;
}

int retro_script_luafunc_hc_system_get_cpus(lua_State* L)
{
    lua_createtable(L, system->v1.num_cpus, 0);
    for (size_t i = 0; i < system->v1.num_cpus; ++i)
    {
        if (system->v1.cpus[i])
        {
            (void)push_cpu(L, system->v1.cpus[i]);
            lua_rawseti(L, -2, i + 1);
        }
    }
    return 1;
}

int retro_script_luafunc_hc_breakpoint_clear(lua_State* L)
{
    assert_argc(L, 1);
    const unsigned int breakpoint_id = lua_tointeger(L, 1);
    const unsigned int was_removed = retro_script_hc_unregister_breakpoint(breakpoint_id);
    lua_pushboolean(L, was_removed);
    return 1;
}

void retro_script_luafield_hc_main_cpu_and_memory(lua_State* L)
{
    if (!debugger || !system) return;
    for (size_t i = 0; i < system->v1.num_cpus; ++i)
    {
        hc_Cpu const* cpu = system->v1.cpus[i];
        if (cpu && cpu->v1.is_main)
        {
            (void)push_cpu(L, cpu);
            lua_rawsetfield(L, -2, "main_cpu");
            
            // add cpu memory if available
            if (cpu->v1.memory_region)
            {
                (void)push_memory_region(L, cpu->v1.memory_region);
                lua_rawsetfield(L, -2, "main_memory");
            }
            return;
        }
    }
}