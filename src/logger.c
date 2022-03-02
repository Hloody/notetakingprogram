#include "logger.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"

/* TODO: test this function */
/* TODO: add va list (printf) */
void
add_log( const char* p_message ) {

	/* open log file */
	char* home_dir = getenv( "HOME" );
	if ( home_dir == NULL ) return;

	char buffer[ 1024 ];
	sprintf( buffer, "%s/.ntp/logs/", home_dir );

	rec_mkdir( buffer, MKDIR_DEFAULT_MODE );

	/* get date */
	time_t t = time( NULL );
	struct tm* tm = localtime( &t );
	char date[ strlen( "YYYYMMDD.log" ) + 1 + 64 ];
	sprintf( date, "%04i%02i%02i.log", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday );

	strcat( buffer, date );

	FILE* log_file = fopen( buffer, "a" );
	if ( log_file == NULL ) {
		printf( "Error: failed to open %s\n", buffer );
		return;
	}

	/* write log message */
	/*   get timestamp */
	char timestamp[ strlen( "[20211215 21:42:54]" ) + 1 + 64 /* disable warning */ ];

	sprintf( timestamp, "[%4i%02i%02i %2i:%02i:%02i]", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );

	fwrite( timestamp, sizeof( char ), strlen( timestamp ), log_file );
	fwrite( " ",       sizeof( char ), 1,                   log_file );
	fwrite( p_message, sizeof( char ), strlen( p_message ), log_file );
	fwrite( "\n",      sizeof( char ), 1,                   log_file );

	/* close log file */
	fclose( log_file );

	return;
}