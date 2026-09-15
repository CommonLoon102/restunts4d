#include "dos_interrupts.h"

#include "../../c/legacy.h"
#include "../../c/fatal.h"

#define DOS_CRITICAL_ERROR_INTERRUPT_VECTOR 36

typedef void interrupt(far *interrupt_handler_type)();
typedef legacy_s16(far *critical_error_callback_type)(void);

static interrupt_handler_type previous_critical_error_handler;
static critical_error_callback_type critical_error_callback;

void dos_interrupts_disable(void)
{
	disable();
}

void dos_interrupts_enable(void)
{
	enable();
}

static void far dos_critical_error_restore(void)
{
	if (previous_critical_error_handler != 0) {
		_setvect(DOS_CRITICAL_ERROR_INTERRUPT_VECTOR, previous_critical_error_handler);
	}
}

#ifndef __WATCOMC__
#pragma argsused
#endif
static void interrupt dos_critical_error_handler(DOS_INTERRUPT_REGISTERS)
{
	DOS_INTERRUPT_AX = (legacy_u16)critical_error_callback();
}

void dos_set_critical_error_handler(critical_error_callback_type callback)
{
	add_exit_handler(dos_critical_error_restore);
	critical_error_callback = callback;
	previous_critical_error_handler = _getvect(DOS_CRITICAL_ERROR_INTERRUPT_VECTOR);
	_setvect(DOS_CRITICAL_ERROR_INTERRUPT_VECTOR, dos_critical_error_handler);
}
