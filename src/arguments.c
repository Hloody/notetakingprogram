#include "arguments.h"

#include "logger.h"
#include "notetaking.h"
#include "note_renderer.h"
#include "utils.h"
#include "note_compare.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void
parse_arguments( int argc, char* argv[] ) {
	if ( argc == 1 ) return;
	
	/* list todos */
	if ( strcmp( argv[ 1 ], "list_todos" ) == 0 ) {
		add_log( "command: list_todos" );

		struct {
			char* status;
			char* date;
			char* priority;
			char* note;
		} filter = {
			.status   = NULL,
			.date     = NULL,
			.priority = NULL,
			.note = NULL
		};
		
		struct {
			bool status;
			bool date;
			bool priority;
			bool note;
		} no_filter = {
			.status   = false,
			.date     = false,
			.priority = false,
			.note = false,
		};

		bool show_all = false;

		for ( size_t i = 2; i < argc; i++ ) {

			/* list ALL todos */
			if ( strcmp( argv[ i ], "--all" ) == 0 ) {
				add_log( "listing ALL todos" );
				no_filter.status   = true;
				no_filter.date     = true;
				no_filter.priority = true;
			} else if ( strcmp( argv[ i ], "--filter" ) == 0 ) {

				if ( i + 1 >= argc ) {
					printf( "usage of '--filter':\n  --filter variable:value\n" );
					exit( 1 );
				}

				i++;

				char* pair_start = argv[ i ];
				for ( char* separator = strchr( argv[ i ], ',' ); true; separator = strchr( pair_start, ',' ) ) {

					if ( separator == NULL ) separator = strchr( pair_start, 0 );

					char* separator_2 = strchr( pair_start, ':' );
					if ( separator_2 == NULL || separator_2 > separator ) {
						printf( "filter error: syntax variable:value - value missing\n" );
						exit( 1 );
					}

					size_t variable_size;
					char* variable;

					/* parse variable */
					variable_size = separator_2 - pair_start;
					variable = malloc( variable_size + 1 );
					strncpy( variable, pair_start, variable_size );
					variable[ variable_size ] = 0;

					/* parse value */
					char*  value;
					size_t value_size;

					value_size = separator - separator_2 - 1;

					value = malloc( value_size + 1 );
					strncpy( value, separator_2 + 1, value_size );
					value[ value_size ] = 0;

					if ( strcmp( variable, "status" ) == 0 ) {
						filter.status = value;
					} else if ( strcmp( variable, "priority" ) == 0 ) {
						filter.priority = value;
					} else if ( strcmp( variable, "date" ) == 0 ) {
						filter.date = value;
					} else if ( strcmp( variable, "note" ) == 0 ) {
						filter.note = value;
					} else {
						printf( "filter error: unknown variable '%s'\n", variable );
						exit( 1 );
					}

					pair_start = separator + 1;

					if ( *separator == 0 ) break;

				}

			} else if ( strcmp( argv[ i ], "--nofilter" ) == 0 ) {

				if ( i + 1 >= argc ) {
					printf( "usage of '--nofilter':\n  --nofilter variable\n" );
					exit( 1 );
				}

				i++;

				char* pair_start = argv[ i ];
				for ( char* separator = strchr( argv[ i ], ',' ); true; separator = strchr( pair_start, ',' ) ) {

					if ( separator == NULL ) separator = strchr( pair_start, 0 );

					size_t variable_size;
					char* variable;

					/* parse variable */
					variable_size = separator - pair_start;
					variable = malloc( variable_size + 1 );
					strncpy( variable, pair_start, variable_size );
					variable[ variable_size ] = 0;

					if ( strcmp( variable, "status" ) == 0 ) {
						no_filter.status = true;
					} else if ( strcmp( variable, "priority" ) == 0 ) {
						no_filter.priority = true;
					} else if ( strcmp( variable, "date" ) == 0 ) {
						no_filter.date = true;
					}  else if ( strcmp( variable, "note" ) == 0 ) {
						no_filter.note = true;
					} else {
						printf( "nofilter error: unknown variable '%s'\n", variable );
						exit( 1 );
					}

					pair_start = separator + 1;

					if ( *separator == 0 ) break;

				}

			} else {
				printf( "Unsupported argument: '%s'\n  see -h\n", argv[ i ] );
				exit( 1 );
			}
		}

		scan_notes();
		sort_notes( config.sort_function );

		bool found_todos = false;
		for ( size_t i = 0; i < notes_list.note_count; i++ ) {

			if ( no_filter.note == false && filter.note != NULL && strcmp( filter.note, notes_list.note_paths[ i ] ) != 0 ) continue;

			/* load note */
			struct note* note = load_note( notes_list.note_paths[ i ] );

			/* parse note */
			struct rendered_note rendered_note = note_render_text( note->content );

			/* print TODOs */
			bool has_todos = false;
			for ( size_t j = 0; j < rendered_note.count; j++ ) {
				if ( rendered_note.segments[ j ].type == st_todo ) {

					bool matches = false;

					/* check if TODO matches filters */
					if ( no_filter.status && no_filter.date && no_filter.priority ) matches = true;

					/* check status */
					if ( no_filter.status == false ) {						

						if ( filter.status != NULL ) {

							const char* status;

							if ( rendered_note.segments[ j ].todo.status == NULL ) status = "";
							else status = rendered_note.segments[ j ].todo.status;

							if ( strcmp( status, filter.status ) != 0 ) {
								/* skip */
								continue;
							}
					
							matches = true;

						} else {

							if ( rendered_note.segments[ j ].todo.status != NULL && strcmp( rendered_note.segments[ j ].todo.status, "Y" ) == 0 ) 
								continue;

							matches = true;

						}

					} else matches = true;

					/* check date */
					if ( no_filter.date == false ) {
						if ( filter.date != NULL ) {

							const char* date;

							if ( rendered_note.segments[ j ].todo.date == NULL ) date = "";
							else date = rendered_note.segments[ j ].todo.date;

							if ( strcmp( date, filter.date ) != 0 ) {
								/* skip */
								continue;
							}
					
							matches = true;

						}
					} else matches = true;

					/* check priority */
					if ( no_filter.priority == false ) {
						if ( filter.priority != NULL ) {

							if ( strcmp( rendered_note.segments[ j ].todo.priority, filter.priority ) != 0 ) {
								/* skip */
								continue;
							}

							matches = true;
						}
					} else matches = true;


					/* skip TODO if it does not match filters */
					if ( matches == false ) continue;

					found_todos = true;

					if ( has_todos == false ) {
						has_todos = true;
						printf( "Note: %s\n", notes_list.note_paths[ i ] );
					}

					printf( "  [%s]", rendered_note.segments[ j ].todo.priority );
					if ( rendered_note.segments[ j ].todo.status != NULL ) printf( "[%s]", rendered_note.segments[ j ].todo.status );
					if ( rendered_note.segments[ j ].todo.date   != NULL ) printf( "[%s]", rendered_note.segments[ j ].todo.date );
					printf( " %s\n", rendered_note.segments[ j ].todo.goal );
				}
			}

			/* free */
			free_rendered_note( rendered_note );
			free_note( note );
		}

		/* no TODOs found */
		if ( found_todos == false ) printf( "No TODOs found\n" );

		exit( 0 );
	}

	/* list notes */
	if ( strcmp( argv[ 1 ], "list_notes" ) == 0 ) {
		add_log( "command: list_notes" );

		bool show_all = false;
		bool advanced = false;

		struct {
			char* name;
			char* created;
			char* last_edit;
			char* tag;
			char* content;
		} filter = {
			.name = NULL,
			.created = NULL,
			.last_edit = NULL,
			.tag = NULL,
			.content = NULL,
		};

		for ( size_t i = 2; i < argc; i++ ) {
			/* list ALL notes */
			if ( strcmp( argv[ i ], "--all" ) == 0 ) {
				add_log( "listing ALL notes" );
				show_all = true;
			} else if ( strcmp( argv[ i ], "--advanced" ) == 0 ) {
				advanced = true;
			} else if ( strcmp( argv[ i ], "--filter" ) == 0 ) {
				if ( i + 1 >= argc ) {
					printf( "usage of '--filter':\n  --filter variable:value\n" );
					exit( 1 );
				}

				i++;

				char* pair_start = argv[ i ];
				for ( char* separator = strchr( argv[ i ], ',' ); true; separator = strchr( pair_start, ',' ) ) {

					if ( separator == NULL ) separator = strchr( pair_start, 0 );

					char* separator_2 = strchr( pair_start, ':' );
					if ( separator_2 == NULL || separator_2 > separator ) {
						printf( "filter error: syntax variable:value - value missing\n" );
						exit( 1 );
					}

					size_t variable_size;
					char* variable;

					/* parse variable */
					variable_size = separator_2 - pair_start;
					variable = malloc( variable_size + 1 );
					strncpy( variable, pair_start, variable_size );
					variable[ variable_size ] = 0;

					/* parse value */
					char*  value;
					size_t value_size;

					value_size = separator - separator_2 - 1;

					value = malloc( value_size + 1 );
					strncpy( value, separator_2 + 1, value_size );
					value[ value_size ] = 0;

					if ( strcmp( variable, "name" ) == 0 ) {
						filter.name = value;
					} else if ( strcmp( variable, "created" ) == 0 ) {
						filter.created = value;
					} else if ( strcmp( variable, "last_edit" ) == 0 ) {
						filter.last_edit = value;
					} else if ( strcmp( variable, "tag" ) == 0 ) {
						filter.tag = value;
					} else if ( strcmp( variable, "content" ) == 0 ) {
						filter.content = value;
					} else {
						printf( "filter error: unknown variable '%s'\n", variable );
						exit( 1 );
					}

					pair_start = separator + 1;

					if ( *separator == 0 ) break;

				}
			} else {
				printf( "Unsupported argument: '%s'\n  see -h\n", argv[ i ] );
				exit( 1 );
			}
		}

		/* scan notes */
		scan_notes();
		sort_notes( config.sort_function );

		/* print */
		bool found_note = false;
		if ( notes_list.note_count != 0 ) {
			
			for ( size_t i = 0; i < notes_list.note_count; i++ ) {

				if ( filter.name && strcmp( filter.name, notes_list.note_paths[ i ] ) != 0 ) continue;

				struct note* note = load_note( notes_list.note_paths[ i ] );

				if ( filter.created ) {

					struct tm created_tm;
					localtime_r( &note->created, &created_tm );

					char created[ 64 ];
					strftime( created, arraysize( created ) - 1, "%d.%m.%Y", &created_tm );

					if ( strcmp( filter.created, created ) != 0 ) continue;
				}

				if ( filter.last_edit ) {

					struct tm last_edit_tm;
					localtime_r( &note->last_edit, &last_edit_tm );

					char last_edit[ 64 ];
					strftime( last_edit, arraysize( last_edit ) - 1, "%d.%m.%Y", &last_edit_tm );

					if ( strcmp( filter.last_edit, last_edit ) != 0 ) continue;
				}

				if ( filter.tag ) {
					bool found_match = false;
					for ( size_t i = 0; i < note->tag_count; i++ ) {
						if ( strcmp( filter.tag, note->tags[ i ] ) == 0 ) {
							found_match = true;
							break;
						}
					}

					if ( found_match == false ) {
						continue;
					}
				}

				if ( filter.content ) {
					if ( strstr( note->content, filter.content ) == NULL ) continue;
				}

				if ( found_note == false ) {
					found_note = true;
					printf( "Notes:\n" );
				}

				if ( advanced ) {

					struct note* note = load_note( notes_list.note_paths[ i ] );

					struct tm created_tm;
					localtime_r( &note->created, &created_tm );
					char created[ 64 ];
					strftime( created, arraysize( created ) - 1, "%d.%m.%Y %H:%M:%S", &created_tm );

					struct tm last_edit_tm;
					localtime_r( &note->last_edit, &last_edit_tm );
					char last_edit[ 64 ];
					strftime( last_edit, arraysize( last_edit ) - 1, "%d.%m.%Y %H:%M:%S", &last_edit_tm );

					/* print */
					printf(
						"  %s:\n"
						"    Metadata:\n"
						"      Created:   %s\n"
						"      Last Edit: %s\n"
						,
						notes_list.note_paths[ i ],
						created,
						last_edit
					);

					if ( note->tag_count != 0 ) {
						printf( "      Tags (%lu): ", note->tag_count );
						for ( size_t i = 0; i < note->tag_count; i++ ) {
							printf( "'%s' ", note->tags[ i ] );
						}
						printf( "\n" );
					}

					printf( "    Size: %lu B\n", note->size );

					free_note( note );

				} else {
					printf( "  %s\n", notes_list.note_paths[ i ] );
				}

			}
		}

		if ( found_note == false ) {
			printf( "No notes found.\n" );
		}

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
			"commands:\n"
			"  -h|--help - this message\n"
			"  list_todos - lists unfinished todos from all notes\n"
			"    all - lists all todos\n"
			"  list_notes - lists all notes and their metadata\n"
		);
		exit( 0 );
	}

	/* invalid command */
	printf( "invalid command: %s\n", argv[ 1 ] );
	exit( 1 );
}