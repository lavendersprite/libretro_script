#pragma once

#include <hcdebug.h>

const char* retro_script_hc_get_cpu_name(unsigned type);
int retro_script_hc_get_cpu_register_count(unsigned type); // returns -1 if unknown.
const char* retro_script_hc_get_cpu_register_name(unsigned cpu_type, unsigned register_type);