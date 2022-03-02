#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <ncurses.h>

#include "logger.h"
#include "popups.h"

void
rec_mkdir( const char* p_path, mode_t p_mode ) {

	char path[ strlen( p_path ) + 1 ];
	strcpy( path, p_path );

    char *sep = strrchr( path, '/' );
    if ( sep != NULL && sep != path ) {
        *sep = 0;
        rec_mkdir( path, p_mode );
        *sep = '/';
    }
    if ( mkdir( path, p_mode ) && errno != EEXIST ) {
		if ( stdscr ) endwin();
		printf( "error while trying to create '%s', errno: %i\n", path, errno );
		exit( 1 );
	} 

	return;
}

const char*
get_notes_dir( void ) {

	static const char* output = NULL;

	if ( output == NULL ) {

		char* home_env = getenv( "HOME" );
		if ( home_env == NULL ) {
			if ( stdscr ) endwin();
			puts( "Failed to load notes:\nenviromant variable 'HOME' does not exist" );
			exit( 1 );
		}

		const char* rel_note_path = "/.ntp/notes";
		/* static char ntp_dir[ strlen( home_env ) + strlen( rel_note_path ) + 1 ]; not constant */
		static char ntp_dir[ 1024 ];
		strcpy( ntp_dir, home_env );
		strcat( ntp_dir, rel_note_path );

		output = ntp_dir;
	}

	return output;
}

int
get_input( void ) {
	char buffer[ 100 ] = { 0 };

	int size = read( 0, buffer, sizeof( buffer ) - 1 );

	/*
	char temp[ 1025 ] = { 0 };
	strcpy( temp, "get_input recieved: " );
	for ( size_t i = 0; i < strlen( buffer ); i++ ) {
		char temp2[ 24 ];
		sprintf( temp2, "%d", buffer[ i ] );
		strcat( temp, temp2 );
		strcat( temp, " " );
	}
	popup_notification( temp );

	return 0;
	*/

	if ( size == 0 ) return 0;

	// Check for ESC
	if ( size == 1 && buffer[ 0 ] == 0x1B )
		return KEY_ESCAPE;

	/* handle other escape codes */
	if ( size == 2 && buffer[ 0 ] == 0x1B ) {
		return CTRL( buffer[ 1 ] );
	}

	if ( buffer[ 0 ] == 27 && buffer[ 1 ] == 79 && buffer[ 2 ] == 68 ) return KEY_LEFT;
	if ( buffer[ 0 ] == 27 && buffer[ 1 ] == 79 && buffer[ 2 ] == 66 ) return KEY_DOWN;
	if ( buffer[ 0 ] == 27 && buffer[ 1 ] == 79 && buffer[ 2 ] == 67 ) return KEY_RIGHT;
	if ( buffer[ 0 ] == 27 && buffer[ 1 ] == 79 && buffer[ 2 ] == 65 ) return KEY_UP;

	if ( buffer[ 0 ] == 27 && buffer[ 1 ] == 91 && buffer[ 2 ] == 51 && buffer[ 3 ] == 126 ) return KEY_DC; /* DELETE */

	if ( buffer[ 0 ] == '\r' ) return '\n';
	if ( buffer[ 0 ] == 127 ) return KEY_BACKSPACE;

	/* return input */
	return buffer[ 0 ];
}

void
ncurses_init( void ) {

	add_log( "ncurses_init" );

	/* start ncurses */
	initscr();

	clear();

	noecho();
	
	keypad( stdscr, TRUE );

	raw();

	nonl();

	cbreak();

	refresh();

	/* disable cursor */
	/*  NOTE: does not work in VSCode terminal */
	if ( curs_set( 0 ) == ERR )
		popup_notification( "Your terminal does not support invisible cursors" );

	/* init colour pairs */
	if ( has_colors() == FALSE ) {
		endwin();
		printf( "Your terminal does not support colours\n" );
		exit( 1 );
	}

	/* makes '-1' colour work */
	use_default_colors();

	start_color();

	if ( config.light_mode ) {
		init_color( COLOR_BLACK, 1000, 1000, 1000 );
		init_color( COLOR_DARK_GRAY,  750, 750, 750 );
		init_color( COLOR_GRAY,  500, 500, 500 );
		init_color( COLOR_WHITE, 0, 0, 0 );
	} else {
		init_color( COLOR_BLACK, 0, 0, 0 );
		init_color( COLOR_DARK_GRAY,  250, 250, 250 );
		init_color( COLOR_GRAY,  500, 500, 500 );
		init_color( COLOR_WHITE, 1000, 1000, 1000 );
	}

	if ( config.colour_background ) {
		init_pair( DEFAULT_PAIR, COLOR_WHITE, COLOR_BLACK );

		init_pair( TEXT_HIDDEN, COLOR_GRAY, COLOR_BLACK );
		
	} else {
		init_pair( DEFAULT_PAIR, -1, -1 );

		init_pair( TEXT_HIDDEN, COLOR_GRAY, -1 );
	}

	init_pair( SELECTOR_PAIR,         COLOR_BLACK,  COLOR_WHITE );
	init_pair( HIGHLIGHT_PAIR,        COLOR_BLACK,  COLOR_GRAY  );
	init_pair( TEXT_HIGHLIGHT,        COLOR_BLACK,  COLOR_WHITE );
	init_pair( TEXT_HIDDEN_HIGHLIGHT, COLOR_GRAY,   COLOR_WHITE );
	init_pair( FOOTER_PAIR,           COLOR_BLACK,  COLOR_GRAY  );

	init_pair( HIGHLIGHT_SEGMENT_PAIR, COLOR_WHITE, COLOR_DARK_GRAY );

	attron( COLOR_PAIR( DEFAULT_PAIR ) );

	return;
}

const char*
text_editor_to_string( enum text_editor_program p_te ) {
	switch ( p_te ) {
		case text_editor_builtin: return "builtin";
		case text_editor_vim:     return "vim";	
		case text_editor_nano:    return "nano";
	}

	return "unknown";
}

const char*
display_to_string( enum display_program p_d ) {
	switch ( p_d ) {
		case display_builtin: return "builtin";
		case display_less:    return "less";
	}

	return "unknown";
}

const char*
sort_function_to_string( enum sort_function p_sf ) {
	switch ( p_sf ) {
		case sort_alphabet_ascending:  return "alphabetical ascending";
		case sort_alphabet_descending: return "alphabetical descending";
	}

	return "unknown";
}

void
fill_screen( void ) {

	wbkgd( stdscr, COLOR_PAIR( DEFAULT_PAIR ) );

	return;
}