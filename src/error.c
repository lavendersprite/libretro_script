#include "libretro_script.h"
#include "error.h"
#include "util.h"

static int error_managed = 0;
static const char* error_text = NULL;

void retro_script_clear_error()
{
    if (error_managed)
    {
        error_managed = 0;
        free((char*)error_text);
    }
    
    error_text = NULL;
}

void retro_script_set_error(const char* s)
{
    error_text = _strdup(s);
    if (error_text)
    {
        error_managed = 1;
    }
    else
    {
        retro_script_set_error_nofree("Insufficient memory to allocate error description.");
    }
}

void retro_script_set_error_nofree(const char* s)
{
    retro_script_clear_error();
    error_managed = 0;
    error_text = s;
}

const char* retro_script_get_error()
{
    return error_text;
}