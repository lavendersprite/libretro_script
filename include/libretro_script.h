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

// returns error text if an error occured, or nullptr if no error.
RETRO_SCRIPT_API const char* retro_script_get_error();

#define RETRO_SCRIPT_DECLT(name) decl_##name##_t
#define RETRO_SCRIPT_INTERCEPT(rtype, name, ...) \
typedef rtype (RETRO_CALLCONV *RETRO_SCRIPT_DECLT(name))(__VA_ARGS__); \
RETRO_SCRIPT_API RETRO_SCRIPT_DECLT(name) retro_script_intercept_##name(RETRO_SCRIPT_DECLT(name));

// these intercept replacements should all be called before retro_init
// example usage: core.retro_set_environment = retro_script_intercept_retro_set_environment(core.retro_set_environment)
RETRO_SCRIPT_INTERCEPT(void, retro_set_environment, retro_environment_t);
RETRO_SCRIPT_INTERCEPT(void*, retro_get_memory_data, unsigned id);
RETRO_SCRIPT_INTERCEPT(size_t, retro_get_memory_size, unsigned id);
RETRO_SCRIPT_INTERCEPT(void, retro_run, void);

typedef uint32_t retro_script_id_t;

// loads a script, returning a handle.
// returns 0 if script load fails.
RETRO_SCRIPT_API retro_script_id_t retro_script_load(const char* path_to_script);

#ifdef __cplusplus
}
#endif

#endif