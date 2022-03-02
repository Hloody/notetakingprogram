#include "note_compare.h"

#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>

#include "notetaking.h"
#include "utils.h"
#include "logger.h"

#include <stdlib.h>

void
sort_notes( enum sort_function p_sf ) {

	add_log( "sorting notes" );
/*
	for ( size_t i = 0; i < notes_list.note_count; i++ ) {
		char a[ 128 ];
		sprintf( a, "%lu > %s", &notes_list.note_paths[ i ], notes_list.note_paths[ i ] );
		add_log( a );
	}

	/* NOTE: sorting just broke and i don't know why */
	qsort( notes_list.note_paths, notes_list.note_count, sizeof( char* ), note_compare_get_function( p_sf ) );
/*
	for ( size_t i = 0; i < notes_list.note_count; i++ ) {
		char a[ 128 ];
		sprintf( a, "%lu > %s", &notes_list.note_paths[ i ], notes_list.note_paths[ i ] );
		add_log( a );
	}
*/
	return;
}

int ( *note_compare_get_function( enum sort_function p_sf ) )( const void*, const void* ) {
	switch ( p_sf ) {
		case sort_alphabet_ascending:  return note_compare_function_alphabetic_ascending;
		case sort_alphabet_descending: return note_compare_function_alphabetic_descending;
	}

	/* default sorting function */
	return note_compare_function_alphabetic_ascending;
}

int
note_compare_function_alphabetic_ascending( const void* p_1, const void* p_2 ) {
	char* n_1 = p_1;
	char* n_2 = p_2;

	char n_1_path[ strlen( n_1 ) + 1 ];
	char n_2_path[ strlen( n_2 ) + 1 ];

	strcpy( n_1_path, n_1 );
	strcpy( n_2_path, n_2 );

	for ( size_t i = 0; i < arraysize( n_1_path ) - 1; i++ ) n_1_path[ i ] = tolower( n_1_path[ i ] );
	for ( size_t i = 0; i < arraysize( n_2_path ) - 1; i++ ) n_2_path[ i ] = tolower( n_2_path[ i ] );

/*
	char a[ 1024 ];
	sprintf( a, "\na %lu: %s\nb %lu: %s", p_1, n_1, p_2, n_2 );
	add_log( a );
*/
	return strcmp( n_1_path, n_2_path );
}

int
note_compare_function_alphabetic_descending( const void* p_1, const void* p_2 ) {
	/* switch p_1 and p_2 */
	return note_compare_function_alphabetic_ascending( p_2, p_1 );
}