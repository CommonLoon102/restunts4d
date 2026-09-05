#ifndef RESTUNTS_KEYBOARD_H
#define RESTUNTS_KEYBOARD_H

#include "legacy.h"

#define KEY_BACKSPACE 8U
#define KEY_TAB 9U
#define KEY_ENTER 13U
#define KEY_ESCAPE 27U
#define KEY_SPACE 32U
#define KEY_F1 15104U
#define KEY_F2 15360U
#define KEY_F3 15616U
#define KEY_F4 15872U
#define KEY_F5 16128U
#define KEY_F6 16384U
#define KEY_F7 16640U
#define KEY_F8 16896U
#define KEY_F9 17152U
#define KEY_F10 17408U
#define KEY_HOME 18176U
#define KEY_UP 18432U
#define KEY_LEFT 19200U
#define KEY_RIGHT 19712U
#define KEY_END 20224U
#define KEY_DOWN 20480U
#define KEY_INSERT 20992U
#define KEY_DELETE 21248U
#define KEY_SHIFT_F1 21504U

void kb_init_interrupt(void);
void kb_exit_handler(void);
legacy_s16 kb_get_key_state(legacy_s16 key);
legacy_s16 dos_kb_get_char(void);
void dos_kb_set_numlock(void);
void dos_kb_clear_numlock(void);
legacy_s16 kb_call_readchar_callback(void);
legacy_s16 kb_read_char(void);
legacy_s16 kb_checking(void);
void flush_stdin(void);
legacy_s16 kb_check(void);

#endif
