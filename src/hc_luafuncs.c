#include "hc_luafuncs.h"
#include "hc_hooks.h"
#include "hashmap.h"
#include "core.h"
#include "hc_registers.h"

#include <libretro.h>
#include <hcdebug.h>

#include <lauxlib.h>
#include <lualib.h>

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

// retrieves userdata field from deepest elt on stack.
static void* get_userdata_from_self(lua_State* L)
{
    if (lua_gettop(L) <= 0) return NULL;
    if (!lua_istable(L, 1)) return NULL;
    
    if (lua_getfield(L, 1, USERDATA_FIELD) != LUA_TLIGHTUSERDATA)
    {
        lua_pop(L, 1);
        return NULL;
    }
    else
    {
        void* v = lua_touserdata(L, -1);
        lua_pop(L, 1);
        return v;
    }
}

static inline void poke_range(hc_Memory const* mem, uint64_t start, size_t count, const void* vdata, bool reverse)
{
    const char* data = (const char*)vdata;
    for (size_t i = 0; i < count; ++i)
    {
        size_t index = reverse ? (count - i - 1) : i;
        mem->v1.poke(core.hc.userdata,  + index, *data);
        data++;
    }
}

static inline void peek_range(hc_Memory const* mem, uint64_t start, size_t count, void* vdata, bool reverse)
{
    char* data = (char*)vdata;
    for (size_t i = 0; i < count; ++i)
    {
        size_t index = reverse ? (count - i - 1) : i;
        *data = mem->v1.peek(core.hc.userdata,  + index);
        data++;
    }
}

// endianness
static const int be = 0;
static const int le = 1;

#define SYS_IS_BIGENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

#define HC_MEMORY_ACCESS_PREAMBLE(argc) \
    if (nargs(L) != argc) return 0; \
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
    char v = mem->v1.peek(core.hc.userdata, address);
    lua_pushinteger(L, v);
    return 1;
}

static int hc_memory_read_byte(lua_State* L)
{
    HC_MEMORY_ACCESS_PREAMBLE(2);
    uint8_t v = mem->v1.peek(core.hc.userdata, address);
    lua_pushinteger(L, v);
    return 1;
}

static int hc_memory_write_char(lua_State* L)
{
    HC_MEMORY_ACCESS_PREAMBLE(3);
    lua_Integer v = lua_tointeger(L, 3);
    mem->v1.poke(core.hc.userdata, address, v);
    return 0;
}

static int hc_memory_write_byte(lua_State* L)
{
    HC_MEMORY_ACCESS_PREAMBLE(3);
    lua_Integer v = lua_tointeger(L, 3);
    mem->v1.poke(core.hc.userdata, address, v);
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

static int cpu_step_into(lua_State* L)
{
    if (nargs(L) != 1) return 0;
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu || !cpu->v1.step_into) return 0;
    
    cpu->v1.step_into(core.hc.userdata);
    return 0;
}

static int cpu_step_over(lua_State* L)
{
    if (nargs(L) != 1) return 0;
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu || !cpu->v1.step_over) return 0;
    
    cpu->v1.step_over(core.hc.userdata);
    return 0;
}

static int cpu_step_out(lua_State* L)
{
    if (nargs(L) != 1) return 0;
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu || !cpu->v1.step_out) return 0;
    
    cpu->v1.step_out(core.hc.userdata);
    return 0;
}

static int memory_peek(lua_State* L)
{
    if (nargs(L) != 2) return 0;
    
    hc_Memory const* memory = (hc_Memory const*)get_userdata_from_self(L);
    if (!memory || !memory->v1.peek) return 0;
    
    uint64_t address = lua_tointeger(L, 2);
    uint8_t value = memory->v1.peek(core.hc.userdata, address);
    
    lua_pushinteger(L, value);
    return 1;
}

static int memory_poke(lua_State* L)
{
    if (nargs(L) != 3) return 0;
    
    hc_Memory const* memory = (hc_Memory const*)get_userdata_from_self(L);
    if (!memory || !memory->v1.poke) return 0;
    
    uint64_t address = lua_tointeger(L, 2);
    
    uint8_t value = lua_tointeger(L, 3);
    
    memory->v1.poke(core.hc.userdata, address, value);
    return 0;
}

static void pcall_function_from_ref(lua_State* L, lua_Integer ref, const int argc, const int retc)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (lua_isfunction(L, -1))
    {
        if (lua_pcall(L, argc, retc, 0) != LUA_OK)
        {
            // one error value on stack. Pop it.
            lua_pop(L, 1);
            
            // supplement stack with nils
            for (size_t i = 0; i < retc; ++i)
            {
                lua_pushnil(L);
            }
        }
    }
    else
    {
        // for some reason, not a function.
        // just clean up the stack.
        lua_pop(L, 1 + argc);
    }
}

// invoked on cpu exec breakpoint trigger
static void on_breakpoint(retro_script_hc_breakpoint_userdata u, unsigned breakpoint_id)
{
    lua_State* L = (lua_State*)u.values[0].ptr;
    lua_Integer ref = u.values[1].u64;
    
    // TODO: arguments.
    const int argc = 0;
    
    pcall_function_from_ref(L, ref, argc, 0);
}

// invoked on cpu exec breakpoint trigger
static void on_cpu_exec(retro_script_hc_breakpoint_userdata u, unsigned breakpoint_id)
{
    lua_State* L = (lua_State*)u.values[0].ptr;
    lua_Integer ref = u.values[1].u64;
    
    // TODO: arguments.
    const int argc = 0;
    
    pcall_function_from_ref(L, ref, argc, 0);
}

// invoked on watchpoint trigger
static void on_memory_access(retro_script_hc_breakpoint_userdata u, unsigned breakpoint_id)
{
    lua_State* L = (lua_State*)u.values[0].ptr;
    uintptr_t ref = (uintptr_t)u.values[1].u64;
    
    // TODO: arguments.
    const int argc = 0;
    
    pcall_function_from_ref(L, ref, argc, 0);
}

// invoked on register breakpoint trigger
static void on_register_breakpoint(retro_script_hc_breakpoint_userdata u, unsigned breakpoint_id)
{
    lua_State* L = (lua_State*)u.values[0].ptr;
    uintptr_t ref = (uintptr_t)u.values[1].u64;
    
    // TODO: arguments.
    const int argc = 0;
    
    pcall_function_from_ref(L, ref, argc, 0);
}

// lua args: self
//      ret: value
static int get_register(lua_State* L)
{
    if (nargs(L) != 1) return 0;
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu) return 0;
    
    lua_Integer idx;
    if (lua_getfield(L, 1, "_idx") == LUA_TNUMBER)
    {
        idx = lua_tointeger(L, -1);
    }
    else
    {
        lua_pop(L, 1);
        return 0;
    }
    
    if (!cpu->v1.get_register) return 0;
    
    lua_pushinteger(L, cpu->v1.get_register(core.hc.userdata, idx));
    
    return 1;
}

// lua args: self, value
static int set_register(lua_State* L)
{
    if (nargs(L) != 2) return 0;
    if (!lua_isinteger(L, 2)) return 0;
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu) return 0;
    
    lua_Integer idx;
    if (lua_getfield(L, 0, "_idx") == LUA_TNUMBER)
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
        core.hc.userdata,
        idx,
        lua_tointeger(L, 2)
    );
    
    return 0;
}

// lua args: self, callback
static int set_register_breakpoint(lua_State* L)
{
    if (nargs(L) != 2) return 0;
    if (!lua_isfunction(L, 2)) return 0;
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu) return 0;
    
    lua_Integer idx;
    if (lua_getfield(L, 0, "_idx") == LUA_TNUMBER)
    {
        idx = lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    else
    {
        lua_pop(L, 1);
        return 0;
    }
    
    if (!cpu->v1.set_reg_breakpoint) return 0;
    
    // REF the provided function
    lua_pushvalue(L, -1);
    uintptr_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    const unsigned id = cpu->v1.set_reg_breakpoint(
        core.hc.userdata, idx
    );
    
    retro_script_hc_breakpoint_userdata u;
    u.values[0].ptr = L;
    u.values[1].u64 = ref;
    retro_script_hc_register_breakpoint(&u, id, on_register_breakpoint);
    
    // return breakpoint id
    lua_pushinteger(L, id);
    return 1;
}

// lua args: self, address, length, [read/write string], callback
//      ret: breakpoint id
static int memory_set_watchpoint(lua_State* L)
{
    if (nargs(L) != 4 && nargs(L) != 5) return 0;
    if (!lua_isfunction(L, -1)) return 0;
    
    hc_Memory const* memory = (hc_Memory const*)get_userdata_from_self(L);
    if (!memory) return 0;
    
    uint64_t address = lua_tointeger(L, 2);
    uint64_t length = lua_tointeger(L, 3);
    const char* mode = lua_isstring(L, 4) ? lua_tostring(L, 4) : NULL;
    const bool watch_read = mode ? !!strchr(mode, 'r') : 0;
    const bool watch_write = mode ? !!strchr(mode, 'w') : 1;
    
    // REF the provided function
    lua_pushvalue(L, -1);
    uintptr_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    const unsigned id = memory->v1.set_watchpoint(
        core.hc.userdata, address, length, watch_read, watch_write
    );
    
    retro_script_hc_breakpoint_userdata u;
    u.values[0].ptr = L;
    u.values[1].u64 = ref;
    retro_script_hc_register_breakpoint(&u, id, on_memory_access);
    
    // return breakpoint id
    lua_pushinteger(L, id);
    return 1;
}

// args: self, [yes], callback
//  ret: breakpoint id
static int breakpoint_enable(lua_State* L)
{
    if (nargs(L) != 3 && nargs(L) != 2) return 0;
    
    hc_Breakpoint const* breakpoint = (hc_Breakpoint const*)get_userdata_from_self(L);
    if (!breakpoint || !breakpoint->v1.enable) return 0;
    
    // FIXME: what does 'yes' mean? How should we respect it..?
    const bool yes = (nargs(L) == 3)
        ?  lua_toboolean(L, 2)
        : 1;
    
    // REF the provided function
    lua_pushvalue(L, -1);
    uintptr_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    const unsigned id = breakpoint->v1.enable(core.hc.userdata, yes);
    
    retro_script_hc_breakpoint_userdata u;
    u.values[0].ptr = L;
    u.values[1].u64 = ref;
    retro_script_hc_register_breakpoint(&u, id, on_breakpoint);
    
    lua_pushinteger(L, id);
    return 1;
}

// args: self, address, callback
//  ret: breakpoint id
static int cpu_set_exec_breakpoint(lua_State* L)
{
    if (nargs(L) != 3) return 0;
    
    hc_Cpu const* cpu = (hc_Cpu const*)get_userdata_from_self(L);
    if (!cpu || !cpu->v1.set_exec_breakpoint) return 0;
    
    uint64_t address = lua_tointeger(L, 2);
    
    // REF the provided function
    lua_pushvalue(L, -1);
    uintptr_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    const unsigned id = cpu->v1.set_exec_breakpoint(core.hc.userdata, address);
    
    retro_script_hc_breakpoint_userdata u;
    u.values[0].ptr = L;
    u.values[1].u64 = ref;
    retro_script_hc_register_breakpoint(&u, id, on_cpu_exec);
    
    lua_pushinteger(L, id);
    return 1;
}

static int push_breakpoint(lua_State* L, hc_Breakpoint const* breakpoint)
{
    if (lua_table_for_data(L, breakpoint))
    {
        lua_pushlightuserdata(L, (void*)breakpoint);
        lua_setfield(L, -2, USERDATA_FIELD);
        
        if (breakpoint->v1.description)
        {
            lua_pushstring(L, breakpoint->v1.description);
            lua_setfield(L, -2, "description");
        }
        
        if (breakpoint->v1.enable)
        {
            lua_pushcfunction(L, breakpoint_enable);
            lua_setfield(L, -2, "enable");
        }
    }
    
    return 1;
}

static int push_memory_region(lua_State* L, hc_Memory const* mem)
{
    if (lua_table_for_data(L, mem))
    {
        lua_pushlightuserdata(L, (void*)mem);
        lua_setfield(L, -2, USERDATA_FIELD);
        
        if (mem->v1.id)
        {
            lua_pushstring(L, mem->v1.id);
            lua_setfield(L, -2, "id");
        }
        
        if (mem->v1.description)
        {
            lua_pushstring(L, mem->v1.description);
            lua_setfield(L, -2, "id");
        }
        
        lua_pushinteger(L, mem->v1.alignment);
        lua_setfield(L, -2, "alignment");
        
        lua_pushinteger(L, mem->v1.base_address);
        lua_setfield(L, -2, "base_address");
        
        lua_pushinteger(L, mem->v1.size);
        lua_setfield(L, -2, "size");
        
        #define FUNCFIELD(name, f) lua_pushcfunction(L, f); lua_setfield(L, -2, #name);
        #define MEMFIELD(read, name, type) \
            FUNCFIELD(read##_##name##_le, hc_memory_##read##_##type##_le); \
            FUNCFIELD(read##_##name##_be, hc_memory_##read##_##type##_be)
        
        if (mem->v1.peek)
        {
            lua_pushcfunction(L, memory_peek);
            lua_setfield(L, -2, "peek");
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
            lua_setfield(L, -2, "poke");
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
        
        if (mem->v1.set_watchpoint)
        {
            lua_pushcfunction(L, memory_set_watchpoint);
            lua_setfield(L, -2, "set_watchpoint");
        }
        
        lua_createtable(L, mem->v1.num_break_points, 0);
        for (size_t i = 0; i < mem->v1.num_break_points; ++i)
        {
            if (mem->v1.break_points[i])
            {
                (void)push_breakpoint(L, mem->v1.break_points[i]);
                lua_rawseti(L, -2, i + 1);
            }
        }
        lua_setfield(L, -2, "breakpoints");
    }
    
    return 1;
}

static int push_cpu(lua_State* L, hc_Cpu const* cpu)
{
    if (lua_table_for_data(L, cpu))
    {
        lua_pushlightuserdata(L, (void*)cpu);
        lua_setfield(L, -2, USERDATA_FIELD);
                
        if (cpu->v1.description)
        {
            lua_pushstring(L, cpu->v1.description);
            lua_setfield(L, -2, "description");
        }
        
        lua_pushinteger(L, cpu->v1.type);
        lua_setfield(L, -2, "type");
        
        const char* cpu_name = retro_script_hc_get_cpu_name(cpu->v1.type);
        if (cpu_name)
        {
            lua_pushstring(L, cpu_name);
            lua_setfield(L, -2, "name");
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
                lua_setfield(L, -2, USERDATA_FIELD);
                
                lua_pushinteger(L, i);
                lua_setfield(L, -2, "_idx");
                
                const char* name = retro_script_hc_get_cpu_register_name(cpu->v1.type, i);
                if (name)
                {
                    lua_pushstring(L, name);
                    lua_setfield(L, -2, "name");
                    
                    // register name as key for register, e.g. registers.X
                    if (*name)
                    {
                        lua_pushvalue(L, -1);
                        lua_setfield(L, -3, name);
                    }
                }
                    
                if (cpu->v1.get_register)
                {
                    lua_pushcfunction(L, get_register);
                    lua_setfield(L, -2, "get");
                }
                
                if (cpu->v1.set_register)
                {
                    lua_pushcfunction(L, set_register);
                    lua_setfield(L, -2, "set");
                }
                
                if (cpu->v1.set_reg_breakpoint)
                {
                    lua_pushcfunction(L, set_register_breakpoint);
                    lua_setfield(L, -2, "watch");
                }
                
                lua_rawseti(L, -2, i + 1);
            }
            
            lua_setfield(L, -2, "registers");
        }
        
        lua_pushboolean(L, cpu->v1.is_main);
        lua_setfield(L, -2, "is_main");
        
        if (cpu->v1.memory_region)
        {
            push_memory_region(L, cpu->v1.memory_region);
            lua_setfield(L, -2, "memory");
        }
        
        if (cpu->v1.set_exec_breakpoint)
        {
            lua_pushcfunction(L, cpu_set_exec_breakpoint);
            lua_setfield(L, -2, "set_exec_breakpoint");
        }
        
        if (cpu->v1.step_into)
        {
            lua_pushcfunction(L, cpu_step_into);
            lua_setfield(L, -2, "step_into");
        }
        
        if (cpu->v1.step_over)
        {
            lua_pushcfunction(L, cpu_step_over);
            lua_setfield(L, -2, "step_over");
        }
        
        if (cpu->v1.step_out)
        {
            lua_pushcfunction(L, cpu_step_out);
            lua_setfield(L, -2, "step_out");
        }
        
        lua_createtable(L, cpu->v1.num_break_points, 0);
        for (size_t i = 0; i < cpu->v1.num_break_points; ++i)
        {
            if (cpu->v1.break_points[i])
            {
                (void)push_breakpoint(L, cpu->v1.break_points[i]);
                lua_rawseti(L, -2, i + 1);
            }
        }
        lua_setfield(L, -2, "breakpoints");
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

void retro_script_luavalue_hc_main_cpu_and_memory(lua_State* L)
{
    for (size_t i = 0; i < system->v1.num_cpus; ++i)
    {
        hc_Cpu const* cpu = system->v1.cpus[i];
        if (cpu && cpu->v1.is_main)
        {
            (void)push_cpu(L, cpu);
            lua_setfield(L, -2, "main_cpu");
            
            // add cpu memory if available
            if (cpu->v1.memory_region)
            {
                (void)push_memory_region(L, cpu->v1.memory_region);
                lua_setfield(L, -2, "main_memory");
            }
            return;
        }
    }
}
