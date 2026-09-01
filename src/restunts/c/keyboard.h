#ifndef RESTUNTS_KEYBOARD_H
#define RESTUNTS_KEYBOARD_H

#include "legacy.h"

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
