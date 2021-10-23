#include "hashmap.h"
#include "util.h"

#include <stdlib.h>

#define HASHTABLE_SIZE 256

typedef struct hashmap_entry_header
{
    const size_t index;
    struct hashmap_entry_header* next;
    
    // ... and data_size bytes of user data.
} hashmap_entry_header;

struct retro_script_hashmap
{
    size_t data_size;
    hashmap_entry_header* table[HASHTABLE_SIZE];
};

struct retro_script_hashmap*
retro_script_hashmap_create(size_t sizeof_data)
{
    struct retro_script_hashmap* map = alloc(struct retro_script_hashmap);
    memset(map, 0, sizeof(*map));
    *(size_t*)&map->data_size = sizeof_data;
    return map;
}

void* retro_script_hashmap_add(struct retro_script_hashmap* map, size_t index)
{
    hashmap_entry_header** entry = &map->table[index * HASHTABLE_SIZE];
    while (*entry)
    {
        if ((*entry)->index == index)
        {
            return NULL;
        }
        if ((*entry)->index > index)
        {
            break;
        }
        entry = &(*entry)->next;
    }
    
    // create a new entry
    hashmap_entry_header* new_entry = malloc(sizeof(hashmap_entry_header) + map->data_size);
    if (!new_entry) return NULL;
    
    // insert into list
    *(size_t*)&new_entry->index = index;
    new_entry->next = *entry ? (*entry)->next : NULL;
    *entry = new_entry;
    
    return ((void*)new_entry) + sizeof(hashmap_entry_header);
}

void* retro_script_hashmap_get(struct retro_script_hashmap const* map, size_t index)
{
    hashmap_entry_header* entry = map->table[index * HASHTABLE_SIZE];
    while (entry)
    {
        if (entry->index == index)
        {
            return ((void*)entry) + sizeof(*entry);
        }
        if (entry->index > index)
        {
            break;
        }
        entry = entry->next;
    }
    
    return NULL;
}

int retro_script_hashmap_remove(struct retro_script_hashmap* map, size_t index)
{
    hashmap_entry_header** entry = &map->table[index * HASHTABLE_SIZE];
    while (*entry)
    {
        if ((*entry)->index == index)
        {
            goto REMOVE;
        }
        if ((*entry)->index > index)
        {
            break;
        }
        entry = &(*entry)->next;
    }
    
    return 0;
    
REMOVE:
    {
        hashmap_entry_header* remove_entry = *entry;
        *entry = (*entry)->next;
        
        free(remove_entry);
        return 1;
    }
}

void retro_script_hashmap_destroy(struct retro_script_hashmap* map)
{
    for (size_t i = 0; i < HASHTABLE_SIZE; ++i)
    {
        hashmap_entry_header* entry = map->table[i];
        while (entry)
        {
            hashmap_entry_header* remove_entry = entry;
            entry = entry->next;
            free(remove_entry);
        }
    }
    
    free(map);
}