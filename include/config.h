#pragma once

#include <stdbool.h>

enum display_program {
	display_builtin,
	display_less,
	display__END
};

enum text_editor_program {
	text_editor_builtin,
	text_editor_vim,
	text_editor_nano,
	text_editor__END
};

enum sort_function {
	sort_alphabet_ascending,
	sort_alphabet_descending,
	sort_function__END
};

struct config {
	bool render_notes;
	bool builtin_text_editor_show_invisible_characters;
	bool show_note_preview;
	bool light_mode;
	bool colour_background;
	bool colour_segments;

	enum display_program display;
	enum text_editor_program text_editor;
	enum sort_function sort_function;
};

extern struct config config;
extern const struct config default_config;

/* file management */
void config_load( void );
void config_save( void );