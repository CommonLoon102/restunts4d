#include "../../c/legacy.h"

extern legacy_u32 timer_callback_counter;
extern legacy_u32 last_timer_callback_counter;

legacy_u32 timer_get_counter(void)
{
	/* The DOS timer interrupt can update either half between ordinary C
	 * loads.  Keep interrupts disabled for the paired 16-bit read. */
	__asm {
		cli
		mov     ax, word ptr timer_callback_counter
		mov     dx, word ptr timer_callback_counter+2
		sti
	}
}

legacy_u32 timer_get_delta(void)
{
	/* Read and update the 32-bit counters as the original 8086 routine did;
	 * Borland otherwise emits two independently interruptible word loads. */
	__asm {
		mov     bx, word ptr last_timer_callback_counter
		mov     cx, word ptr last_timer_callback_counter+2
		cli
		mov     ax, word ptr timer_callback_counter
		mov     dx, word ptr timer_callback_counter+2
		sti
		mov     word ptr last_timer_callback_counter, ax
		mov     word ptr last_timer_callback_counter+2, dx
		sub     ax, bx
		sbb     dx, cx
	}
}
