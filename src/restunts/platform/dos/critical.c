#include <dos.h>

#include "../../c/legacy.h"

typedef void interrupt (far* interrupt_handler_type)();
typedef legacy_s16 (far* critical_error_callback_type)(void);
typedef void (far* exit_handler_type)(void);

extern void add_exit_handler(exit_handler_type exit_handler);
extern void _CType _setvect(legacy_s16 interrupt_number,
	interrupt_handler_type handler);
extern interrupt_handler_type _CType _getvect(legacy_s16 interrupt_number);

static interrupt_handler_type previous_critical_error_handler;
static critical_error_callback_type critical_error_callback;

static void far dos_critical_error_restore(void)
{
	if (previous_critical_error_handler != 0)
		_setvect(0x24, previous_critical_error_handler);
}

#pragma argsused
static void interrupt dos_critical_error_handler(legacy_u16 bp,
	legacy_u16 di, legacy_u16 si, legacy_u16 ds, legacy_u16 es,
	legacy_u16 dx, legacy_u16 cx, legacy_u16 bx, legacy_u16 ax,
	legacy_u16 ip, legacy_u16 cs, legacy_u16 flags)
{
	ax = (legacy_u16)critical_error_callback();
}

void dos_set_critical_error_handler(critical_error_callback_type callback)
{
	add_exit_handler(dos_critical_error_restore);
	critical_error_callback = callback;
	previous_critical_error_handler = _getvect(0x24);
	_setvect(0x24, dos_critical_error_handler);
}
