#include "popups.h"

#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "utils.h"
#include "note_editor.h"
#include "notetaking.h"
#include "note_renderer.h"
#include "config.h"
#include "note_compare.h"

void
popup_open_note( const char* p_note_path ) {

	int selector = 0;
	
	bool loop = true;

	if ( p_note_path == NULL ) {

		size_t index = 0;
		char note_name[ 64 ] = { 0 };

		while ( true ) {
			clear();
			fill_screen();

			printw( "note name: %s", note_name );

			refresh();

			int ch = get_input();
			switch ( ch ) {
				case KEY_ESCAPE:
					return;

				case KEY_BACKSPACE:
					if ( index != 0 ) index--;
					note_name[ index ] = 0;
					break;

				case KEY_ENTER:
				case '\n':
					if ( strlen( note_name ) == 0 ) popup_notification( "enter a name" );
					else {

						int note_already_exists = 0;
						for ( size_t i = 0; i < notes_list.note_count; i++ ) {
							if ( strcmp( note_name, notes_list.note_paths[ i ] ) == 0 ) {
								note_already_exists = 1;
								break;
							}
						}

						if ( note_already_exists ) {
							popup_notification( "a note with this name already exists" );
						} else {
							char* note_path = malloc( strlen( note_name ) + 1 );
							memcpy( note_path, note_name, strlen( note_name ) );
							note_path[ strlen( note_name ) ] = 0;

							notes_list.note_count++;
							notes_list.note_paths = realloc( notes_list.note_paths, notes_list.note_count * sizeof( char* ) );

							if ( notes_list.note_paths == NULL ) {
								if ( stdscr ) endwin();
								puts( "Failed to allocate memory for new note" );
								exit( 1 );
							}

							notes_list.note_paths[ notes_list.note_count - 1 ] = note_path;

							char full_note_path[ strlen( get_notes_dir() ) + /**/1 + strlen( note_path ) + strlen( ".note" ) + 1 ];
							sprintf( full_note_path, "%s/%s.note", get_notes_dir(), note_path );

							/* create a new note file */
							struct note note;
							note.created = time( NULL );
							note.last_edit = note.created;
							note.tag_count = 0;
							note.path = notes_list.note_paths[ notes_list.note_count - 1 ];
							note.content = NULL;
							save_note( &note );

							/* sort notes */
							sort_notes( config.sort_function );

							popup_open_note( note_path );
							return;

						}
					}
					break;

				default:
					if ( ch == '/' ) ;

					else if ( index != arraysize( note_name ) - 1 ) {
						note_name[ index ] = ch;
						index++;
					}
			}

		}
	}

	while ( loop ) {
		/* clear screen buffer */
		clear();
		fill_screen();

		int screen_w = getmaxx( stdscr );
		int screen_h = getmaxy( stdscr );

		enum option_type {
			ot_edit,
			ot_display,
			ot_edit_tags,
			ot_rename,
			ot_delete,
			ot_back
		};

		struct option {
			const char* name;
			enum option_type type;
		};

		const struct option options[] = {
			{ "Edit", ot_edit },
			{ "Display", ot_display },
			{ "Edit tags", ot_edit_tags },
			{ "Rename", ot_rename },
			{ "Delete", ot_delete },
			{ "Back", ot_back },
		};

		char top_text[ 1024 ];
		snprintf( top_text, arraysize( top_text ), "Note: %s", p_note_path );
		mvprintw( ( screen_h - arraysize( options ) ) / 2, ( screen_w - strlen( top_text ) ) / 2, top_text );

		for ( int i = 0; i < arraysize( options ); i++ ) {
			move( ( screen_h - arraysize( options ) ) / 2 + 2 + i, ( screen_w - strlen( options[ i ].name ) ) / 2 );

			if ( selector == i ) attron( COLOR_PAIR( SELECTOR_PAIR ) );
			printw( options[ i ].name );
			if ( selector == i ) attroff( COLOR_PAIR( SELECTOR_PAIR ) );

			continue;
		}

		/* display screen */
		refresh();

		bool loop2 = true;
		while ( loop2 ) {
			loop2 = false;

			int ch = get_input();
			switch ( ch ) {
				case 'q':
					loop = false;
					break;
				case KEY_ENTER: case '\n': case '\r':
					switch ( options[ selector ].type ) {
						case ot_edit:
							note_editor( p_note_path );
							break;
						case ot_display:
							note_display( p_note_path );
							break;
						case ot_delete:
							{
								char full_note_path[ strlen( get_notes_dir() ) + /**/1 + strlen( p_note_path ) + strlen( ".note" ) + 1 ];
								sprintf( full_note_path, "%s/%s.note", get_notes_dir(), p_note_path );

								remove( full_note_path );

								scan_notes();
							}
							
							loop = false;
							break;
						case ot_back:
							loop = false;
							break;
						case ot_rename:
							p_note_path = popup_rename_note( p_note_path );
							if ( p_note_path == NULL ) return;
							break;
						case ot_edit_tags:
							popup_edit_tags( p_note_path );
							break;
					}
					break;
				case KEY_DOWN:
					if ( selector != ( arraysize( options ) - 1 ) ) selector++;
					break;
				case KEY_UP:
					if ( selector != 0 ) selector--;
					break;
				default:
					loop2 = true;
			}
		}
	}

	return;
}

const char*
popup_search( void ) {

	/* nothing to choose from */
	if ( notes_list.note_count == 0 ) {
		popup_notification( "Unable to search: no notes found" );
		return NULL;
	}

	char searched_term[ 1024 ];
	searched_term[ 0 ] = 0;
	size_t st_cursor = 0;
	bool searched_term_change = true;

	struct note_match {
		const char* path;

		bool matches_any;
		bool matches_name;
		bool matches_content;
		bool matches_tag;
		bool matches_created;
		bool matches_last_edit;
	};

	struct note_match matching_notes[ notes_list.note_count ];
	uint  matching_notes_size = 0;
	uint  matching_cursor = 0;

	while ( 1 ) {

		if ( searched_term_change ) {
			searched_term_change = false;
			/* TODO: search all notes and find matching terms in contents, paths, names, dates, ... */

			clear();
			fill_screen();
			addstr( "searching through files" );
			refresh();

			/* reset matching_notes */
			matching_notes_size = 0;

			/* go through all notes */
			for ( size_t i = 0; i < notes_list.note_count; i++ ) {

				matching_notes[ matching_notes_size ].path = notes_list.note_paths[ i ];
				matching_notes[ matching_notes_size ].matches_any     = false;
				matching_notes[ matching_notes_size ].matches_name    = false;
				matching_notes[ matching_notes_size ].matches_content = false;
				matching_notes[ matching_notes_size ].matches_tag     = false;
				matching_notes[ matching_notes_size ].matches_created   = false;
				matching_notes[ matching_notes_size ].matches_last_edit = false;

				/* if searching for nothing -> show everything */
				if ( searched_term[ 0 ] == 0 ) {
					matching_notes[ matching_notes_size ].matches_any = true;
					matching_notes_size++;
					continue;
				}

				bool matches = false;

				/* if name contains searched term */
				if ( strstr( notes_list.note_paths[ i ], searched_term ) != NULL ) {
					matching_notes[ matching_notes_size ].matches_name = true;

					matches = true;
				}

				struct note* note = load_note( notes_list.note_paths[ i ] );

				/* check if meta data match */
				for ( size_t i = 0; i < note->tag_count; i++ ) {
					if ( strstr( note->tags[ i ], searched_term ) != NULL ) {
						matching_notes[ matching_notes_size ].matches_tag = true;

						matches = true;
					}
				}

				/* check created */
				struct tm created_tm;
				localtime_r( &note->created, &created_tm );
				char created[ 64 ];
				strftime( created, arraysize( created ) - 1, "%d.%m.%Y %H:%M:%S", &created_tm );
				if ( strstr( created, searched_term ) != NULL ) {
					matching_notes[ matching_notes_size ].matches_created = true;
					matches = true;
				}

				/* check last_edit */
				struct tm last_edit_tm;
				localtime_r( &note->last_edit, &last_edit_tm );
				char last_edit[ 64 ];
				strftime( last_edit, arraysize( last_edit ) - 1, "%d.%m.%Y %H:%M:%S", &last_edit_tm );
				if ( strstr( last_edit, searched_term ) != NULL ) {
					matching_notes[ matching_notes_size ].matches_last_edit = true;
					matches = true;
				}

				/* check file contents */
				if ( strstr( note->content, searched_term ) != NULL ) {
					matching_notes[ matching_notes_size ].matches_content = true;

					matches = true;
				}

				if ( matches ) matching_notes_size++;

				free_note( note );

				continue;
			}

			/* check cursor */
			while ( matching_notes[ matching_cursor ].path == NULL ) matching_cursor--;

		}

		int x = getmaxx( stdscr );
		int y = getmaxy( stdscr );

		clear();
		fill_screen();
		printw( "Searching for: %s", searched_term );
		attron( A_REVERSE ); addstr( " " ); attroff( A_REVERSE );

		int offset = 0;
		if ( matching_cursor + 1 > y - 3 ) offset = y - 4 - matching_cursor;

		/*
		move( y - 2, x - 50 );
		printw( "offset: %i; selector: %i; first index: %i", offset, matching_cursor, 0 - offset );
		*/

		for ( size_t i = 0; i < min( matching_notes_size, y - 2 ); i++ ) {

			if ( i - offset == matching_notes_size ) break;

			move( 2 + i, 5 );

			if ( ( i - offset ) == matching_cursor ) attron( A_REVERSE );
			else attron( COLOR_PAIR( HIGHLIGHT_PAIR ) );
			addstr( matching_notes[ ( i - offset ) ].path );
			if ( ( i - offset ) == matching_cursor ) attroff( A_REVERSE );
			else attroff( COLOR_PAIR( HIGHLIGHT_PAIR ) );

			if ( matching_notes[ ( i - offset ) ].matches_name || matching_notes[ ( i - offset ) ].matches_created || matching_notes[ ( i - offset ) ].matches_last_edit || matching_notes[ ( i - offset ) ].matches_content || matching_notes[ ( i - offset ) ].matches_tag ) {
				addstr( " matches: " );
				if ( matching_notes[ ( i - offset ) ].matches_name      ) addstr( "name " );
				if ( matching_notes[ ( i - offset ) ].matches_content   ) addstr( "content " );
				if ( matching_notes[ ( i - offset ) ].matches_tag       ) addstr( "tag " );
				if ( matching_notes[ ( i - offset ) ].matches_created   ) addstr( "created " );
				if ( matching_notes[ ( i - offset ) ].matches_last_edit ) addstr( "last_edit " );
			}

			continue;
		}
		refresh();

		int ch = get_input();
		switch ( ch ) {
			case KEY_ENTER:
			case '\n':
				/* TODO: implement opening a note from selection */
				if ( matching_notes_size == 0 ) break;
				return matching_notes[ matching_cursor ].path;
			case KEY_ESCAPE /* KEY_ESCAPE */:
				return NULL; /* NULL */
			case KEY_BACKSPACE:
				searched_term_change = 1;
				if ( st_cursor != 0 ) st_cursor--;
				searched_term[ st_cursor ] = 0;
				
				break;
			case KEY_UP:
				if ( matching_cursor != 0 ) matching_cursor--;
				break;
			case KEY_DOWN:
				if ( matching_cursor < matching_notes_size - 1 ) matching_cursor++;
				break;
			case KEY_RIGHT:
			case KEY_LEFT:
				break;
			default: /* TODO: filter printable characters */
				if ( st_cursor == arraysize( searched_term ) - 1 ) break;
				searched_term_change = true;
				searched_term[ st_cursor ] = ( char )ch;
				st_cursor++;
				searched_term[ st_cursor ] = 0;
		}

	}

	return NULL;
}

void
popup_sort() {

	popup_notification( "Sorting NOT yet IMPLEMENTED" );
	
	return;
}

void
popup_notification( const char* p_message ) {

	const char* notification_title = "Notification";

	while ( 1 ) {

		int w = getmaxx( stdscr ), h = getmaxy( stdscr );

		clear();
		fill_screen();

		move( ( h - 3 ) / 2, ( w - strlen( notification_title ) ) / 2 );
		addstr( notification_title );
		move( ( h - 3 ) / 2 + 2, ( w - strlen( p_message ) ) / 2 );
		addstr( p_message );

		refresh();

		int ch = get_input();
		if ( ch == KEY_ENTER || ch == '\n' || ch == '\r' ) break;

		continue;
	}

	return;
}

void
popup_notification_nonblocking( const char* p_message ) {
	const char* notification_title = "Notification";

	int w = getmaxx( stdscr ), h = getmaxy( stdscr );

	clear();

	move( ( h - 3 ) / 2, ( w - strlen( notification_title ) ) / 2 );
	addstr( notification_title );
	move( ( h - 3 ) / 2 + 2, ( w - strlen( p_message ) ) / 2 );
	addstr( p_message );

	refresh();

	return;
}

void
note_display( const char* p_note_path ) {

	/* setup for using the function to preview notes in main menu */
	size_t w_w, w_h;
	size_t offset_x, offset_y;

	int cursor_x, cursor_y;

	/* get file contents */
	struct note* note = load_note( p_note_path );
	if ( note->content == NULL ) {
		popup_notification( "Failed to open selected note." );
		return;
	}

	/* render note */
	struct rendered_note rendered_note = { .count = 0 };
	if ( config.render_notes ) {
		free_rendered_note( rendered_note );
		rendered_note = note_render_text( note->content );
	} else {
		rendered_note.count = 1;
		rendered_note.segments = malloc( sizeof( struct note_segment ) );
		rendered_note.segments[ 0 ].type = st_text;
		rendered_note.segments[ 0 ].text = note->content;
	}

	/* TODO: add the ability to launch an external program to view the note */
	bool use_builtin_displayer = false;

	switch ( config.display ) {
		case display_less: {
			if ( stdscr ) endwin();

			FILE* less_ = popen( "less -R", "w" );

			fputs( "\e[47;30mNote name:\e[0m ", less_ );
			fputs( note->path, less_ );
			fputs( "\n", less_ );

			fputs( "\e[47;30mMetadata:\e[0m\n", less_ );

			struct tm last_edit_tm;
			localtime_r( &note->last_edit, &last_edit_tm );
			char last_edit[ 64 ];
			strftime( last_edit, arraysize( last_edit ) - 1, "%d.%m.%Y %H:%M:%S", &last_edit_tm );

			struct tm created_tm;
			localtime_r( &note->created, &created_tm );
			char created[ 64 ];
			strftime( created, arraysize( created ) - 1, "%d.%m.%Y %H:%M:%S", &created_tm );

			fprintf( less_, " created: %s, modified: %s\n", created, last_edit );
			if ( note->tag_count != 0 ) {
				fputs( "\n  tags: ", less_ );

				for ( size_t i = 0; i < note->tag_count; i++ ) {
					fputs( note->tags[ i ], less_ );
					if ( i != note->tag_count - 1 ) fputs( ", ", less_ );
				}

				fputs( "\n", less_ );
			}

			fputs( "\e[47;30mContents:\e[0m\n", less_ );
			for ( size_t i = 0; i < rendered_note.count; i++ ) {
				switch ( rendered_note.segments[ i ].type ) {
					case st_text:
						fputs( rendered_note.segments[ i ].text, less_ );
						break;

					case st_todo: {
						struct segment_todo* todo = &rendered_note.segments[ i ].todo;
					
						/* priority */
						fputs( "[", less_ );
						fputs( todo->priority, less_ );
						fputs( "]", less_ );

						/* status */
						if ( todo->status != NULL ) {
							fputs( "[", less_ );
							fputs( todo->status, less_ );
							fputs( "]", less_ );
						}
						
						/* date/deadline */
						if ( todo->date != NULL ) {
							fputs( "[", less_ );
							fputs( todo->date, less_ );
							fputs( "]", less_ );
						}

						fputs( " ", less_ );

						/* goal */
						fputs( todo->goal, less_ );
						}
						break;
				}
			}

			pclose( less_ );

			ncurses_init();
			}
			break;

		default:
			use_builtin_displayer = true;
	}

	if ( use_builtin_displayer ) while ( true ) {

		clear();
		fill_screen();

		w_w = getmaxx( stdscr ); w_h = getmaxy( stdscr );

		size_t row_y = 0;

		/* print note name */
		move( row_y++, 0 );
		attron( COLOR_PAIR( HIGHLIGHT_PAIR ) );
		addstr( "Note name:" );
		attroff( COLOR_PAIR( HIGHLIGHT_PAIR ) );
		attron( COLOR_PAIR( DEFAULT_PAIR ) );
		addch( ' ' );
		addstr( note->path );

		/* print metadata */
		move( row_y++, 0 );
		attron( COLOR_PAIR( HIGHLIGHT_PAIR ) );
		addstr( "Metadata:" );
		attroff( COLOR_PAIR( HIGHLIGHT_PAIR ) );
		attron( COLOR_PAIR( DEFAULT_PAIR ) );

		struct tm last_edit_tm;
		localtime_r( &note->last_edit, &last_edit_tm );
		char last_edit[ 64 ];
		strftime( last_edit, arraysize( last_edit ) - 1, "%d.%m.%Y %H:%M:%S", &last_edit_tm );

		struct tm created_tm;
		localtime_r( &note->last_edit, &created_tm );
		char created[ 64 ];
		strftime( created, arraysize( created ) - 1, "%d.%m.%Y %H:%M:%S", &created_tm );

		printw( " created: %s, modified: %s", created, last_edit );
		if ( note->tag_count != 0 ) {
			move( row_y++, 0 );
			addstr( "  tags: " );

			for ( size_t i = 0; i < note->tag_count; i++ ) {
				addstr( note->tags[ i ] );
				if ( i != note->tag_count - 1 ) addstr( ", " );
			}

		}

		/* print note contents */
		move( row_y++, 0 );
		attron( COLOR_PAIR( HIGHLIGHT_PAIR ) );
		addstr( "Contents:" );
		attroff( COLOR_PAIR( HIGHLIGHT_PAIR ) );
		attron( COLOR_PAIR( DEFAULT_PAIR ) );

		int x = 0, y = row_y;
		for ( size_t i = 0; i < rendered_note.count; i++ ) {
			switch ( rendered_note.segments[ i ].type ) {
				case st_text:
					for ( size_t j = 0; rendered_note.segments[ i ].text[ j ] != 0; j++ ) {
						switch ( rendered_note.segments[ i ].text[ j ] ) {
							case '\n':
								x = 0;
								y++;
								break;

							case '\t':
								x += 4;
								break;

							default:
								mvprintw( y, x, "%c", rendered_note.segments[ i ].text[ j ] );
								x++;
						}

						if ( w_w + 1 <= x ) {
							x = 0;
							y++;
						}
					}
					break;

				case st_todo: {
					struct segment_todo* todo = &rendered_note.segments[ i ].todo;

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
								x = 0;
								y++;
								break;

							case '\t':
								x += 4;
								break;

							default:
								mvprintw( y, x, "%c", buffer[ i ] );
								x++;
						}

						/* reached right side of the "window" */
						if ( w_w + 1 <= x ) {
							x = 0;
							y++;
						}
					}
					if ( config.colour_segments ) attroff( COLOR_PAIR( HIGHLIGHT_SEGMENT_PAIR ) );
					}
					break;
			}
		}

		refresh();

		int ch = get_input();
		if ( ch == KEY_ENTER || ch == '\n' || ch == KEY_ESCAPE || ch == 'q' ) break;

		continue;
	}

	free_note( note );

	if ( config.render_notes )
		free_rendered_note( rendered_note );

	return;
}

void
popup_settings( void ) {

	enum mi_type {
		mi_text_editor,
		mi_text_viewer,
		mi_render_text,
		mi_show_main_screen_footer,
		mi_sorting,
		mi_light_mode,
		mi_colour_background,

		mi_reset,
		mi_save_and_back,
		mi_back,

		mi_separator
	};

	struct menu_item {
		const char* name;
		enum mi_type type;
	};

	struct menu_item settings_menu[] = {
		{ "Text Editor: %s", mi_text_editor },
		{ "Text Viewer: %s", mi_text_viewer },
		{ "Sort Notes: %s", mi_sorting },
		{ "Note Text Rendering: %s", mi_render_text },
		{ "Light Mode: %s", mi_light_mode },
		{ "Colour Background: %s", mi_colour_background },
		{ NULL, mi_separator },
		{ "Reset ALL Settings", mi_reset },
		{ "Save & Back", mi_save_and_back },
		{ "Exit Without Saving", mi_back }
	};

	uint8_t index = 0;

	/* config settings */
	struct config _config = config;

	bool run = true;
	while ( run ) {

		int win_w = getmaxx( stdscr ), win_h = getmaxy( stdscr );

		clear();
		fill_screen();

		attron( COLOR_PAIR( DEFAULT_PAIR ) );

		move( ( win_h - ( arraysize( settings_menu ) + 2 ) ) / 2, ( win_w - strlen( "Settings" ) ) / 2 );
		addstr( "Settings" );

		for ( size_t i = 0; i < arraysize( settings_menu ); i++ ) {

			char temp[ 64 ];
			*temp = 0;

			if ( i == index ) {
				if ( settings_menu[ i ].type == mi_text_editor || settings_menu[ i ].type == mi_text_viewer || settings_menu[ i ].type == mi_render_text )
					strcat( temp, "<" );
			}


			char temp2[ 64 ];
			switch ( settings_menu[ i ].type ) {
				case mi_text_editor:
					sprintf( temp2, settings_menu[ i ].name, text_editor_to_string( _config.text_editor ) );
					break;
				case mi_text_viewer:
					sprintf( temp2, settings_menu[ i ].name, display_to_string( _config.display ) );
					break;
				case mi_sorting:
					sprintf( temp2, settings_menu[ i ].name, sort_function_to_string( _config.sort_function ) );
					break;
				case mi_render_text:
					sprintf( temp2, settings_menu[ i ].name, _config.render_notes ? "on" : "off" );
					break;

				case mi_light_mode:
					sprintf( temp2, settings_menu[ i ].name, _config.light_mode ? "on" : "off" );
					break;

				case mi_colour_background:
					sprintf( temp2, settings_menu[ i ].name, _config.colour_background ? "on" : "off" );
					break;

				case mi_separator: /* skip */
					temp2[ 0 ] = 0;
					break;

				case mi_save_and_back:
				case mi_back:
				default:
					strcpy( temp2, settings_menu[ i ].name );
					break;
			}
			strcat( temp, temp2 );

			if ( i == index ) {
				if ( settings_menu[ i ].type == mi_text_editor || settings_menu[ i ].type == mi_text_viewer || settings_menu[ i ].type == mi_render_text )
					strcat( temp, ">" );
			}

			if ( i == index ) attron( COLOR_PAIR( SELECTOR_PAIR ) );
			move( ( win_h - ( arraysize( settings_menu ) + 2 ) ) / 2 + 2 + i, ( win_w - strlen( temp ) ) / 2 );
			addstr( temp );
			if ( i == index ) {
				attroff( COLOR_PAIR( SELECTOR_PAIR ) );
				attron( COLOR_PAIR( DEFAULT_PAIR ) );
			}

		}

		refresh();

		int ch = get_input();

		if ( ch == 'q' ) break;

		if ( ch == KEY_UP ) {
			if ( index != 0 ) index--;

			while ( settings_menu[ index ].type == mi_separator ) index--;

		} else if ( ch == KEY_DOWN ) {
			if ( index != arraysize( settings_menu ) - 1 ) index++;

			while ( settings_menu[ index ].type == mi_separator ) index++;

		} else if ( ch == KEY_RIGHT ) {
			switch ( settings_menu[ index ].type ) {
				case mi_render_text:
					_config.render_notes = !_config.render_notes;
					break;

				case mi_light_mode:
					_config.light_mode = !_config.light_mode;
					break;

				case mi_colour_background:
					_config.colour_background = !_config.colour_background;
					break;

				case mi_text_editor:
					_config.text_editor++;
					if ( _config.text_editor == text_editor__END ) _config.text_editor = 0;
					break;

				case mi_text_viewer:
					_config.display++;
					if ( _config.display == display__END ) _config.display = 0;
					break;

				case mi_sorting:
					_config.sort_function++;
					if ( _config.sort_function == sort_function__END ) _config.sort_function = 0;
					break;
			}
		} else if ( ch == KEY_LEFT ) {
			switch ( settings_menu[ index ].type ) {
				case mi_render_text:
					_config.render_notes = !_config.render_notes;
					break;

				case mi_light_mode:
					_config.light_mode = !_config.light_mode;
					break;

				case mi_colour_background:
					_config.colour_background = !_config.colour_background;
					break;

				case mi_text_editor:

					if ( _config.text_editor == text_editor_builtin ) _config.text_editor = text_editor__END;
					_config.text_editor--;

					break;
				case mi_text_viewer:
					
					if ( _config.display == display_builtin ) _config.display = display__END;
					_config.display--;

					break;

				case mi_sorting:

					if ( _config.sort_function == sort_alphabet_ascending ) _config.sort_function = sort_function__END;
					_config.sort_function--;

					break;
			}
		} else if ( ch == 'R' /* Reset */ ) {
			switch ( settings_menu[ index ].type ) {
				case mi_render_text:
					_config.render_notes = default_config.render_notes;
					break;

				case mi_light_mode:
					_config.light_mode = default_config.light_mode;
					break;

				case mi_colour_background:
					_config.colour_background = default_config.colour_background;
					break;

				case mi_text_editor:
					_config.text_editor = default_config.text_editor;
					break;

				case mi_text_viewer:
					_config.display = default_config.display;
					break;

				case mi_sorting:
					_config.sort_function = default_config.sort_function;
					break;
			}

		} else if ( ch == KEY_ENTER || ch == '\n' ) {
			switch ( settings_menu[ index ].type ) {
				case mi_save_and_back:
					run = false;
					break;
				case mi_back:
					return; /* don't save */
				case mi_reset:
					_config = default_config;
					break;
			}
		}

	}

	/* rewrite config variables */
	config.display      = _config.display;
	config.text_editor  = _config.text_editor;
	config.render_notes = _config.render_notes;

	if ( config.light_mode != _config.light_mode || config.colour_background != _config.colour_background ) {
		config.light_mode        = _config.light_mode;
		config.colour_background = _config.colour_background;
		endwin();
		ncurses_init();
	}

	if ( config.sort_function != _config.sort_function ) {
		config.sort_function = _config.sort_function;

		/* sort notes_list with selected function */
		sort_notes( config.sort_function );
	}

	/* save config to file */
	config_save();

	return;
}

char*
popup_rename_note( const char* p_note_path ) {
	/* get new name */
	/* check if it's not already used */
	/* move note from old_name.note to new_name.note */
	/* return new_name */

	static char new_name[ 64 + 1 ];
	new_name[ 0 ] = 0;
	size_t nm_cursor = 0;

	bool run = true;
	while ( run ) {
		clear();
		fill_screen();

		attron( COLOR_PAIR( DEFAULT_PAIR ) );

		printw( "Old name: %s", p_note_path );
		move( 1, 0 );
		printw( "New name: %s", new_name );

		attron( COLOR_PAIR( SELECTOR_PAIR ) );
		addstr( " " );
		attroff( COLOR_PAIR( SELECTOR_PAIR ) );

		refresh();

		int ch = get_input();
		switch ( ch ) {
			case KEY_ESCAPE /* KEY_ESCAPE */:
				return NULL; /* NULL */
			case KEY_BACKSPACE:

				if ( nm_cursor != 0 ) nm_cursor--;
				new_name[ nm_cursor ] = 0;
				
				break;
			
			case KEY_ENTER:
			case '\n': {
				/* check if name is not already used */
				bool used = false;
				for ( size_t i = 0; i < notes_list.note_count; i++ ) {
					if ( strcmp( new_name, notes_list.note_paths[ i ] ) == 0 ) { 
						used = true;
						break;
					}
				}

				if ( used ) {
					popup_notification( "Name is already in use." );
				} else {
					/* move file to new name */
					char old_path[ strlen( get_notes_dir() ) + /**/1 + strlen( p_note_path ) + strlen( ".note" ) + 1 ];
					sprintf( old_path, "%s/%s.note", get_notes_dir(), p_note_path );

					char new_path[ strlen( get_notes_dir() ) + /**/1 + strlen( new_name ) + strlen( ".note" ) + 1 ];
					sprintf( new_path, "%s/%s.note", get_notes_dir(), new_name );

					if ( rename( old_path, new_path ) != 0 ) {
						popup_notification( "Failed to rename note." );
						break;
					}

					scan_notes();
					sort_notes( config.sort_function );

					run = false;
				}

				}
				break;

			default: /* TODO: filter printable characters */
				if ( nm_cursor == arraysize( new_name ) - 1 ) break;

				new_name[ nm_cursor ] = ( char )ch;
				nm_cursor++;
				new_name[ nm_cursor ] = 0;
		}
	}

	return new_name;
}

char*
popup_textbox( const char* p_default_text ) {

	static char textbox[ 128 + 1 ];
	size_t tb_cursor = 0;

	if ( p_default_text == NULL ) {
		textbox[ 0 ] = 0;
	} else {
		strncpy( textbox, p_default_text, arraysize( textbox ) - 1 );
		textbox[ arraysize( textbox ) - 1 ] = 0;

		tb_cursor = strlen( textbox );
	}


	bool run = true;
	while ( run ) {
		clear();
		fill_screen();

		attron( COLOR_PAIR( DEFAULT_PAIR ) );

		printw( "Text: %s", textbox );

		attron( COLOR_PAIR( SELECTOR_PAIR ) );
		addstr( " " );
		attroff( COLOR_PAIR( SELECTOR_PAIR ) );

		refresh();

		int ch = get_input();
		switch ( ch ) {
			case KEY_ESCAPE:
				return NULL;

			case '\n':
				run = false;
				break;

			case KEY_BACKSPACE:

				if ( tb_cursor != 0 ) tb_cursor--;
				textbox[ tb_cursor ] = 0;
				
				break;

			default: /* TODO: filter printable characters */
				if ( tb_cursor == arraysize( textbox ) - 1 ) break;

				textbox[ tb_cursor ] = ( char )ch;
				tb_cursor++;
				textbox[ tb_cursor ] = 0;
		}
	}

	return textbox;
}

void
edit_tags_menu( struct note* p_note, size_t p_index ) {

	size_t cursor = 0;

	char* menu[] = {
		"Edit",
		"Delete",
		"Back"
	};

	bool run = true;
	while ( run ) {
		clear();
		fill_screen();

		attron( COLOR_PAIR( DEFAULT_PAIR ) );

		for ( size_t i = 0; i < arraysize( menu ); i++ ) {
			if ( i == cursor ) attron( COLOR_PAIR( SELECTOR_PAIR ) );
			else attron( COLOR_PAIR( DEFAULT_PAIR ) );

			mvprintw( i, 0, menu[ i ] );

			if ( i == cursor ) attroff( COLOR_PAIR( SELECTOR_PAIR ) );
			else attroff( COLOR_PAIR( DEFAULT_PAIR ) );
		}

		refresh();

		int ch = get_input();
		switch ( ch ) {
			case 'q':
				return;

			case KEY_UP:
				if ( cursor != 0 ) cursor--;
				break;

			case KEY_DOWN:
				if ( cursor != arraysize( menu ) - 1 ) cursor++;
				break;
			
			case KEY_ENTER:
			case '\n':
				
				switch ( cursor ) {
					case 0:; /* EDIT */

						char* new_tag = popup_textbox( p_note->tags[ p_index ] );
						if ( new_tag == NULL ) break;

						free( p_note->tags[ p_index ] );
						p_note->tags[ p_index ] = malloc( strlen( new_tag ) + 1 );
						strcpy( p_note->tags[ p_index ], new_tag );

						return;
					case 1: /* DELETE */

						free( p_note->tags[ p_index ] );

						for ( size_t i = p_index; i < p_note->tag_count - 1; i++ ) {
							p_note->tags[ i ] = p_note->tags[ i + 1 ];
						}

						p_note->tag_count--;

						return;
					case 2: /* BACK */
						return;
				}
				break;
		}
	}
}

void
popup_edit_tags( const char* p_note_path ) {
	/*  */

	struct note* note = load_note( p_note_path );

	bool run = true;
	size_t cursor = 0;

	while ( run ) {
		clear();
		fill_screen();

		attron( COLOR_PAIR( DEFAULT_PAIR ) );

		mvprintw( 0, 0, "Tags:" );
		for ( size_t i = 0; i < note->tag_count; i++ ) {

			move( i + 1, 2 );

			if ( i == cursor ) attron( COLOR_PAIR( SELECTOR_PAIR ) );
			else               attron( COLOR_PAIR( DEFAULT_PAIR ) );

			addstr( note->tags[ i ] );

			if ( i == cursor ) attroff( COLOR_PAIR( SELECTOR_PAIR ) );
			else               attroff( COLOR_PAIR( DEFAULT_PAIR ) );
		}

		if ( note->tag_count == cursor ) attron( COLOR_PAIR( SELECTOR_PAIR ) );
		else               attron( COLOR_PAIR( DEFAULT_PAIR ) );
		mvprintw( 2 + note->tag_count, 0, "Add tag" );
		if ( note->tag_count == cursor ) attroff( COLOR_PAIR( SELECTOR_PAIR ) );
		else               attroff( COLOR_PAIR( DEFAULT_PAIR ) );

		refresh();

		int ch = get_input();
		switch ( ch ) {
			case KEY_ESCAPE:
				run = false;
				break;

			case KEY_DOWN:
				if ( cursor != note->tag_count ) cursor++;
				break;

			case KEY_UP:
				if ( cursor != 0 ) cursor--;
				break;

			case '\n':
				/* a tag is selected */
				if ( cursor < note->tag_count ) {

					size_t temp_tag_count = note->tag_count;

					edit_tags_menu( note, cursor );

					if ( temp_tag_count != note->tag_count )
						if ( cursor != 0 ) cursor--;


				} else {
					/* add tag */
					char* new_tag = popup_textbox( NULL );

					if ( new_tag == NULL ) break;

					note->tag_count++;
					note->tags = realloc( note->tags, note->tag_count * sizeof( char* ) );

					note->tags[ note->tag_count - 1 ] = malloc( strlen( new_tag ) + 1 );
					strcpy( note->tags[ note->tag_count - 1 ], new_tag );

				}
		}
	}

	note->last_edit = time( NULL );
	save_note( note );

	free_note( note );

	return;
}