#include "../../c/legacy.h"

#include <dos.h>

#define getvect _getvect
extern void interrupt (far* _CType _getvect(legacy_s16 interrupt_number))(void);

typedef void interrupt (far* interrupt_handler_type)(void);

extern legacy_u32 timer_callback_counter;
extern legacy_u32 last_timer_callback_counter;
extern legacy_u16 word_3F87C;
extern legacy_u16 word_3F87E;
extern legacy_u8 byte_3F880;
extern legacy_u8 byte_3F881;
extern legacy_u16 word_3F884;
extern legacy_u16 word_3F886;
extern legacy_u8 byte_3F88C;
extern void (far* timerintr[6])(void);
extern interrupt_handler_type dword_3F874;
extern void interrupt timer_intr_callback(void);
extern void add_exit_handler(void (far* exit_handler)(void));

static void dos_timer_write_vector(interrupt_handler_type handler)
{
	legacy_u16 handler_offset;
	legacy_u16 handler_segment;

	handler_offset = FP_OFF(handler);
	handler_segment = FP_SEG(handler);
	__asm {
		cli
		push    es
		xor     ax, ax
		mov     es, ax
		mov     ax, handler_offset
		mov     es:[20h], ax
		mov     ax, handler_segment
		mov     es:[22h], ax
		pop     es
		sti
	}
}

void dos_timer_shutdown(void)
{
	interrupt_handler_type installed_handler;
	legacy_u8 interrupt_mask;

	installed_handler = getvect(8);
	if (installed_handler != timer_intr_callback)
		return;

	interrupt_mask = (legacy_u8)inp(0x21U);
	outp(0x21U, interrupt_mask | 3U);
	dos_timer_write_vector(dword_3F874);
	interrupt_mask = (legacy_u8)inp(0x21U);
	outp(0x21U, interrupt_mask & 0xFCU);
	outp(0x40U, 0);
	outp(0x40U, 0);
	outp(0x61U, inp(0x61U) & 0xFCU);
}

void dos_timer_setup_interrupt(void)
{
	interrupt_handler_type installed_handler;
	legacy_u8 interrupt_mask;

	word_3F884 = 5U;
	word_3F886 = 5U;
	byte_3F880 = 0;
	byte_3F881 = 1U;

	disable();
	byte_3F88C = 0;
	timerintr[0] = 0;
	enable();

	outp(0x61U, inp(0x61U) & 0xFCU);
	outp(0x43U, 0xB6U);
	interrupt_mask = (legacy_u8)inp(0x21U);
	outp(0x21U, interrupt_mask | 3U);

	installed_handler = getvect(8);
	if (installed_handler != timer_intr_callback) {
		dword_3F874 = installed_handler;
		dos_timer_write_vector(timer_intr_callback);
	}

	interrupt_mask = (legacy_u8)inp(0x21U);
	outp(0x21U, interrupt_mask & 0xFCU);
	outp(0x40U, 0x9CU);
	outp(0x40U, 0x2EU);
	add_exit_handler(dos_timer_shutdown);
}

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

legacy_u32 timer_get_slow_counter(void)
{
	/* This counter advances when the interrupt divider expires, rather than
	 * on every hardware timer interrupt. */
	__asm {
		cli
		mov     ax, word_3F87C
		mov     dx, word_3F87E
		sti
	}
}
