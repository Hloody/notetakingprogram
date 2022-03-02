#include "note_renderer.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "utils.h"

extern int stdscr;

struct rendered_note
note_render_text( const char* p_note_something ) {

	struct rendered_note output;
	output.count = 0;
	output.segments = malloc( 1 );

	size_t prev_segment_end = 0;

	size_t j = 0;
	for ( size_t i = 0; i < strlen( p_note_something ); i++ ) {

		//printf( "looking at %c\n", p_note_something[ i ] );
		
		/* start of special sequence */
		if ( p_note_something[ i ] == '[' && p_note_something[ i + 1 ] == '[' && p_note_something[ i + 2 ] != '[' ) {

			/* add previous segment as 'text' */
			if ( prev_segment_end != i ) {
				output.count++;
				output.segments = realloc( output.segments, output.count * sizeof( struct note_segment ) );
				output.segments[ output.count - 1 ].type = st_text;

				output.segments[ output.count - 1 ].text = malloc( i - prev_segment_end + 1 );
				strncpy( output.segments[ output.count - 1 ].text, p_note_something + prev_segment_end, i - prev_segment_end );
				output.segments[ output.count - 1 ].text[ i - prev_segment_end ] = 0;

				prev_segment_end = i - 1;
			}

			/* search for an end on the same line */
			size_t segment_count = 1;
			enum segment_type segment_type = st_unknown;
			int  segment_end = -1;
			for ( size_t ii = 0; p_note_something[ i + ii ] != 0 || p_note_something[ i + ii ] != '\n'; ii++ ) {
				if ( p_note_something[ i + ii ] == '|' ) {
					segment_count++;
					continue;
				}

				if ( p_note_something[ i + ii ] == ']' && p_note_something[ i + ii + 1 ] == ']' ) {
					segment_end = ii;
					break;
				}

			}

			/*
			printf( "segment_count %lu, segment_end %i\n", segment_count, segment_end );
			*/

			if ( segment_end == -1 ) {
				break;
			}

			/* split it into segments */
			char* segments[ segment_count ];
			size_t segment_size = 0;
			size_t segment_index = 0;

			for ( size_t ij = 2; true; ij++ ) {

				if ( ij == segment_end || p_note_something[ i + ij ] == '|' ) {

					segments[ segment_index ] = malloc( ( segment_size + 1 ) * sizeof( char ) );

					memcpy( segments[ segment_index ], &p_note_something[ i + ij - segment_size ], segment_size );
					segments[ segment_index ][ segment_size ] = 0;

					segment_size = 0;
					segment_index++;

					if ( ij == segment_end ) break;

				} else {
					segment_size++;
				}
			}

			/*
			printf( "segments (%lu):\n", segment_count );
			for ( size_t i = 0; i < segment_count; i++ ) {
				printf( " %lu. segment '%s'\n", i, segments[ i ] );
			}
			*/
			

			/* get segment type -> read first segment */
			if ( strcmp( segments[ 0 ], "TD" ) == 0 ) {
				free( segments[ 0 ] );
				segment_type = st_todo;
			}

			switch ( segment_type ) {
				case st_todo: {
					/* write '[priority][status][date] have fun' instead of '[[TD|priority|goal|date|status]]' */
					/* priority can be empty || = 0 */
					/* date and status can be missing */

					/* TODO: rewrite, parse segments as segment_list */
					/* move this into a function that gets a string and returns a pointer to a special segment (NULL if not found) and the number of characters the segment consists of */

					/* */
					output.count++;
					output.segments = realloc( output.segments, output.count * sizeof( struct note_segment ) );

					output.segments[ output.count - 1 ].type = st_todo;
					struct segment_todo* todo = &output.segments[ output.count - 1 ].todo;
					todo->valid = true;

					switch ( segment_count - 1 ) {
						case 1:
							todo->goal = segments[ 1 ];

							todo->priority = NULL;
							todo->date     = NULL;
							todo->status   = NULL;
							break;

						case 2:
							todo->priority = segments[ 1 ];
							todo->goal     = segments[ 2 ];

							todo->date   = NULL;
							todo->status = NULL;
							break;

						case 3:
							todo->priority = segments[ 1 ];
							todo->goal     = segments[ 2 ];
							todo->status   = segments[ 3 ];

							todo->date = NULL;
							break;

						case 4:
							todo->priority = segments[ 1 ];
							todo->goal     = segments[ 2 ];
							todo->date     = segments[ 3 ];
							todo->status   = segments[ 4 ];
							break;

						default:
							todo->valid = false;

							/* free all segments */
							for ( size_t i = 1; i < segment_count; i++ ) {
								free( segments[ i ] );
							}
							break;
					}

					/* skip if not valid */
					if ( todo->valid == false ) {
						i += segment_end + 1;
						break;
					}

					/* empty priority ==> 0 */
					if ( todo->priority == NULL || strlen( todo->priority ) == 0 ) {
						if ( todo->priority != NULL ) free( todo->priority );

						todo->priority = malloc( 2 );
						strcpy( todo->priority, "0" );
					}

#ifdef AhfoauhfHO
					/* TEMP: print TODO */

					/* priority */
					out[ j ] = '[';
					j++;

					int priority_size = strlen( todo.priority );
					memcpy( out + j, todo.priority, priority_size );
					j += priority_size;

					out[ j ] = ']';
					j++;

					/* status */
					if ( todo.status != NULL ) {
						out[ j ] = '[';
						j++;

						int status_size = strlen( todo.status );
						memcpy( out + j, todo.status, status_size );
						j += status_size;

						out[ j ] = ']';
						j++;
					}
					
					/* date/deadline */
					if ( todo.date != NULL ) {
						out[ j ] = '[';
						j++;

						int date_size = strlen( todo.date );
						memcpy( out + j, todo.date, date_size );
						j += date_size;

						out[ j ] = ']';
						j++;
					}

					out[ j ] = ' ';
					j++;

					/* goal */
					int goal_size = strlen( todo.goal );
					memcpy( out + j, todo.goal, goal_size );
					j += goal_size;
#endif
					i += segment_end + 1 + 2;
					prev_segment_end = i - 1;

					}

					/* printf( "'printed'\n" ); */
					break;

				/* unknown segment type */
				default:
					break;
			}


		}

		continue;
	}

	/* add previous segment as 'text' */
	size_t note_length = strlen( p_note_something );
	if ( prev_segment_end != note_length - 1 ) {
		output.count++;
		output.segments = realloc( output.segments, output.count * sizeof( struct note_segment ) );
		output.segments[ output.count - 1 ].type = st_text;

		output.segments[ output.count - 1 ].text = malloc( note_length - prev_segment_end + 1 );
		strncpy( output.segments[ output.count - 1 ].text, p_note_something + prev_segment_end, note_length - prev_segment_end );
		output.segments[ output.count - 1 ].text[ note_length - prev_segment_end ] = 0;

		prev_segment_end = note_length - 1;
	}

	return output;
}

void
free_rendered_note( struct rendered_note p_rn ) {
	for ( size_t i = 0; i < p_rn.count; i++ ) {
		switch ( p_rn.segments[ i ].type ) {
			case st_text:
				free( p_rn.segments[ i ].text );
				break;
		}
	}
}