#include "note_editor.h"

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "popups.h"
#include "utils.h"
#include "config.h"
#include "logger.h"

/* TODO: add markup syntax highlighting */

void
note_editor( const char* p_note_path ) {

	add_log( "Openning note: %s" );

	/* check if builtin editor is selected*/
	if ( config.text_editor != text_editor_builtin ) {
		note_editor_external( p_note_path, config.text_editor );
		return;
	}

	int note_saved = 1;
	int cursor = 0;

	/* open note */
	struct note* note = load_note( p_note_path );

	size_t note_content_size = strlen( note->content );

	bool run = true;
	while ( run ) {

		clear();
		fill_screen();

		uint x = 0, y = 0;

		for ( size_t i = 0; true; i++ ) {

			bool is_hidden = false;
			switch ( note->content[ i ] ) {
				case 0:
				case '\n':
				case '\t':
				case ' ':
					is_hidden = true;
					break;
			}

			if ( is_hidden && config.builtin_text_editor_show_invisible_characters == false ) {
				switch ( note->content[ i ] ) {
					case '\n':
						y++;
						x = 0;
						break;
					case '\t':
						x += 4;
						break;
					case ' ':
						x++;
						break;
				}
				continue;
			}

			if ( i == cursor ) attron( COLOR_PAIR( TEXT_HIDDEN_HIGHLIGHT ) );
			else if ( is_hidden ) attron( COLOR_PAIR( TEXT_HIDDEN ) );

			switch ( note->content[ i ] ) {
				case 0:
					mvprintw( y, x, "." );
					x++;
					break;

				case '\n':
					mvprintw( y, x, "\\n" );
					y++;
					x = 0;
					break;

				case '\t':
					mvprintw( y, x, "  \\t" );
					x += 4;
					break;

				case ' ':
					mvprintw( y, x, "_" );
					x++;
					break;

				default:

					if ( i == cursor ) attron( COLOR_PAIR( TEXT_HIGHLIGHT ) );

					mvprintw( y, x, "%c", note->content[ i ] );
					x++;

					if ( i == cursor ) attroff( COLOR_PAIR( TEXT_HIGHLIGHT ) );
			}

			if ( i == cursor ) attroff( COLOR_PAIR( TEXT_HIDDEN_HIGHLIGHT ) );
			else if ( is_hidden ) attroff( COLOR_PAIR( TEXT_HIDDEN ) );

			if ( note->content[ i ] == 0 ) break;
		}

		refresh();

		int ch = get_input();
		switch ( ch ) {

			case KEY_ESCAPE: ; /* ESC */
				/* show menu with options like: save, exit without saving, back */
				int t = note_menu( note );
				if ( t == 1 /* EXIT */ ) {
					run = false;
				}
				break;

			case KEY_BACKSPACE:
				if ( cursor != 0 ) {
				
					/* move note text after cursor 1 cell lower */
					size_t i;
					for ( i = cursor; note->content[ i ] != 0; i++ ) {
						note->content[ i - 1 ] = note->content[ i ];
					}

					note->content[ i - 1 ] = 0;

					cursor--;
				}
				break;

			case KEY_DC: /* DELETE */ {
				
					/* move note text after 'cursor + 1' 1 cell lower */
					size_t i;
					for ( i = cursor + 1; note->content[ i ] != 0; i++ ) {
						note->content[ i - 1 ] = note->content[ i ];
					}

					note->content[ i - 1 ] = 0;

				}
				break;

			case KEY_LEFT:
				if ( cursor != 0 ) cursor--;
				break;

			case KEY_RIGHT:
				if ( note->content[ cursor ] != 0 ) cursor++;
				break;

			case KEY_UP: ; /* move one line up */
				/* find ending of the line (is this line the first one) */
				const char* line_end = NULL;
				for ( const char* i = note->content + cursor; i != note->content; i-- ) {
					if ( *i == '\n' ) {
						line_end = i;
						break;
					}
				}
				
				/* if prev line is found */
				if ( line_end != NULL ) {
					/* get start of prev line */
					const char* prev_line_end = note->content;
					for ( const char* i = line_end - 1; i != note->content; i-- ) {
						if ( *i == '\n' ) {
							prev_line_end = i;
							break;
						}
					}

					/* move the cursor to the same place on the prev line */
					const size_t space_from_start_of_this_line = note->content + cursor - line_end;
					const size_t prev_line_start = prev_line_end - note->content;

					/* if selected line is longer than the one above */
					if ( prev_line_start + space_from_start_of_this_line + note->content > line_end ) {
						cursor = line_end - note->content;
					} else
						cursor = prev_line_start + space_from_start_of_this_line;
				}
				break;

			case KEY_DOWN:  ; /* move one line down */
				/* find the start of the next line */
				const char* line_start = strchr( note->content + cursor, '\n' );
				
				/* if next line is found */
				if ( line_start != NULL ) {
					/* get start of selected line */
					const char* sel_line_start = note->content;
					for ( const char* i = note->content + cursor; i != note->content; i-- ) {
						if ( *i == '\n' ) {
							sel_line_start = i;
							break;
						}
					}

					/* get end of next line */
					const char* next_line_end = note->content + strlen( note->content ) - 1;
					const char* nle_temp = strchr( line_start + 1, '\n' );
					if ( nle_temp != NULL ) next_line_end = ( next_line_end > nle_temp ) ? nle_temp : next_line_end;


					/* move the cursor to the same place on the prev line */
					const size_t space_from_start_of_this_line = note->content + cursor - sel_line_start;
					const size_t next_line_start = line_start - note->content;

					/* TODO: fix this mess */

					/* if the next line is shorter */
					if ( next_line_start + space_from_start_of_this_line < space_from_start_of_this_line ) {
						cursor = next_line_end - note->content;
					} else
						cursor = next_line_start + space_from_start_of_this_line;
				}
				break;

			default:

				note_saved = 0;

				/* cursor at the end of the note */
				if ( note->content[ cursor ] == 0 ) {
					note->content[ cursor ] = ch;
					note_content_size++;
					note->content = realloc( note->content, note_content_size + 1 );

					note->content[ note_content_size ] = 0;

					cursor++;
				} else {

					/* increase size of note */
					note_content_size++;
					note->content = realloc( note->content, note_content_size + 1 );
					note->content[ note_content_size ] = 0;

					/* move text after cursor 1 cell higher */
					for ( size_t i = note_content_size - 1; i != cursor - 1; i-- ) {
						note->content[ i + 1 ] = note->content[ i ];
					}
					note->content[ cursor ] = ch;
					cursor++;

				}

		}

	}

	return;
}

int
note_menu( struct note* p_note ) {

	int selector = 0;
	const char* menu[] = {
		"Save",
		"Exit",
		"Exit without saving",
		"Back",		
	};

	size_t menu_longest = 0;
	for ( size_t i = 0; i < arraysize( menu ); i++ ) {
		size_t menu_i_len = strlen( menu[ i ] );
		if ( menu_longest < menu_i_len )
			menu_longest = menu_i_len;
	}
		

	while ( 1 ) {

		clear();

		fill_screen();

		int w = getmaxx( stdscr ), h = getmaxy( stdscr );

		for ( size_t i = 0; i < arraysize( menu ); i++ ) {
			if ( selector == i ) attron( A_REVERSE );
			move( ( h - arraysize( menu ) ) / 2 + i, ( w - menu_longest ) / 2 );
			addstr( menu[ i ] );
			if ( selector == i ) attroff( A_REVERSE );
		}

		refresh();

		int ch = get_input();
		switch ( ch ) {
			case 27:
				return 0;
			case '\n': case KEY_ENTER:
				switch ( selector ) {
					case 0: /* SAVE */
						p_note->last_edit = time( NULL );
						save_note( p_note );
						return 0;
					case 1: /* EXIT */
						p_note->last_edit = time( NULL );
						save_note( p_note );
						return 1;
					case 2: /* EXIT NO SAVE */
						return 1;
					case 3: /* BACK */
						return 0;
				}
				break;
			case KEY_UP:
				if ( selector != 0 ) selector--;
				break;
			case KEY_DOWN:
				if ( selector != arraysize( menu ) - 1 ) selector++;
				break;
		}

		if ( ch == 27 ) break;

	}

	return 0;
}

void
note_editor_external( const char* p_note_path, enum text_editor_program config_text_editor ) {
	add_log( "Openning note %s with external editor (%s)" );

	/* IDEA: copy content part of the note to /tmp with known name, run external text editor on it and after it exits copy the edited file back to its original place (after adding the metadata JSON to it) */

	struct note* note = load_note( p_note_path );

	/* open temp file */
	char note_code[ 64 ];
	sprintf( note_code, "%08i", rand() % 10000000 );
	char temp_note_path[ 128 ];
	sprintf( temp_note_path, "/tmp/npt-note-%s", note_code );
	FILE* temp_note = fopen( temp_note_path, "w" );

	/* write content to temp file */
	fputs( note->content, temp_note );

	/* close temp file */
	fclose( temp_note );

	/* open temp file in text editor of choice */
	endwin();
	char system_call_string[ 1024 ];
	sprintf( system_call_string, "vim %s", temp_note_path );
	int so = system( system_call_string );

	if ( so != 0 ) {
		printf( "External text editor returned code: %i, press ENTER to continue", so );
		while ( getch() != '\n' );
	}

	ncurses_init();

	/* open temp file */
	temp_note = fopen( temp_note_path, "r" );

	/* get its contents */
	fseek( temp_note, 0, SEEK_END );
	size_t note_content_size = ftell( temp_note );
	rewind( temp_note );

	char* temp_note_content = malloc( note_content_size + 1 );
	size_t fread_size = fread( temp_note_content, sizeof( char ), note_content_size, temp_note );
	temp_note_content[ note_content_size ] = 0;

	fclose( temp_note );

	/* compare content and new content */
	if ( strcmp( note->content, temp_note_content ) != 0 ) {
		/* update metadata */
		note->last_edit = time( NULL );
		/* TODO: */

		free( note->content );
		note->content = temp_note_content;

		save_note( note );

	}

	/* free */
	free_note( note );

	return;
}