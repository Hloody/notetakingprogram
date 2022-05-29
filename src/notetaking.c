#include "notetaking.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>

/* opendir, readdir */
#include <sys/types.h>
#include <dirent.h>

#include <jansson.h>

#include "popups.h"
#include "utils.h"
#include "logger.h"
#include "note_renderer.h"
#include "config.h"
#include "note_compare.h"

/*
TODO: add subtodo support
*/

void
ntp_main( void ) {
	int x, y;

	int screen_selector = 0;
	size_t selector = 0;

	size_t note_selector = 0;
	struct note* note = NULL;

	struct rendered_note rendered_note = { .count = 0 };

	size_t selected_note = 0;

	bool preview_force_refresh = false;

	while ( 1 ) {
		clear();
		fill_screen();

		x = getmaxx( stdscr ); y = getmaxy( stdscr );

		attron( COLOR_PAIR( DEFAULT_PAIR ) );

		if ( screen_selector == 0 && selector >= notes_list.note_count ) selector = notes_list.note_count - 1;

		/* split screen in two */
		int hx = x / 2;
		move( 0, hx );
		vline( '|', y - 1 );

		/* left side = note selector (something like ranger) */
		if ( notes_list.note_count == 0 ) {
			const char* message = "No notes found";
			move( y / 2, ( hx - strlen( message ) ) / 2 );
			if ( screen_selector == 0 ) attron( COLOR_PAIR( SELECTOR_PAIR ) );
			printw( message );
			if ( screen_selector == 0 ) {
				attroff( COLOR_PAIR( SELECTOR_PAIR ) );
				attron( COLOR_PAIR( DEFAULT_PAIR ) );
			}
		} else {
			int offset = 0;

			if ( selector + 1 > y - 3 ) offset = y - 4 - selector;

			/*
			move( y - 2, x - 50 );
			printw( "offset: %i; selector: %i; first index: %i", offset, selector, 0 - offset );
			*/

			for ( int i = 0; i < min( notes_list.note_count, y - 2 ); i++ ) {

				if ( i - offset == notes_list.note_count ) break;

				move( 1 + i, 1 );

				if ( note_selector == ( i - offset ) ) {
					if ( screen_selector == 0 ) {
						attroff( COLOR_PAIR( DEFAULT_PAIR ) );
						attron( COLOR_PAIR( SELECTOR_PAIR ) );
					} else {
						attroff( COLOR_PAIR( DEFAULT_PAIR ) );
						attron( COLOR_PAIR( HIGHLIGHT_PAIR ) );
					}

				}

				addnstr( notes_list.note_paths[ i - offset ], hx - 1 );

				if ( note_selector == ( i - offset ) ) {
					if ( screen_selector == 0 ) {
						attroff( COLOR_PAIR( SELECTOR_PAIR ) );
						attron( COLOR_PAIR( DEFAULT_PAIR ) );
					} else {
						attroff( COLOR_PAIR( HIGHLIGHT_PAIR ) );
						attron( COLOR_PAIR( DEFAULT_PAIR ) );
					}
				}

				continue;
			}
		}
		

		/* right side - note preview */
		if ( config.show_note_preview ) {

			if ( notes_list.note_count == 0 ) {
				move( 0, 1 + hx );
				attron( COLOR_PAIR( SELECTOR_PAIR ) );
				addstr( "No note selected" );
				attroff( COLOR_PAIR( SELECTOR_PAIR ) );

			} else {

				/* get note contents */
				if ( note_selector != selected_note || preview_force_refresh ) {
					if ( note != NULL ) {
						free_note( note );
						note = NULL;
					}
					selected_note = note_selector;

					preview_force_refresh = false;
				}

				if ( note == NULL ) {
					note = load_note( notes_list.note_paths[ selected_note ] );

					if ( note == NULL ) {
						popup_notification( "Failed to open/read selected note file." );
						break;
					} else {

						/* render note */
						if ( config.render_notes ) {
							free_rendered_note( rendered_note );
							rendered_note = note_render_text( note->content );
						} else {
							rendered_note.count = 1;
							rendered_note.segments = malloc( sizeof( struct note_segment ) );
							rendered_note.segments[ 0 ].type = st_text;
							rendered_note.segments[ 0 ].text = note->content;
						}
						
					}

				}

				size_t row_y = 0;

				/* print note preview */
				/* TODO: move to a function to be called everytime a note needs to be shown (previews, note display,...) */
				/* TODO: implement scrolling */
				/* print note name */
				move( row_y++, 1 + hx );
				attron( COLOR_PAIR( HIGHLIGHT_PAIR ) );
				addstr( "Note name:" );
				attroff( COLOR_PAIR( HIGHLIGHT_PAIR ) );
				attron( COLOR_PAIR( DEFAULT_PAIR ) );
				addch( ' ' );

				if ( screen_selector == 1 ) attron( COLOR_PAIR( SELECTOR_PAIR ) );
				addnstr( notes_list.note_paths[ selected_note ], x - hx - strlen( "Note name: " ) - 1 );
				if ( screen_selector == 1 ) {
					attroff( COLOR_PAIR( SELECTOR_PAIR ) );
					attron( COLOR_PAIR( DEFAULT_PAIR ) );
				}

				/* print metadata */
				move( row_y++, 1 + hx );
				attron( COLOR_PAIR( HIGHLIGHT_PAIR ) );
				addstr( "Metadata:" );
				attroff( COLOR_PAIR( HIGHLIGHT_PAIR ) );
				attron( COLOR_PAIR( DEFAULT_PAIR ) );

				struct tm last_edit_tm;
				localtime_r( &note->last_edit, &last_edit_tm );
				char last_edit[ 64 ];

				struct tm created_tm;
				localtime_r( &note->created, &created_tm );
				char created[ 64 ];

				/* if full metadata string fits */
				if ( x - hx > strlen( "Metadata: created: DD.MM.YY HH:mm:SS, modified: DD.MM.YY HH:mm:SS" ) ) {
					/* full metadata */
					strftime( last_edit, arraysize( last_edit ) - 1, "%d.%m.%Y %H:%M:%S", &last_edit_tm );
					strftime( created,   arraysize( created   ) - 1, "%d.%m.%Y %H:%M:%S", &created_tm   );

					printw( " created: %s, modified: %s", created, last_edit );
				} else {
					/* abreviates "created" and "modified", removes time, keeps date */
					strftime( last_edit, arraysize( last_edit ) - 1, "%d.%m.%Y", &last_edit_tm );
					strftime( created,   arraysize( created   ) - 1, "%d.%m.%Y", &created_tm   );

					char buffer[ 1024 ];
					sprintf( buffer, " cre: %s, mod: %s", created, last_edit );

					addnstr( buffer, x - hx - strlen( "Metadata: " ) );
				}

				if ( note->tag_count != 0 ) {
					move( row_y++, 1 + hx );
					addstr( "  tags: " );

					for ( size_t i = 0; i < note->tag_count; i++ ) {
						addstr( note->tags[ i ] );
						if ( i != note->tag_count - 1 ) addstr( ", " );
					}

				}

				/* print note contents */
				move( row_y++, 1 + hx );
				attron( COLOR_PAIR( HIGHLIGHT_PAIR ) );
				addstr( "Contents:" );
				attroff( COLOR_PAIR( HIGHLIGHT_PAIR ) );
				attron( COLOR_PAIR( DEFAULT_PAIR ) );

				/* TODO: find a way to do line wrapping */

				size_t p_y = row_y;
				size_t p_x = 0;

				for ( size_t i = 0; i < rendered_note.count; i++ ) {
					switch ( rendered_note.segments[ i ].type ) {
						case st_text:
							for ( size_t j = 0; rendered_note.segments[ i ].text[ j ] != 0; j++ ) {
								switch ( rendered_note.segments[ i ].text[ j ] ) {
									case '\n':
										p_x = 0;
										p_y++;
										break;

									case '\t':
										p_x += 4;
										break;

									default:
										mvprintw( p_y, p_x + hx + 1, "%c", rendered_note.segments[ i ].text[ j ] );
										p_x++;
								}

								/* reached right side of the "window" */
								if ( p_x + hx + 1 >= x ) {
									p_x = 0;
									p_y++;
								}
							}
							break;

						case st_todo: {
							const struct segment_todo* todo = &rendered_note.segments[ i ].todo;

							char buffer[ 64 + strlen( todo->priority ) + strlen( todo->goal ) + ( todo->status != NULL ? strlen( todo->status ) : 0 ) + ( todo->date != NULL ? strlen( todo->date ) : 0 ) ];
							size_t buffer_index = 0;

							/* priority */
							buffer_index += sprintf( buffer + buffer_index, "[%s]", todo->priority );

							/* status */
							if ( todo->status != NULL ) {
								buffer_index += sprintf( buffer + buffer_index, "[%s]", todo->status );
							}
							
							/* date/deadline */
							if ( todo->date != NULL ) {
								buffer_index += sprintf( buffer + buffer_index, "[%s]", todo->date );
							}

							/* goal */
							buffer_index += sprintf( buffer + buffer_index, " " );
							buffer_index += sprintf( buffer + buffer_index, todo->goal );

							if ( config.colour_segments ) attron( COLOR_PAIR( HIGHLIGHT_SEGMENT_PAIR ) );
							for ( size_t i = 0; i < buffer_index; i++ ) {
								switch ( buffer[ i ] ) {
									case '\n':
										p_x = 0;
										p_y++;
										break;

									case '\t':
										p_x += 4;
										break;

									default:
										mvprintw( p_y, p_x + hx + 1, "%c", buffer[ i ] );
										p_x++;
								}

								/* reached right side of the "window" */
								if ( p_x + hx + 1 >= x ) {
									p_x = 0;
									p_y++;
								}
							}
							if ( config.colour_segments ) attroff( COLOR_PAIR( HIGHLIGHT_SEGMENT_PAIR ) );
							}
							break;
					}
				}
			}
		}

		/* footer */
		attron( COLOR_PAIR( FOOTER_PAIR ) );

		const char* footer_text       = " n - new note | r - search | R - rescan notes | enter - open selected note | q - quit | tab - change view | S - settings ";
		const char* small_footer_text = " n - new note | r - search | q - quit ";

		/* footer fits on screen */
		if ( x >= strlen( footer_text ) ) {
			/* show full footer */
			mvprintw( y - 1, 0, footer_text );

		} else {
			/* show small footer */
			mvprintw( y - 1, 0, small_footer_text );
		}

		for ( size_t i = getcurx( stdscr ); i < x; i++ ) addch( ' ' );

		attroff( COLOR_PAIR( FOOTER_PAIR ) );
		attron( COLOR_PAIR( DEFAULT_PAIR ) );

		/* print buffer to screen */
		refresh();

		/* user input */
		const int ch = get_input();
		switch ( ch ) {
			case 'n':
				popup_open_note( NULL );
				break;

			case 'r': {

				const char* selected_note_path = popup_search();
				if ( selected_note_path == NULL ) break;

				popup_open_note( selected_note_path );

				}			
				break;

			case 'R':
				popup_notification_nonblocking( "Scanning notes" );
				scan_notes();
				sort_notes( config.sort_function );

				preview_force_refresh = true;

				break;

				/*
			case 's':
				popup_sort();
				break;
				*/

			case 'S':
				popup_settings();
				break;

			case 'q':
				exit( 0 );
				break;

			case /* TAB */KEY_STAB: case KEY_BTAB: case KEY_CTAB: case KEY_CATAB: case '\t':
				if ( screen_selector == 1 ) screen_selector = 0;
				else screen_selector = 1;
				selector = 0;
				break;
		}

		if ( screen_selector == 0 )
			switch ( ch ) {
				case KEY_ENTER: case '\n':
					popup_open_note( notes_list.note_paths[ note_selector ] );

					/* force program to refresh note */
					if ( note != NULL ) free_note( note );
					note = NULL;

					break;
				case KEY_DOWN:
					if ( note_selector != ( notes_list.note_count - 1 ) ) note_selector++;
					break;
				case KEY_UP:
					if ( note_selector != 0 ) note_selector--;
					break;
			}
		else
			switch ( ch ) {
				case '\n': /* open selected note, if possible, for display */
					if ( notes_list.note_count != 0 )
						note_display( notes_list.note_paths[ selected_note ] );
					break;
				case KEY_DOWN:
					/* scroll view */
					break;
				case KEY_UP:
					/* scroll view */
					break;
			}

		continue;
	}

	return;
}

void
scan_notes( void ) {
	add_log( "scanning notes" );

	const char* ntp_dir = get_notes_dir();

	DIR* dir = opendir( ntp_dir );
	
	if ( dir == NULL ) { /* directory does not exist */
		
		rec_mkdir( ntp_dir, MKDIR_DEFAULT_MODE );

		dir = opendir( ntp_dir );

		if ( dir == NULL ) {
			if ( stdscr ) {
				char text[ strlen( "Failed to open/create directory: %s\n" ) + strlen( ntp_dir ) + 1 ];
				strcpy( text, "Failed to open/create directory: " );
				strcat( text, ntp_dir );
				popup_notification( text );
			} else {
				if ( stdscr ) endwin();
				printf( "Failed to open/create directory: %s\n", ntp_dir );
				exit( 1 );
			}

		}
	}

	/* innitialize notes_list */
	if ( notes_list.note_paths != NULL ) {
		free( notes_list.note_paths );
	}
	notes_list.note_paths = NULL;
	notes_list.note_count = 0;

	struct dirent* dirent;
	while ( ( dirent = readdir( dir ) ) != NULL ) {
		if ( strcmp( dirent->d_name, ".." ) == 0 || strcmp( dirent->d_name, "." ) == 0 ) continue;

		/* if file ends with .note */
		if ( strlen( dirent->d_name ) > strlen( ".note" ) && dirent->d_name[ strlen( dirent->d_name ) - 5 ] == '.' && dirent->d_name[ strlen( dirent->d_name ) - 4 ] == 'n' && dirent->d_name[ strlen( dirent->d_name ) - 3 ] == 'o' && dirent->d_name[ strlen( dirent->d_name ) - 2 ] == 't' && dirent->d_name[ strlen( dirent->d_name ) - 1 ] == 'e' ) {

			char* note_path;

			note_path = malloc( strlen( dirent->d_name ) - strlen( ".note" ) + 1 );
			memcpy( note_path, dirent->d_name, strlen( dirent->d_name ) - strlen( ".note" ) );
			note_path[ strlen( dirent->d_name ) - strlen( ".note" ) ] = 0;

			/* add note to notes_list */
			notes_list.note_count++;
			notes_list.note_paths = realloc( notes_list.note_paths, notes_list.note_count * sizeof( char* ) );

			if ( notes_list.note_paths == NULL ) {
				puts( "Failed to allocate memory for notes" );
				exit( 1 );
			}
			
			memcpy( notes_list.note_paths + ( notes_list.note_count - 1 ), &note_path, sizeof( char* ) );
		}

	}

	closedir( dir );

	/* sort notes with config_sort_function */
	sort_notes( config.sort_function );

	return;
}

char*
load_note_raw( const char* p_note_path ) {
	char full_note_path[ strlen( get_notes_dir() ) + /**/1 + strlen( p_note_path ) + strlen( ".note" ) + 1 ];
	sprintf( full_note_path, "%s/%s.note", get_notes_dir(), p_note_path );
	FILE* note_file = fopen( full_note_path, "r" );
	if ( note_file == NULL ) {
		return NULL;
	}

	fseek( note_file, 0, SEEK_END );
	size_t note_content_size = ftell( note_file );
	rewind( note_file );

	char* note_content = malloc( note_content_size + 1 );
	size_t fread_size = fread( note_content, sizeof( char ), note_content_size, note_file );
	note_content[ note_content_size ] = 0;

	fclose( note_file );

	return note_content;
}

struct note*
load_note( const char* p_note_path ) {

	char log_text[ strlen( "Loading note: %s" ) + strlen( p_note_path ) ];
	sprintf( log_text, "loading note: %s", p_note_path );
	add_log( log_text );

	/* get note content */
	char* note_raw = load_note_raw( p_note_path );
	if ( note_raw != NULL ) {
		struct note* note = malloc( sizeof( struct note ) );
		note->path = malloc( strlen( p_note_path ) + 1 );
		strcpy( note->path, p_note_path );

		note->size = strlen( note_raw );

		/* separate first line */
		char* first_line_end = strchr( note_raw, '\n' );
		if ( first_line_end == NULL ) {
			first_line_end = strchr( note_raw, 0 );
		}

		char* metadata_json = malloc( ( first_line_end - note_raw ) + 1 );
		strncpy( metadata_json, note_raw, first_line_end - note_raw );
		metadata_json[ ( first_line_end - note_raw ) ] = 0;

		/* save rest as note::content */
		if ( *first_line_end != 0 ) {
			note->content = malloc( strlen( first_line_end ) + 1 );
			strcpy( note->content, first_line_end + 1 );
		} else {
			note->content = NULL;
		}

		/* parse first line */
		json_error_t json_error;
		json_t* metadata_root = json_loads( metadata_json, 0, &json_error );
		free( metadata_json );

		if ( metadata_root == NULL ) {

			note->created = 0;
			note->last_edit = 0;
			note->tag_count = 0;

			add_log( "note failed to load: invalid json" );

			return note;
		}

		if ( json_is_object( metadata_root ) == false ) {
			/**/
			json_decref( metadata_root );

			note->created = 0;
			note->last_edit = 0;
			note->tag_count = 0;

			add_log( "note failed to load: invalid json" );

			return note;
		}

		/* get created */
		json_t* metadata_created = json_object_get( metadata_root, "created" );
		if ( metadata_created == NULL ) {
			note->created = 0;
		} else {
			if ( json_is_integer( metadata_created ) == false ) {
				note->created = 0;
			} else {
				note->created = json_integer_value( metadata_created );
			}
		}

		/* get last_edit */
		json_t* metadata_last_edit = json_object_get( metadata_root, "last_edit" );
		if ( metadata_last_edit == NULL ) {
			note->last_edit = 0;
		} else {
			if ( json_is_integer( metadata_last_edit ) == false ) {
				note->last_edit = 0;
			} else {
				note->last_edit = json_integer_value( metadata_last_edit );
			}
		}

		/* get tags */
		json_t* metadata_tags = json_object_get( metadata_root, "tags" );
		if ( metadata_tags == NULL ) {
			note->tags = NULL;
			note->tag_count = 0;
		} else {
			if ( json_is_array( metadata_tags ) == false ) {
				note->tags = NULL;
				note->tag_count = 0;
			} else {

				note->tag_count = json_array_size( metadata_tags );
				note->tags = malloc( note->tag_count * sizeof( char* ) );

				for ( size_t i = 0; i < note->tag_count; i++ ) {

					json_t* tag = json_array_get( metadata_tags, i );

					if ( json_is_string( tag ) == false ) {

					} else {
						note->tags[ i ] = malloc( json_string_length( tag ) + 1 );
						strcpy( note->tags[ i ], json_string_value( tag ) );
					}

				}
			}
		}

		/* free */
		json_decref( metadata_root );	
		free( note_raw );

		add_log( "note loaded successfully" );

		return note;

	}

	struct note* note = malloc( sizeof( struct note ) );
	note->path = malloc( strlen( p_note_path ) + 1 );
	strcpy( note->path, p_note_path );
	note->created = time( NULL );
	note->last_edit = note->created;
	note->content = NULL;
	note->tag_count = 0;

	save_note( note );

	scan_notes();

	add_log( "created a new note" );

	return note;
}

void
free_note( struct note* p_note ) {
	if ( p_note->content ) free( p_note->content );
	if ( p_note->tag_count ) free( p_note->tags );


	free( p_note );
}

bool
save_note( struct note* p_note ) {

	/* open original note file */
	char full_note_path[ strlen( get_notes_dir() ) + /**/1 + strlen( p_note->path ) + strlen( ".note" ) + 1 ];
	sprintf( full_note_path, "%s/%s.note", get_notes_dir(), p_note->path );
	FILE* note_file = fopen( full_note_path, "w" );

	if ( note_file == NULL ) return false;

	/* write metadata */
	/* generate json */
	json_t* metadata_tags_json = json_array();
	for ( size_t i = 0; i < p_note->tag_count; i++ ) {
		json_t* tag_json = json_string( p_note->tags[ i ] );

		json_array_append_new( metadata_tags_json, tag_json );
	}

	json_t* metadata_json = json_pack(
		"{ si si so }",
		/* created  last_edit  tags */
		"created",   p_note->created,
		"last_edit", p_note->last_edit,
		"tags",      metadata_tags_json
	);

	char* metadata_json_text = json_dumps( metadata_json, 0 );

	json_decref( metadata_json );

	fputs( metadata_json_text, note_file );
	fputc( '\n', note_file );

	/* write new content */
	if ( p_note->content != NULL )
		fputs( p_note->content, note_file );

	/* close file */
	fclose( note_file );

	return true;
}
