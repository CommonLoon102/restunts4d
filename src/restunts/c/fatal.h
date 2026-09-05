#ifndef RESTUNTS_FATAL_H
#define RESTUNTS_FATAL_H

#include "legacy.h"

#define EXIT_HANDLER_MAX_COUNT 10
#define EXIT_HANDLER_SLOT_COUNT (EXIT_HANDLER_MAX_COUNT + 1U)

void add_exit_handler(void (far* exit_handler)(void));
void call_exitlist(void);
void call_exitlist2(void);
void fatal_error(const legacy_s8* format, ...);

#endif
