#ifndef RESTUNTS_DOS_INTERRUPTS_H
#define RESTUNTS_DOS_INTERRUPTS_H

#include <dos.h>
#include "../../c/legacy.h"

typedef void interrupt(far *dos_interrupt_handler_type)();

/* Unprefixed C symbols for the Borland runtime's interrupt-vector entry points. */
void _CType _setvect(legacy_s16 interrupt_number, dos_interrupt_handler_type handler);
dos_interrupt_handler_type _CType _getvect(legacy_s16 interrupt_number);

#endif
