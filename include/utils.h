#pragma once

/* mkdir */
#include <sys/stat.h>

#define arraysize( X ) ( sizeof( X ) / sizeof( *X ) )
#define min( X, Y ) ( X < Y ? X : Y )
#define max( X, Y ) ( X > Y ? X : Y )

/* recursive mkdir */
void rec_mkdir( const char* path, mode_t mode );

#define MKDIR_DEFAULT_MODE 0770

const char* get_notes_dir( void );

#define CTRL( X ) ( X | 0x1000 )
#define KEY_ESCAPE CTRL( 0 )

int get_input( void ); /* replaces getch */

/* ncurses colour pairs */
#define DEFAULT_PAIR           1
#define SELECTOR_PAIR          2
#define HIGHLIGHT_PAIR         3
#define TEXT_HIGHLIGHT         4
#define TEXT_HIDDEN_HIGHLIGHT  5
#define TEXT_HIDDEN            6
#define FOOTER_PAIR            7
#define HIGHLIGHT_SEGMENT_PAIR 8

#define COLOR_GRAY COLOR_RED
#define COLOR_DARK_GRAY COLOR_BLUE

void ncurses_init( void );

/* addchars ' ' to all cells of the terminal */
void fill_screen( void );

#include "config.h"

const char* text_editor_to_string( enum text_editor_program );
const char* display_to_string( enum display_program );
const char* sort_function_to_string( enum sort_function );

