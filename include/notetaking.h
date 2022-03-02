#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

struct note {
	char* path;
	char* content;

	size_t size;

	/* meta data */
	time_t created;
	time_t last_edit;
	
	char** tags;
	size_t tag_count;
};

struct notes_list {
	char** note_paths;
	size_t note_count;
};

extern struct notes_list notes_list;

void ntp_main( void );
void scan_notes( void );

/* returns a pointer to note's content */
/* return NULL if it failed to open note's file */
char* load_note_raw( const char* note_path );

struct note* load_note( const char* note_path );
bool save_note( struct note* );

void free_note( struct note* );