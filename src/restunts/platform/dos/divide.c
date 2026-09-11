#include "dos_interrupts.h"

#include "platform.h"

typedef void interrupt(far *interrupt_handler_type)();

#define DOS_DIVIDE_INSTRUCTION_SIZE 2U

extern legacy_u16 legacy_divide_fault_segment;
extern legacy_u16 legacy_divide_fault_offset;

static interrupt_handler_type previous_divide_error_handler;

static void interrupt dos_divide_error_handler(union INTPACK registers)
{
	legacy_divide_fault_segment = registers.w.cs;
	legacy_divide_fault_offset = registers.w.ip;
	registers.w.ip = LEGACY_U16_WRAP_ADD(registers.w.ip, DOS_DIVIDE_INSTRUCTION_SIZE);
	registers.w.ax = 0;
}

void dos_install_divide_error_handler(void)
{
	previous_divide_error_handler = _dos_getvect(0);
	_dos_setvect(0, dos_divide_error_handler);
}
