#include "arguments.h"

#include "logger.h"
#include "notetaking.h"
#include "note_renderer.h"
#include "utils.h"
#include "note_compare.h"
#include "cli.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void
parse_arguments( int argc, char* argv[] ) {
	if ( argc == 1 ) return;
	
	/* list todos */
	if ( strcmp( argv[ 1 ], "list_todos" ) == 0 ) {
		cli_list_todos( argc - 2, argv + 2 );
		exit( 0 );
	}

	/* list notes */
	if ( strcmp( argv[ 1 ], "list_notes" ) == 0 ) {
		cli_list_notes( argc - 2, argv + 2 );
		exit( 0 );
	}

	if ( argc != 2 ) {
		printf( "Usage: <exec> <command> <arguments>\n  see -h\n");
		exit( 1 );
	}

	/* help command */
	if ( strcmp( argv[ 1 ], "-h" ) == 0 || strcmp( argv[ 1 ], "--help" ) == 0 ) {
		add_log( "command: help" );
		printf(
			"notetakingprogram\n"
			"commands:\n"
			"  -h|--help - this message\n"
			"  list_todos - lists unfinished todos from all notes\n"
			"    all - lists all todos\n"
			"    --filter variable:value\n"
			"  list_notes - lists all notes and their metadata\n"
			"    --filter variable:value\n"
			"    --advanced - show additional info\n"
		);
		exit( 0 );
	}

	/* invalid command */
	printf( "invalid command: %s\n  see -h\n", argv[ 1 ] );
	exit( 1 );
}