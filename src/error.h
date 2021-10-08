#pragma once

#define set_error           retro_script_set_error
#define set_error_nofree    retro_script_set_error_nofree
#define clear_error         retro_script_clear_error

void retro_script_set_error(const char*);
void retro_script_set_error_nofree(const char*);
void retro_script_clear_error();