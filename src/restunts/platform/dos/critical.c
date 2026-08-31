#include "doscompat.h"

typedef legacy_s16 (far* critical_error_callback_type)(void);
typedef void (far* exit_handler_type)(void);

extern void add_exit_handler(exit_handler_type exit_handler);

static dos_interrupt_handler_type previous_critical_error_handler;
static critical_error_callback_type critical_error_callback;

void dos_interrupts_disable(void)
{
	dos_disable_interrupts();
}

void dos_interrupts_enable(void)
{
	dos_enable_interrupts();
}

static void far dos_critical_error_restore(void)
{
	if (previous_critical_error_handler != 0)
		dos_setvect(0x24, previous_critical_error_handler);
}

#pragma argsused
#if defined(__WATCOMC__)
static void interrupt dos_critical_error_handler(union INTPACK registers)
#else
static void interrupt dos_critical_error_handler(legacy_u16 bp,
	legacy_u16 di, legacy_u16 si, legacy_u16 ds, legacy_u16 es,
	legacy_u16 dx, legacy_u16 cx, legacy_u16 bx, legacy_u16 ax,
	legacy_u16 ip, legacy_u16 cs, legacy_u16 flags)
#endif
{
#if defined(__WATCOMC__)
	registers.w.ax = (legacy_u16)critical_error_callback();
#else
	ax = (legacy_u16)critical_error_callback();
#endif
}

void dos_set_critical_error_handler(critical_error_callback_type callback)
{
	add_exit_handler(dos_critical_error_restore);
	critical_error_callback = callback;
	previous_critical_error_handler = dos_getvect(0x24);
	dos_setvect(0x24, dos_critical_error_handler);
}
