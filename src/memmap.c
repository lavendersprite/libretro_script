#include "memmap.h"
#include "util.h"

static char** addrspaces;
static struct retro_memory_map memmap;

static void free_memmap()
{
    if (addrspaces)
    {
        for (char** addrspace = addrspaces; addrspace && *addrspace; ++addrspace)
        {
            free(*addrspace);
        }
        
        free(addrspaces);
    }
    if (memmap.num_descriptors)
    {
        free((void*)memmap.descriptors);
    }
}

char const* const* retro_script_list_memory_addrspaces()
{
    if (addrspaces)
    {
        return (char const* const*) addrspaces;
    }
    else
    {
        // (pointer to NULL indicates empty list.)
        return (char const* const*) &addrspaces;
    }
}

void retro_script_clear_memory_map()
{
    free_memmap();
}

bool retro_script_set_memory_map(struct retro_memory_map* core_memmap)
{
    if (core_memmap)
    {
        free_memmap();
        memmap.num_descriptors = core_memmap->num_descriptors;
        memmap.descriptors = malloc_array(struct retro_memory_descriptor, memmap.num_descriptors);
        
        memcpy((void*)memmap.descriptors, core_memmap->descriptors, sizeof(struct retro_memory_descriptor) * memmap.num_descriptors);
        
        // copy addrspace to static store.
        addrspaces = malloc_array(char*, memmap.num_descriptors + 1);
        memset(addrspaces, 0, sizeof(char*) * (memmap.num_descriptors + 1));
        for (size_t i = 0; i < memmap.num_descriptors; ++i)
        {
            struct retro_memory_descriptor* descriptor = (struct retro_memory_descriptor*)&memmap.descriptors[i];
            
            char** addrspace;
            for (addrspace = addrspaces; addrspace && *addrspace; ++addrspace)
            {
                if ((*addrspace == descriptor->addrspace) || strcmp(*addrspace, descriptor->addrspace) == 0)
                {
                    // reuse store entry
                    descriptor->addrspace = *addrspace;
                    goto next_descriptor;
                }
            }
            // else
            {
                // duplicate string to static store.
                *addrspace = descriptor->addrspace ? retro_script_strdup(descriptor->addrspace) : NULL;
                descriptor->addrspace = *addrspace;
            }
        
        next_descriptor:
            continue;
        }
    }
    
    return false;
}

struct retro_memory_descriptor* retro_script_memory_find_descriptor_at_address(size_t emulated_address, size_t* offset)
{
    for (size_t i = 0; i < memmap.num_descriptors; ++i)
    {
        struct retro_memory_descriptor* descriptor = (struct retro_memory_descriptor*)&memmap.descriptors[i];
        size_t addr = emulated_address & ~descriptor->disconnect;
        if (addr < descriptor->start) continue;
        addr -= descriptor->start;
        if (addr >= descriptor->len) continue;
        *offset = addr;
        return descriptor;
    }
    
    return NULL;
}

static char* get_address_from_descriptor_and_offset(struct retro_memory_descriptor* descriptor, size_t offset)
{
    if (offset > descriptor->len)
    {
        return NULL;
    }
    
    return descriptor->ptr + descriptor->offset + offset;
}

char* retro_script_memory_access(size_t emulated_address)
{
    size_t offset;
    struct retro_memory_descriptor* descriptor = retro_script_memory_find_descriptor_at_address(emulated_address, &offset);
    
    if (descriptor)
    {
        return get_address_from_descriptor_and_offset(descriptor, offset);
    }
    else
    {
        return NULL;
    }
}

#define SYS_IS_BIGENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

static char readbuff[8];
static FORCEINLINE char* readmem_chunk(size_t emulated_address, size_t count, bool flip)
{
    for (size_t i = 0; i < count; ++i)
    {
        size_t offset;
        struct retro_memory_descriptor* descriptor = retro_script_memory_find_descriptor_at_address(emulated_address, &offset);
        
        // no valid memory chunk; fail.
        if (!descriptor) return NULL;
        
        // return direct access if possible
        if (!flip && i == 0 && offset + count <= descriptor->len)
        {
            return get_address_from_descriptor_and_offset(descriptor, offset);
        }
        
        // otherwise, copy to buffer and return the buffer.
        for (; offset < descriptor->len; ++offset, ++i)
        {
            readbuff[flip ? (count - i - 1) : i] = *get_address_from_descriptor_and_offset(descriptor, offset);
        }
    }
    
    return readbuff;
}


static FORCEINLINE bool writemem_chunk(size_t emulated_address, const char* data, size_t count, bool flip)
{
    for (int k = 0; k <= 1; ++k) // on first pass, just check that there is enough room.
    {
        for (size_t i = 0; i < count; ++i)
        {
            size_t offset;
            struct retro_memory_descriptor* descriptor = retro_script_memory_find_descriptor_at_address(emulated_address, &offset);
            
            // no writeable chunk; fail.
            if (!descriptor || (descriptor->flags & RETRO_MEMDESC_CONST)) return false;
            
            if (k == 1)
            {
                // directly write.
                for (; offset < descriptor->len; ++offset, ++i)
                {
                    *get_address_from_descriptor_and_offset(descriptor, offset)
                        = data[flip ? (count - i - 1) : i];
                }
            }
            
            i += descriptor->len - offset;
        }
    }
    
    return true;
}

bool retro_script_memory_read_char(size_t emulated_address, char* out)
{
    const char* data = retro_script_memory_access(emulated_address);
    if (data)
    {
        *out = *data;
        return true;
    }
    return false;
}

bool retro_script_memory_read_byte(size_t emulated_address, unsigned char* out)
{
    return retro_script_memory_read_char(emulated_address, (char*)out);
}

bool retro_script_memory_write_char(size_t emulated_address, char in)
{
    size_t offset;
    struct retro_memory_descriptor* descriptor = retro_script_memory_find_descriptor_at_address(emulated_address, &offset);
    
    // cannot write if memory region is const
    if (!descriptor || (descriptor->flags & RETRO_MEMDESC_CONST)) return false;
    
    char* data = get_address_from_descriptor_and_offset(descriptor, offset);
    if (data)
    {
        *data = in;
        return true;
    }
    return false;
}

bool retro_script_memory_write_byte(size_t emulated_address, unsigned char in)
{
    return retro_script_memory_write_char(emulated_address, (char)in);
}

// endianness
static const int be = 0;
static const int le = 1;

#define MEMORY_READ_WRITE(type, ctype, le) \
bool retro_script_memory_read_##type##_##le(size_t emulated_address, ctype* out) \
{ \
    char* data = readmem_chunk(emulated_address, sizeof(ctype), le == SYS_IS_BIGENDIAN ); \
    if (data) \
    { \
        *out = *(ctype*)data; \
        return true; \
    } \
    return false; \
} \
bool retro_script_memory_write_##type##_##le(size_t emulated_address, ctype in) \
{ \
    return writemem_chunk(emulated_address, (char*)(void*)&in, sizeof(ctype), le == SYS_IS_BIGENDIAN); \
}

MEMORY_READ_WRITE(int16, int16_t, le);
MEMORY_READ_WRITE(int16, int16_t, be);
MEMORY_READ_WRITE(uint16, uint16_t, le);
MEMORY_READ_WRITE(uint16, uint16_t, be);
MEMORY_READ_WRITE(int32, int32_t, le);
MEMORY_READ_WRITE(int32, int32_t, be);
MEMORY_READ_WRITE(uint32, uint32_t, le);
MEMORY_READ_WRITE(uint32, uint32_t, be);
MEMORY_READ_WRITE(int64, int64_t, le);
MEMORY_READ_WRITE(int64, int64_t, be);
MEMORY_READ_WRITE(uint64, uint64_t, le);
MEMORY_READ_WRITE(uint64, uint64_t, be);
MEMORY_READ_WRITE(float32, float, le);
MEMORY_READ_WRITE(float32, float, be);
MEMORY_READ_WRITE(float64, double, le);
MEMORY_READ_WRITE(float64, double, be);