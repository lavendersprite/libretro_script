# Libretro_script

This is a zero-dependency middleware library layer intended to lie between a libretro frontend and libretro implementation.

It allows executing scripts that modify the behaviour of or extract information from a running instance. For example, a script might cause enemies to spawn in a particular NES game.

Currently, the only front-end that supports libretro_script is ludo. However, it's extremely easy to add support to a front-end. See the [frontend section below](#Usage%C32in%C32a%C32frontend).

## Writing a script

Scripts are just [lua](https://www.lua.org/) files. Lua syntax 5.4.3 is supported. How to load a script depends on the frontend. 

An example script (for `Castlevania (U).nes`):

```lua
print("castlevania dagger & infinite hearts hack")

retro.on_run_begin(
    -- callback executes once per frame
    function ()
        local game_mode = retro.read_byte(0x18)
        if game_mode == 0x5 then
            retro.write_byte(0x064, 0x02) -- enable 3x shot
            retro.write_byte(0x071, 0x63) -- 99 hearts
            retro.write_byte(0x15B, 0x08) -- subweapon dagger
        end
    end
)
```

## Reference

Values marked with an asterisk (\*) may not be available, depending on the core. It is advisable to check if they are nil before using them.

### retro.on_run_begin(callback)

runs callback directly before each update tick.

### retro.on_run_end(callback)

runs callback directly after each update tick.

### retro.read_char(address)

Reads a signed byte (-128 to +127) from the given address.

### retro.write_char(address, value)

Writes a signed byte to the given address

### retro.read_byte(address)

Reads an unsigned byte (0 to 255) from the given address

### retro.write_byte(address, value)

Writes an unsigned byte to the given address

### retro.read_int16_le(address)
### retro.read_int16_be(address)
### retro.read_uint16_le(address)
### retro.read_uint16_be(address)
### retro.write_int16_le(address, value)
### retro.write_int16_be(address, value)
### retro.write_uint16_le(address, value)
### retro.write_uint16_be(address, value)

### retro.read_int32_le(address)
### retro.read_int32_be(address)
### retro.read_uint32_le(address)
### retro.read_uint32_be(address)
### retro.write_int32_le(address, value)
### retro.write_int32_be(address, value)
### retro.write_uint32_le(address, value)
### retro.write_uint32_be(address, value)

### retro.read_int64_le(address)
### retro.read_int64_be(address)
### retro.read_uint64_le(address)
### retro.read_uint64_be(address)
### retro.write_int64_le(address, value)
### retro.write_int64_be(address, value)
### retro.write_uint64_le(address, value)
### retro.write_uint64_be(address, value)

Reads/writes a signed/unsigned 16-bit/32-bit/64-bit little-endian/big-endian value at the given address.

### retro.write_float32_le(address, value)
### retro.write_float32_be(address, value)
### retro.write_float64_le(address, value)
### retro.write_float64_be(address, value)

Reads/writes a 32/64-bit floating point value (represented as per IEEE-754).

### retro.hc

This is non-nil if the core supports [hcdebug](https://github.com/leiradel/hackable-console), allowing breakpoints and watchpoints. The following fields are available:

### retro.hc.system_get_description()

Retrieves the description for the system, such as "NES"

### retro.hc.system_get_memory_regions()

Retrieves a list of all memory regions which *are not addressable directly by a CPU*. (This typically excludes main memory! -- To access CPU-addressable memory, see `retro.hc.get_cpus()` below.) The following fields may be included:

### retro.hc.system_get_breakpoints()

Retrieves a list of special system breakpoints. The following fields may be included:

### \* breakpoint.description

### \* breakpoint:enable([yes,] callback)

*Returns:* a breakpoint handle (currently not useful).

### \* mem.id

### \* mem.description

### mem.alignment

Byte alignment for words in the memory region. Might be 1, 2, 4, 8, etc.

### mem.base_address

### mem.size

### \* mem:peek(address)

Retrieves an unsigned byte from the given address.

Also available if this is available are all the convenience access methods like
`mem:read_uint16_le`, `mem:read_float64_be`, etc. (see `retro.read_int16_le` above).

### \* mem:poke(address, value)

Writes a byte to the given address.

Also available if this is available are all the convenience access methods like
`mem:write_uint64_be`, `mem:write_float32_le`, etc. (see `retro.write_int16_le` above).

### \* mem:set_watchpoint(address, length, [type: string,] callback: () -> nil)

Sets a watchpoint callback for the given address and length. Type may be "w" (trigger on write), "r" (trigger on read), or "wr" (trigger on write or on read).

Ideally, the value written/read and the address accessed would be available, but hcdebug does not support this yet.

*Returns*: a handle for the watchpoint. (Not currently useful.)

### mem.breakpoints

A list of special breakpoints. (see `retro.hc.system_get_breakpoints()` for fields).

### retro.hc.system_get_cpus()

Retrieves a list of all emulated CPUs. The following fields may be included:

### \* cpu.description

### cpu.type

An integer indicating the type of CPU as defined in [hcdebug.h](deps/hcdebug.h).

### \* cpu.name

Hackable-console macro name for the cpu. For example, `Z80` or `6502`. Inferred from `cpu.type`.

### cpu.is_main

### \* cpu.memory

A table for the addressable memory region. See `retro.hc.system_get_memory_regions()` for a list of fields.

### \* cpu:set_exec_breakpoint(address, callback)

Sets a breakpoint which is triggered when the given address is set.

*Returns*: a breakpoint handle (currently not useful).

### cpu.breakpoints

A list of special breakpoints. (see `retro.hc.system_get_breakpoints()` for fields).

### cpu.registers

A list of registers, also mapped by name. For example, 6502 registers could be access as `cpu.registers[1]`, `cpu.registers[2]`, etc., or by name, like `cpu.registers.X` or `cpu.registers.PC`, etc. (Note: not necessarily the same registers are accessed in both examples)

The following fields may be included:

### register.name

The Hackable-console macro name for the register. Example (for 6502): `A`, `X`, `SP`, etc.

### register:get()

Retrieves the current value of the register.

### register:set(value)

Sets the current value of the register.

### register:watch(callback)

Sets a watchpoint to trigger when the register value changes.

*Returns*: breakpoint id (currently not useful).

### retro.hc.main_cpu

The first cpu marked as `is_main`.

### retro.hc.main_memory

The memory region addressed by `retro.hc.main_cpu`

## Usage in a frontend

Main reference: [libretro_script.h](include/libretro_script.h).

In brief:

```C
#include <libretro_script.h>

// after loading the core, but before running retro_init():
{
    retro_script_init();
    core.retro_set_environment = retro_script_intercept_retro_set_environment(core.retro_set_environment);
    core.retro_get_memory_data = retro_script_intercept_retro_get_memory_data(core.retro_get_memory_data);
    core.retro_get_memory_size = retro_script_intercept_retro_get_memory_size(core.retro_get_memory_size);
    core.retro_init = retro_script_intercept_retro_init(core.retro_init);
    core.retro_deinit = retro_script_intercept_retro_deinit(core.retro_deinit);
    core.retro_run = retro_script_intercept_retro_run(core.retro_run);
}
```

Build with linker flag `-lretro_script`.

## Building libretro_script

Run `make lib` or `make shlib` depending on if a static or shared library is required. There are no dependencies beyond just `gcc`.