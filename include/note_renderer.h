#pragma once

#include <stdbool.h>
#include <stddef.h>

enum segment_type {
	st_text,
	st_todo,

	st_unknown,
};

struct segment_todo {
	char* priority;
	char* goal;
	char* date;
	char* status;

	bool valid;
};

struct note_segment {
	enum segment_type type;
	union {
		struct segment_todo todo;
		char* text;
	};
};

struct rendered_note {
	size_t count;
	struct note_segment* segments;
};

struct rendered_note note_render_text( const char* );
void free_rendered_note( struct rendered_note );