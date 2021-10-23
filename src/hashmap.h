#pragma once

#include <stddef.h>

struct retro_script_hashmap;

struct retro_script_hashmap* retro_script_hashmap_create(size_t sizeof_data);
void* retro_script_hashmap_add(struct retro_script_hashmap*, size_t index); // returns nullptr if already exists
void* retro_script_hashmap_get(struct retro_script_hashmap const*, size_t index); // returns nullptr if not in map.
int retro_script_hashmap_remove(struct retro_script_hashmap*, size_t index); // returns 1 if removed.
void retro_script_hashmap_destroy(struct retro_script_hashmap*);