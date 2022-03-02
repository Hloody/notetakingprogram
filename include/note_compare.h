#pragma once

#include "config.h"

void sort_notes( enum sort_function );

int ( *note_compare_get_function( enum sort_function ) )( const void*, const void* );

/* functions for qsort */
int note_compare_function_alphabetic_ascending( const void*, const void* );
int note_compare_function_alphabetic_descending( const void*, const void* );