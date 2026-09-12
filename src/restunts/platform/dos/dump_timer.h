#ifndef RESTUNTS_DUMP_TIMER_H
#define RESTUNTS_DUMP_TIMER_H

/* Original offline dump engines advance by replay frames without timer ticks.
 * Mask IRQ0 after init_main so callbacks cannot mutate resource data read by
 * legacy memory aliases, or leave interrupt stack residue read by simulation.
 * Preserve other IRQ masks and the CPU's IF. The registered exit handlers
 * restore the timer before exit. Keep this operation inline to avoid extra
 * caller frames or static data. */
#if defined(__WATCOMC__)
static void dump_disable_timer_irq(void);
#pragma aux dump_disable_timer_irq = "in al,21h"                                                   \
									 "or al,1"                                                     \
									 "out 21h,al" modify exact[al];
#elif defined(__BORLANDC__) || defined(__TURBOC__)
#define dump_disable_timer_irq()                                                                   \
	do {                                                                                           \
		__asm { in al, 21h }                                                                          \
		__asm                                                                                      \
		{                                                                                          \
			or al, 1                                                                               \
		}                                                                                          \
		__asm { out 21h, al }                                                                         \
	} while (0)
#else
#error Original offline dump capture requires a supported DOS compiler
#endif

#endif
