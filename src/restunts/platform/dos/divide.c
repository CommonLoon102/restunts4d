#include "dos_interrupts.h"

#include "platform.h"

typedef void interrupt(far *interrupt_handler_type)();

#define DOS_DIVIDE_INSTRUCTION_SIZE 2U

extern legacy_u16 legacy_divide_fault_segment;
extern legacy_u16 legacy_divide_fault_offset;

static interrupt_handler_type previous_divide_error_handler;

#ifndef __WATCOMC__
#pragma argsused
#endif
static void interrupt dos_divide_error_handler(DOS_INTERRUPT_REGISTERS)
{
	legacy_divide_fault_segment = DOS_INTERRUPT_CS;
	legacy_divide_fault_offset = DOS_INTERRUPT_IP;
	DOS_INTERRUPT_IP = LEGACY_U16_WRAP_ADD(DOS_INTERRUPT_IP, DOS_DIVIDE_INSTRUCTION_SIZE);
	DOS_INTERRUPT_AX = 0;
}

void dos_install_divide_error_handler(void)
{
	previous_divide_error_handler = _getvect(0);
	_setvect(0, dos_divide_error_handler);
}
