#ifndef LIBRETRO_SCRIPT_H_
#define LIBRETRO_SCRIPT_H_

#include <libretro.h>
//#include <hcdebug.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RETRO_SCRIPT_API
#   if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) 
#        ifdef __GNUC__
#            define RETRO_SCRIPT_API RETRO_CALLCONV __attribute__((__dllexport__))
#        else
#            define RETRO_SCRIPT_API RETRO_CALLCONV __declspec(dllexport)
#        endif
#    else
#        if defined(__GNUC__) && __GNUC__ >= 4
#            define RETRO_SCRIPT_API RETRO_CALLCONV __attribute__((__visibility__("default")))
#        else
#            define RETRO_SCRIPT_API RETRO_CALLCONV
#        endif
#    endif
#endif

// incremented only for backward-incompatible changes
#define RETRO_SCRIPT_API_VERSION         1

RETRO_SCRIPT_API void retro_script_init();
RETRO_SCRIPT_API void retro_script_deinit();

RETRO_SCRIPT_API void retro_script_load(const char* path_to_script);

#ifdef __cplusplus
}
#endif

#endif