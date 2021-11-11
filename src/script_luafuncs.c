#include "script_luafuncs.h"
#include "memmap.h"
#include "core.h"

#include <lua_5.4.3.h>

int retro_script_luafunc_input_poll(lua_State* L)
{
    if (frontend_callbacks.retro_input_poll)
    {
        frontend_callbacks.retro_input_poll();
    }
    return 0;
}

int retro_script_luafunc_input_state(lua_State* L)
{
    // validate args
    int n = lua_gettop(L);
    if (n != 4) return 0; // number of args
    if (!lua_isinteger(L, 1)) return 0; // port
    if (!lua_isinteger(L, 2)) return 0; // device
    if (!lua_isinteger(L, 3)) return 0; // index
    if (!lua_isinteger(L, 4)) return 0; // id
    
    if (frontend_callbacks.retro_input_state)
    {
        int16_t result = frontend_callbacks.retro_input_state(
            lua_tointeger(L, 1),
            lua_tointeger(L, 2),
            lua_tointeger(L, 3),
            lua_tointeger(L, 4)
        );
        
        lua_pushinteger(L, result);
        return 1;
    }
    else
    {
        return 0;
    }
}

void retro_script_luafield_constants(struct lua_State* L)
{
    // adds the macro twice, once with the RETRO_ prefix cropped.
    #define REGISTER_INT_MACRO(macro) \
        lua_pushinteger(L, macro); \
        lua_setfield(L, -2, #macro); \
        if (strncmp(#macro, "RETRO_", strlen("RETRO_")) == 0) { \
            lua_pushinteger(L, macro); \
            lua_setfield(L, -2, #macro + strlen("RETRO_")); \
        }
    
    REGISTER_INT_MACRO(RETRO_DEVICE_NONE);
    REGISTER_INT_MACRO(RETRO_DEVICE_JOYPAD);
    REGISTER_INT_MACRO(RETRO_DEVICE_MOUSE);
    REGISTER_INT_MACRO(RETRO_DEVICE_KEYBOARD);
    REGISTER_INT_MACRO(RETRO_DEVICE_LIGHTGUN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ANALOG);
    REGISTER_INT_MACRO(RETRO_DEVICE_POINTER);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_B);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_Y);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_SELECT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_START);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_UP);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_DOWN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_LEFT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_A);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_L);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_R);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_L2);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_R2);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_L3);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_R3);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_MASK);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_INDEX_ANALOG_LEFT);
    REGISTER_INT_MACRO(RETRO_DEVICE_INDEX_ANALOG_RIGHT);
    REGISTER_INT_MACRO(RETRO_DEVICE_INDEX_ANALOG_BUTTON);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_ANALOG_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_ANALOG_Y);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_Y);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_LEFT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_RIGHT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_WHEELUP);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_WHEELDOWN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_MIDDLE);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_BUTTON_4);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_BUTTON_5);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_TRIGGER);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_RELOAD);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_AUX_A);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_AUX_B);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_START);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_SELECT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_AUX_C);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_DPAD_UP);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_DPAD_DOWN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_DPAD_LEFT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_DPAD_RIGHT);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_POINTER_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_POINTER_Y);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_POINTER_PRESSED);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_POINTER_COUNT);
    
    REGISTER_INT_MACRO(RETRO_REGION_NTSC);
    REGISTER_INT_MACRO(RETRO_REGION_PAL);
}

int retro_script_luafunc_memory_read_char(lua_State* L)
{
    int n = lua_gettop(L);
    if (n > 0 && lua_isinteger(L, -1))
    {
        lua_Integer addr = lua_tointeger(L, -1);
        if (addr < 0) return 0; // invalid usage
        
        char out;
        if (retro_script_memory_read_char(addr, &out))
        {
            lua_pushinteger(L, out);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

int retro_script_luafunc_memory_read_byte(lua_State* L)
{
    int n = lua_gettop(L);
    if (n >= 1 && lua_isinteger(L, -1))
    {
        lua_Integer addr = lua_tointeger(L, -1);
        if (addr < 0) return 0; // invalid usage
        
        unsigned char out;
        if (retro_script_memory_read_byte(addr, &out))
        {
            lua_pushinteger(L, out);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

int retro_script_luafunc_memory_write_char(lua_State* L)
{
    int n = lua_gettop(L);
    if (n >= 2 && lua_isinteger(L, -1) && lua_isinteger(L, -2))
    {
        lua_Integer addr = lua_tointeger(L, -2);
        if (addr < 0) return 0; // invalid usage
        
        char in = lua_tointeger(L, -1);
        lua_pushinteger(L,
            retro_script_memory_write_char(addr, in)
        );
        
        return 1;
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

int retro_script_luafunc_memory_write_byte(lua_State* L)
{
    int n = lua_gettop(L);
    if (n >= 2 && lua_isinteger(L, -1) && lua_isinteger(L, -2))
    {
        lua_Integer addr = lua_tointeger(L, -2);
        if (addr < 0) return 0; // invalid usage
        
        unsigned char in = lua_tointeger(L, -1);
        lua_pushinteger(L,
            retro_script_memory_write_byte(addr, in)
        );
        
        return 1;
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

#define DEFINE_LUAFUNCS_MEMORY_ACCESS(type, ctype, luatype) \
    DEFINE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
    DEFINE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, be)

#define DEFINE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
int retro_script_luafunc_memory_read_##type##_##le(lua_State* L) \
{ \
    int n = lua_gettop(L); \
    if (n >= 1 && lua_isinteger(L, -1)) \
    { \
        lua_Integer addr = lua_tointeger(L, -1); \
        if (addr < 0) return 0; \
        ctype out; \
        if (retro_script_memory_read_##type##_##le(addr, &out)) \
        { \
            lua_push##luatype(L, out); \
            return 1; \
        } \
    } \
    return 0; \
} \
int retro_script_luafunc_memory_write_##type##_##le(lua_State* L) \
{ \
    int n = lua_gettop(L); \
    if (n >= 2 && lua_isinteger(L, -2) && lua_is##luatype(L, -1)) \
    { \
        lua_Integer addr = lua_tointeger(L, -2); \
        if (addr < 0) return 0; \
        ctype in = lua_to##luatype(L, -1); \
        lua_pushinteger(L, \
            retro_script_memory_write_##type##_##le(addr, in) \
        ); \
        return 1; \
    } \
    return 0; \
}

DEFINE_LUAFUNCS_MEMORY_ACCESS(int16, int16_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(uint16, uint16_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(int32, int32_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(uint32, uint32_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(int64, int64_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(uint64, uint64_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(float32, float, number);
DEFINE_LUAFUNCS_MEMORY_ACCESS(float64, double, number);