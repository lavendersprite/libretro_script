#pragma once

#include <hcdebug.h>

const char* retro_script_hc_get_cpu_name(hc_Cpu const* cpu);
int retro_script_hc_get_cpu_register_count(hc_Cpu const* cpu); // returns -1 if unknown.
const char* retro_script_hc_get_cpu_register_name(hc_Cpu const* cpu, unsigned register_type);