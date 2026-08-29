#include "../../c/legacy.h"

#include <dos.h>

#define getvect _getvect
extern void interrupt (far* _CType _getvect(legacy_s16 interrupt_number))(void);

typedef void interrupt (far* interrupt_handler_type)(void);

extern void add_exit_handler(void (far* exit_handler)(void));

legacy_u32 dos_timer_counter;
legacy_s16 dos_timer_callbacks_suspended;
void (far* dos_timer_callbacks[6])(void);

static legacy_u32 dos_timer_last_counter;
static legacy_u16 dos_timer_slow_low;
static legacy_u16 dos_timer_slow_high;
static legacy_u8 dos_timer_chain_timeout_active;
static legacy_u8 dos_timer_chain_enabled;
static legacy_u16 dos_timer_chain_timeout;
static legacy_u16 dos_timer_divider_period;
static legacy_u16 dos_timer_divider;
static legacy_u16 dos_timer_max_reentry;
static legacy_u16 dos_timer_reentry;
static legacy_u8 dos_timer_in_callbacks;
static interrupt_handler_type previous_timer_interrupt;

static void interrupt dos_timer_interrupt(void);

static void dos_timer_chain_previous_handler(void)
{
	__asm {
		pushf
		call dword ptr previous_timer_interrupt
	}
}

static void dos_timer_increment_counter(legacy_u32* counter)
{
	legacy_u16* words;

	words = (legacy_u16*)counter;
	words[0] = (legacy_u16)(words[0] + 1U);
	if (words[0] == 0)
		words[1] = (legacy_u16)(words[1] + 1U);
}

static void interrupt dos_timer_interrupt(void)
{
	legacy_u16 callback_index;
	legacy_s16 reentry_count;

	/* Match the original IRQ0 handler: allow nested interrupts after the
	 * compiler's interrupt prologue has saved the interrupted registers. */
	enable();
	dos_timer_divider = (legacy_u16)(dos_timer_divider - 1U);
	if (LEGACY_S16_FROM_BITS(dos_timer_divider) <= 0) {
		dos_timer_slow_low = (legacy_u16)(dos_timer_slow_low + 1U);
		if (dos_timer_slow_low == 0)
			dos_timer_slow_high = (legacy_u16)(dos_timer_slow_high + 1U);
		dos_timer_divider = dos_timer_divider_period;

		if (dos_timer_chain_enabled != 0) {
			if (dos_timer_chain_timeout_active != 0) {
				dos_timer_chain_timeout = (legacy_u16)(dos_timer_chain_timeout - 1U);
				if (LEGACY_S16_FROM_BITS(dos_timer_chain_timeout) <= 0) {
					dos_timer_chain_timeout_active = 0;
					dos_timer_chain_enabled = 0;
				}
			}
			dos_timer_chain_previous_handler();
		} else {
			outp(0x20U, 0x20U);
		}
	} else {
		outp(0x20U, 0x20U);
	}

	if (((legacy_u16)dos_timer_callbacks_suspended & 0x00FFU) != 0)
		goto callbacks_finished;

	dos_timer_increment_counter(&dos_timer_counter);
	disable();
	if (dos_timer_in_callbacks == 0) {
		dos_timer_in_callbacks = 1U;
		enable();
		for (callback_index = 0; callback_index < 6U;
			callback_index++) {
			if (FP_SEG(dos_timer_callbacks[callback_index]) == 0U)
				break;
			dos_timer_callbacks[callback_index]();
		}
		goto callbacks_finished;
	}

	dos_timer_reentry = (legacy_u16)(dos_timer_reentry + 1U);
	reentry_count = LEGACY_S16_FROM_BITS(dos_timer_reentry);
	if (LEGACY_S16_FROM_BITS(dos_timer_max_reentry) < reentry_count)
		dos_timer_max_reentry = dos_timer_reentry;
	return;

callbacks_finished:
	dos_timer_in_callbacks = 0;
	dos_timer_reentry = 0;
}

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
	if (installed_handler != dos_timer_interrupt)
		return;

	interrupt_mask = (legacy_u8)inp(0x21U);
	outp(0x21U, interrupt_mask | 3U);
	dos_timer_write_vector(previous_timer_interrupt);
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

	dos_timer_divider_period = 5U;
	dos_timer_divider = 5U;
	dos_timer_chain_timeout_active = 0;
	dos_timer_chain_enabled = 1U;

	disable();
	dos_timer_in_callbacks = 0;
	dos_timer_callbacks[0] = 0;
	enable();

	outp(0x61U, inp(0x61U) & 0xFCU);
	outp(0x43U, 0xB6U);
	interrupt_mask = (legacy_u8)inp(0x21U);
	outp(0x21U, interrupt_mask | 3U);

	installed_handler = getvect(8);
	if (installed_handler != dos_timer_interrupt) {
		previous_timer_interrupt = installed_handler;
		dos_timer_write_vector(dos_timer_interrupt);
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
		mov     ax, word ptr dos_timer_counter
		mov     dx, word ptr dos_timer_counter+2
		sti
	}
}

legacy_u32 timer_get_delta(void)
{
	/* Read and update the 32-bit counters as the original 8086 routine did;
	 * Borland otherwise emits two independently interruptible word loads. */
	__asm {
		mov     bx, word ptr dos_timer_last_counter
		mov     cx, word ptr dos_timer_last_counter+2
		cli
		mov     ax, word ptr dos_timer_counter
		mov     dx, word ptr dos_timer_counter+2
		sti
		mov     word ptr dos_timer_last_counter, ax
		mov     word ptr dos_timer_last_counter+2, dx
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
		mov     ax, dos_timer_slow_low
		mov     dx, dos_timer_slow_high
		sti
	}
}
