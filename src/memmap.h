#pragma once

#include <libretro.h>
#include <stdlib.h>

bool retro_script_set_memory_map(struct retro_memory_map*);
void retro_script_clear_memory_map();

char const* const* retro_script_list_memory_addrspaces();
struct retro_memory_descriptor* retro_script_memory_find_descriptor_at_address(size_t emulated_address, size_t* offset);
char* retro_script_memory_access(size_t emulated_address);

// these all return false if an error occurred, true if successful.
bool retro_script_memory_read_char(size_t emulated_address, char* out);
bool retro_script_memory_read_byte(size_t emulated_address, unsigned char* out);
bool retro_script_memory_read_int16_le(size_t emulated_address, int16_t* out);
bool retro_script_memory_read_int16_be(size_t emulated_address, int16_t* out);
bool retro_script_memory_read_uint16_le(size_t emulated_address, uint16_t* out);
bool retro_script_memory_read_uint16_be(size_t emulated_address, uint16_t* out);
bool retro_script_memory_read_int32_le(size_t emulated_address, int32_t* out);
bool retro_script_memory_read_int32_be(size_t emulated_address, int32_t* out);
bool retro_script_memory_read_uint32_le(size_t emulated_address, uint32_t* out);
bool retro_script_memory_read_uint32_be(size_t emulated_address, uint32_t* out);
bool retro_script_memory_read_int64_le(size_t emulated_address, int64_t* out);
bool retro_script_memory_read_int64_be(size_t emulated_address, int64_t* out);
bool retro_script_memory_read_uint64_le(size_t emulated_address, uint64_t* out);
bool retro_script_memory_read_uint64_be(size_t emulated_address, uint64_t* out);
bool retro_script_memory_read_float32_le(size_t emulated_address, float* out);
bool retro_script_memory_read_float32_be(size_t emulated_address, float* out);
bool retro_script_memory_read_float64_le(size_t emulated_address, double* out);
bool retro_script_memory_read_float64_be(size_t emulated_address, double* out);

bool retro_script_memory_write_char(size_t emulated_address, char in);
bool retro_script_memory_write_byte(size_t emulated_address, unsigned char in);
bool retro_script_memory_write_int16_le(size_t emulated_address, int16_t in);
bool retro_script_memory_write_int16_be(size_t emulated_address, int16_t in);
bool retro_script_memory_write_uint16_le(size_t emulated_address, uint16_t in);
bool retro_script_memory_write_uint16_be(size_t emulated_address, uint16_t in);
bool retro_script_memory_write_int32_le(size_t emulated_address, int32_t in);
bool retro_script_memory_write_int32_be(size_t emulated_address, int32_t in);
bool retro_script_memory_write_uint32_le(size_t emulated_address, uint32_t in);
bool retro_script_memory_write_uint32_be(size_t emulated_address, uint32_t in);
bool retro_script_memory_write_int64_le(size_t emulated_address, int64_t in);
bool retro_script_memory_write_int64_be(size_t emulated_address, int64_t in);
bool retro_script_memory_write_uint64_le(size_t emulated_address, uint64_t in);
bool retro_script_memory_write_uint64_be(size_t emulated_address, uint64_t in);
bool retro_script_memory_write_float32_le(size_t emulated_address, float in);
bool retro_script_memory_write_float32_be(size_t emulated_address, float in);
bool retro_script_memory_write_float64_le(size_t emulated_address, double in);
bool retro_script_memory_write_float64_be(size_t emulated_address, double in);