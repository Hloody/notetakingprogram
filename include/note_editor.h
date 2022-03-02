#pragma once

#include "notetaking.h"

#include "config.h"

void note_editor( const char* );
void note_editor_external( const char*, enum text_editor_program );

int  note_menu( struct note* );