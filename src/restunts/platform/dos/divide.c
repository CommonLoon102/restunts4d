#include "platform.h"
#include "doscompat.h"

extern legacy_u16 word_3BE30;
extern legacy_u16 word_3BE32;

static dos_interrupt_handler_type previous_divide_error_handler;

#pragma argsused
#if defined(__WATCOMC__)
static void interrupt dos_divide_error_handler(union INTPACK registers)
#else
static void interrupt dos_divide_error_handler(legacy_u16 bp,
	legacy_u16 di, legacy_u16 si, legacy_u16 ds, legacy_u16 es,
	legacy_u16 dx, legacy_u16 cx, legacy_u16 bx, legacy_u16 ax,
	legacy_u16 ip, legacy_u16 cs, legacy_u16 flags)
#endif
{
#if defined(__WATCOMC__)
	word_3BE30 = registers.w.cs;
	word_3BE32 = registers.w.ip;
	registers.w.ip = LEGACY_U16_WRAP_ADD(registers.w.ip, 2U);
	registers.w.ax = 0;
#else
	word_3BE30 = cs;
	word_3BE32 = ip;
	ip = LEGACY_U16_WRAP_ADD(ip, 2U);
	ax = 0;
#endif
}

void dos_install_divide_error_handler(void)
{
	previous_divide_error_handler = dos_getvect(0);
	dos_setvect(0, dos_divide_error_handler);
}
