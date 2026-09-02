#ifndef RESTUNTS_FATAL_H
#define RESTUNTS_FATAL_H

#include "legacy.h"

void add_exit_handler(void (far* exit_handler)(void));
void call_exitlist(void);
void call_exitlist2(void);
void fatal_error(const legacy_s8* format, ...);

#endif
