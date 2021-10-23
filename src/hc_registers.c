#include "hc_registers.h"

#include <stddef.h>

const char* retro_script_hc_get_cpu_name(unsigned type)
{
    #define CASE(NAME) case HC_CPU_##NAME: return #NAME;
    switch (type)
    {
    CASE(Z80);
    CASE(6502);
    default: return NULL;
    }
    #undef CASE
}

int retro_script_hc_get_cpu_register_count(unsigned type)
{
    #define CASE(NAME) case HC_CPU_##NAME: return HC_##NAME##_NUM_REGISTERS;
    switch(type)
    {
    CASE(Z80);
    CASE(6502);
    default: return -1;
    }
    #undef CASE
}

const char* retro_script_hc_get_cpu_register_name(unsigned cpu_type, unsigned register_type)
{
    #define CPUSHIFT 8
    if (register_type >= (1 << CPUSHIFT)) return NULL;
    
    #define COMBINE(cpu, reg) (((uint64_t)cpu << (uint64_t)CPUSHIFT) | ((uint64_t)reg & (uint64_t)((1 << CPUSHIFT) - 1)))
    #define CASE(CPU, REG) case COMBINE(HC_CPU_##CPU, HC_##CPU##_##REG): return #REG;
    switch(COMBINE(cpu_type, register_type))
    {
        CASE(Z80, A);
        CASE(Z80, F);
        CASE(Z80, BC);
        CASE(Z80, DE);
        CASE(Z80, HL);
        CASE(Z80, IX);
        CASE(Z80, IY);
        CASE(Z80, AF2);
        CASE(Z80, BC2);
        CASE(Z80, DE2);
        CASE(Z80, HL2);
        CASE(Z80, I);
        CASE(Z80, R);
        CASE(Z80, SP);
        CASE(Z80, PC);
        CASE(Z80, IFF);
        CASE(Z80, IM);
        CASE(Z80, WZ);
        
        CASE(6502, A);
        CASE(6502, X);
        CASE(6502, Y);
        CASE(6502, S);
        CASE(6502, PC);
        CASE(6502, P);
        default: return NULL;
    }
    #undef COMBINE
    #undef CASE
}