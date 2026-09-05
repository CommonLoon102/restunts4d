#include <dos.h>

#include "platform.h"

typedef void interrupt (far* interrupt_handler_type)();

#define DOS_DIVIDE_INSTRUCTION_SIZE 2U

extern void _CType _setvect(legacy_s16 interrupt_number,
	interrupt_handler_type handler);
extern interrupt_handler_type _CType _getvect(legacy_s16 interrupt_number);
extern legacy_u16 word_3BE30;
extern legacy_u16 word_3BE32;

static interrupt_handler_type previous_divide_error_handler;

#pragma argsused
static void interrupt dos_divide_error_handler(legacy_u16 bp,
	legacy_u16 di, legacy_u16 si, legacy_u16 ds, legacy_u16 es,
	legacy_u16 dx, legacy_u16 cx, legacy_u16 bx, legacy_u16 ax,
	legacy_u16 ip, legacy_u16 cs, legacy_u16 flags)
{
	word_3BE30 = cs;
	word_3BE32 = ip;
	ip = LEGACY_U16_WRAP_ADD(ip, DOS_DIVIDE_INSTRUCTION_SIZE);
	ax = 0;
}

void dos_install_divide_error_handler(void)
{
	previous_divide_error_handler = _getvect(0);
	_setvect(0, dos_divide_error_handler);
}
