#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <locale.h>
#include <unistd.h>

#include "utils.h"
#include "notetaking.h"
#include "popups.h"
#include "logger.h"
#include "arguments.h"

#include "note_renderer.h"

/* TODO: test this function */
void
startup_function( void ) {
	add_log( "startup_function" );

	char* home_folder = getenv( "HOME" );
	if ( home_folder == NULL ) {
		add_log( "failed to get HOME env var" );
		return;
	}

	char buffer[ 1024 ];
	sprintf( buffer, "%s/.ntp/startup_script.sh", home_folder );

	/* check if startup_script exists */
	if ( access( buffer, F_OK ) == 0 ) {

		add_log( "running startup_script" );

		/* run script */
		int sr = system( buffer );

	} else
		add_log( "startup_script not found" );

	return;
}

/* TODO: test this function */
void
exit_function( void ) {

	/* end curses mode */
	if ( stdscr ) endwin();

	add_log( "exit_function" );

	char* home_folder = getenv( "HOME" );
	if ( home_folder == NULL ) {
		add_log( "failed to get HOME env var" );
		return;
	}

	char buffer[ 1024 ];
	sprintf( buffer, "%s/.ntp/exit_script.sh", home_folder );

	/* check if exit_script exists */
	if ( access( buffer, F_OK ) == 0 ) {

		add_log( "running exit_script" );

		/* run script */
		int sr = system( buffer );

	} else
		add_log( "exit_script not found" );

	return;
}

/* TODO: get tab length and use it */

struct notes_list notes_list;

int main( int argc, char* argv[] ) {

	/* TODO: try to break the renderer  /*
	printf( "note_render_text test\n" );

	const char* testing_todos[] = {
		"[[TD|goal]]",
		"[[TD|priority|goal]]",
		"[[TD|priority|goal|status]]",
		"[[TD|priority|goal|date|status]]",

		"[[TD||goal]]",

		/* these should NOT work */ /*
		"[[not a TD||goal]]",
		"[[TD|||||||||goal]]",
		"[[TD|0|goal]]\n\t[[TD|1|goal]]\n\t[[TD|2|goal]]\n\t[[TD|3|goal]]\n\t[[TD|4|goal]]\n\t[[TD|5|goal]]\n\t[[TD|6|goal]]\n\t[[TD|7|goal]]\n\t[[TD|8|goal]]\n\t[[TD|9|goal]]",
	};
	for ( size_t i = 0; i < arraysize( testing_todos ); i++ ) {
		printf( "TODO:\n%s\n", testing_todos[ i ] );
		char* rendered_todo = note_render_text( testing_todos[ i ] );
		printf( "Rendered todo:\n%s\n", rendered_todo );
		free( rendered_todo );
	}

	return 0;
	/* */

	add_log( "starting ntp" );

	/* load config */
	config_load();

	/* start startup script */
	startup_function();

	/* register "exit_function" to be run at exit */
	atexit( exit_function );

	/* parse arguments */
	add_log( "parsing arguments" );
	parse_arguments( argc, argv );

	/* scan notes */
	add_log( "scanning notes" );
	scan_notes();

	/* set default locale */
	setlocale( LC_ALL, "" );

	/* start ncurses */
	ncurses_init();

	/* run notetaking_main */
	add_log( "ntp_main" );
	ntp_main();			

	/* start exit script */
	exit( 0 );
}