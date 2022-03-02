#include "config.h"

#include "utils.h"
#include "logger.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <curses.h>
#include <jansson.h>

const struct config default_config = {
	.render_notes = false,
	.builtin_text_editor_show_invisible_characters = true, /* add this to settings and config */
	.show_note_preview = true,
	.light_mode = false,
	.colour_background = false,
	.colour_segments = true,

	.display = display_builtin,
	.text_editor   = text_editor_builtin,
	.sort_function = sort_alphabet_ascending
};

struct config config = default_config;

static
const char*
get_config_file_path( void ) {
	static char* output = NULL;
	if ( output != NULL ) return output;

	char* home_env = getenv( "HOME" );
	if ( home_env == NULL ) {
		if ( stdscr ) endwin();
		puts( "Failed to load config file:\nenviromant variable 'HOME' does not exist" );
		exit( 1 );
	}

	const char* rel_config_path = "/.ntp/config.json";

	output = malloc( 1024 );

	strcpy( output, home_env );
	strcat( output, rel_config_path );

	return output;
}

bool
string_to_text_editor_program( const char* p_str, enum text_editor_program* p_te ) {
	if ( strcmp( p_str, text_editor_to_string( text_editor_builtin ) ) == 0 ) {
		*p_te = text_editor_builtin;
		return true;
	}

	if ( strcmp( p_str, text_editor_to_string( text_editor_vim ) ) == 0 ) {
		*p_te = text_editor_vim;
		return true;
	}

	if ( strcmp( p_str, text_editor_to_string( text_editor_nano ) ) == 0 ) {
		*p_te = text_editor_nano;
		return true;
	}

	return false;
}

bool
string_to_display_program( const char* p_str, enum display_program* p_d ) {
	if ( strcmp( p_str, display_to_string( display_builtin ) ) == 0 ) {
		*p_d = display_builtin;
		return true;
	}

	if ( strcmp( p_str, display_to_string( display_less ) ) == 0 ) {
		*p_d = display_less;
		return true;
	}

	return false;
}

bool
string_to_sort_function( const char* p_str, enum sort_function* p_sf ) {
	if ( strcmp( p_str, sort_function_to_string( sort_alphabet_ascending ) ) == 0 ) {
		*p_sf = sort_alphabet_ascending;
		return true;
	}

	if ( strcmp( p_str, sort_function_to_string( sort_alphabet_descending ) ) == 0 ) {
		*p_sf = sort_alphabet_descending;
		return true;
	}

	return false;
}

void
config_load( void ) {
	add_log( "loading config" );

	const char* config_path = get_config_file_path();

	FILE* config_file = fopen( config_path, "r" );
	if ( config_file == NULL ) {
		/* check for error */
		add_log( "error: failed to open config file" );

		return;
	}

	fseek( config_file, 0L, SEEK_END );
	long config_file_size = ftell( config_file );
	rewind( config_file );

	char* config_file_contents = malloc( config_file_size + 1 );
	if ( config_file_contents == NULL ) {
		add_log( "error: failed to malloc for config_file_contents" );

		fclose( config_file );
		return;
	}

	size_t read = fread( config_file_contents, sizeof( char ), config_file_size, config_file );
	config_file_contents[ config_file_size ] = 0;

	fclose( config_file );

	if ( read != config_file_size ) {
		char buffer[ 1024 ];
		sprintf( buffer, "error: failed to read file; %lu/%lu bytes", read, config_file_size );

		add_log( buffer );
		return;
	}

	/* config variables - cache - use defaults */
	struct config _config = config;

	/* parse config */
	json_error_t json_error;
	json_t* config_root = json_loads( config_file_contents, 0, &json_error );
	free( config_file_contents );

	if ( config_root == NULL ) {
		char buffer[ 1024 ];
		sprintf( buffer, "error: failed to parse config json; error: %s", json_error.text );

		add_log( buffer );
		return;
	}

	if ( json_is_object( config_root ) == false ) {
		add_log( "error: config json does not match expectations" );
		json_decref( config_root );
		return;
	}

	/* get config_display */
	json_t* json_config_display = json_object_get( config_root, "display" );
	if ( json_config_display == NULL ) add_log( "warning: missing 'display', skipping" );
	else {
		if ( json_is_string( json_config_display ) == false )
			add_log( "error: invalid 'display', skipping" );
		else {
			/* parse string */
			bool json_config_display_parsed = string_to_display_program( json_string_value( json_config_display ), &_config.display );
			if ( json_config_display_parsed == false )
				add_log( "error: invalid 'display' value, skipping" );
		}
	}


	/* get config_text_editor */
	json_t* json_config_text_editor = json_object_get( config_root, "text_editor" );
	if ( json_config_text_editor == NULL ) add_log( "warning: missing 'text_editor', skipping" );
	else {
		if ( json_is_string( json_config_text_editor ) == false )
			add_log( "error: invalid 'text_editor', skipping" );
		else {
			/* parse string */
			bool json_config_text_editor_parsed = string_to_text_editor_program( json_string_value( json_config_text_editor ), &_config.text_editor );
			if ( json_config_text_editor_parsed == false )
				add_log( "error: invalid 'text_editor' value, skipping" );
		}
	}


	/* get config_sort_function */
	json_t* json_config_sort_function = json_object_get( config_root, "sort_function" );
	if ( json_config_sort_function == NULL ) add_log( "warning: missing 'sort_function', skipping" );
	else {
		if ( json_is_string( json_config_sort_function ) == false )
			add_log( "error: invalid 'sort_function', skipping" );
		else {
			/* parse string */
			bool json_config_sort_function_parsed = string_to_sort_function( json_string_value( json_config_sort_function ), &_config.sort_function );
			if ( json_config_sort_function_parsed == false )
				add_log( "error: invalid 'sort_function' value, skipping" );
		}
	}


	/* get config_render_notes */
	json_t* json_config_render_notes = json_object_get( config_root, "render_notes" );
	if ( json_config_render_notes == NULL ) add_log( "warning: missing 'render_notes', skipping" );
	else {
		if ( json_is_boolean( json_config_render_notes ) == false )
			add_log( "error: invalid 'render_notes', skipping" );
		else
			_config.render_notes = json_boolean_value( json_config_render_notes );
	}

	/* get config_light_mode */
	json_t* json_config_light_mode = json_object_get( config_root, "light_mode" );
	if ( json_config_light_mode == NULL ) add_log( "warning: missing 'light_mode', skipping" );
	else {
		if ( json_is_boolean( json_config_light_mode ) == false )
			add_log( "error: invalid 'light_mode', skipping" );
		else
			_config.light_mode = json_boolean_value( json_config_light_mode );
	}

	/* get config_colour_background */
	json_t* json_config_colour_background = json_object_get( config_root, "colour_background" );
	if ( json_config_colour_background == NULL ) add_log( "warning: missing 'colour_background', skipping" );
	else {
		if ( json_is_boolean( json_config_colour_background ) == false )
			add_log( "error: invalid 'colour_background', skipping" );
		else
			_config.colour_background = json_boolean_value( json_config_colour_background );
	}

	/* set config variable */
	config = _config;

	json_decref( config_root );

	add_log( "config loaded" );

	return;
}

void
config_save( void ) {
	add_log( "saving config" );

	const char* config_path = get_config_file_path();

	FILE* config_file = fopen( config_path, "w" );

	/* generate json */
	json_t* output_json = json_pack(
		"{ ss ss ss sb sb sb }",
		/* display  text_editor  sort_function  render_notes light_mode colour_background */
		"display",            display_to_string( config.display ),
		"text_editor",        text_editor_to_string( config.text_editor ),
		"sort_function",      sort_function_to_string( config.sort_function ),
		"render_notes",       config.render_notes,
		"light_mode",         config.light_mode,
		"colour_background",  config.colour_background
	);

	char* output_json_text = json_dumps( output_json, 0 );

	json_decref( output_json );

	fwrite( output_json_text, strlen( output_json_text ), sizeof( char ), config_file );

	free( output_json_text );

	fclose( config_file );

	add_log( "config saved" );

	return;
}