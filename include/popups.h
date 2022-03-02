#pragma once

#include "notetaking.h"

void popup_open_note( const char* );
const char* popup_search( void );
void popup_sort();
void popup_delete();
void popup_notification( const char* message );
void popup_notification_nonblocking( const char* message );
void note_display( const char* );
void popup_settings( void );
char* popup_rename_note( const char* note_path );
void popup_edit_tags( const char* note_path );