#include "keyboard.h"
#include "doscompat.h"

typedef dos_interrupt_handler_type voidinterruptfunctype;
typedef void (far* voidfunctype)();

static voidinterruptfunctype old_kb_int9_handler;
static voidinterruptfunctype old_kb_int16_handler;

extern void add_exit_handler(voidfunctype exitfunc);
extern legacy_s16 kb_parse_key(legacy_s16 key);
extern legacy_s16 dos_data_stack_segments_match(void);

static legacy_u8 dos_kb_input[90];

static legacy_u16 dos_kb_buffer_write;
static legacy_u16 dos_kb_buffer_read;
static legacy_u16 dos_kb_buffer_size = 2U;
static legacy_u16 dos_kb_buffer_count;
static legacy_u16 dos_kb_buffer[64];
static legacy_u16 dos_kb_last_input;

static legacy_u16 dos_kb_interrupt(union REGS* input, union REGS* output)
{
#if defined(__WATCOMC__)
	union REGPACK registers;
	struct SREGS segments;

	segread(&segments);
	registers.w.ax = input->x.ax;
	registers.w.bx = input->x.bx;
	registers.w.cx = input->x.cx;
	registers.w.dx = input->x.dx;
	registers.w.bp = 0;
	registers.w.si = input->x.si;
	registers.w.di = input->x.di;
	registers.w.ds = segments.ds;
	registers.w.es = segments.es;
	registers.w.flags = 0;
	intr(0x16, &registers);
	output->x.ax = registers.w.ax;
	output->x.bx = registers.w.bx;
	output->x.cx = registers.w.cx;
	output->x.dx = registers.w.dx;
	output->x.si = registers.w.si;
	output->x.di = registers.w.di;
	output->x.cflag = registers.w.flags & 1U;
	return (legacy_u16)registers.w.flags;
#else
	dos_int86(0x16, input, output);
	return output->x.flags;
#endif
}

static const legacy_u8 dos_kb_keymap1[91] = {
	0, 27, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, 45, 61, 8, 9,
	113, 119, 101, 114, 116, 121, 117, 105, 111, 112, 91, 93, 13, 0,
	97, 115, 100, 102, 103, 104, 106, 107, 108, 59, 39, 96, 0, 92,
	122, 120, 99, 118, 98, 110, 109, 44, 46, 47, 0, 42, 0, 32, 0,
	187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 0, 0, 199, 200,
	201, 45, 203, 204, 205, 43, 207, 208, 209, 210, 211, 0, 0, 0, 0,
	0, 0, 0
};

static const legacy_u8 dos_kb_keymap2[91] = {
	0, 27, 33, 64, 35, 36, 37, 94, 38, 42, 40, 41, 95, 43, 8, 143,
	81, 87, 69, 82, 84, 89, 85, 73, 79, 80, 123, 125, 13, 0, 65, 83,
	68, 70, 71, 72, 74, 75, 76, 58, 34, 126, 0, 124, 90, 88, 67, 86,
	66, 78, 77, 60, 62, 63, 0, 0, 0, 32, 0, 212, 213, 214, 215, 216,
	217, 218, 219, 220, 221, 0, 0, 199, 200, 201, 45, 203, 204, 205,
	43, 207, 208, 209, 210, 211, 0, 0, 0, 0, 0, 0, 0
};

static const legacy_u8 dos_kb_keymap3[91] = {
	0, 27, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, 45, 61, 8, 143,
	81, 87, 69, 82, 84, 89, 85, 73, 79, 80, 91, 93, 13, 0, 65, 83,
	68, 70, 71, 72, 74, 75, 76, 59, 39, 96, 0, 92, 90, 88, 67, 86,
	66, 78, 77, 44, 46, 47, 0, 0, 0, 32, 0, 212, 213, 214, 215, 216,
	217, 218, 219, 220, 221, 0, 0, 199, 200, 201, 45, 203, 204, 205,
	43, 207, 208, 209, 210, 211, 0, 0, 0, 0, 0, 0, 0
};

static const legacy_u8 dos_kb_keymap4[91] = {
	0, 27, 33, 0, 35, 36, 37, 30, 38, 42, 40, 41, 31, 43, 127, 9,
	17, 23, 5, 18, 20, 25, 21, 9, 15, 16, 27, 29, 13, 0, 1, 19,
	4, 6, 7, 8, 10, 11, 12, 59, 44, 96, 0, 28, 26, 24, 3, 22,
	2, 14, 178, 60, 62, 63, 0, 0, 0, 32, 0, 222, 223, 224, 225, 226,
	227, 228, 229, 230, 231, 0, 0, 199, 200, 201, 45, 203, 204, 205,
	43, 207, 208, 209, 210, 211, 0, 0, 0, 0, 0, 0, 0
};

static const legacy_u8 dos_kb_keymap5[92] = {
	0, 27, 33, 64, 35, 36, 37, 94, 38, 42, 40, 41, 95, 43, 8, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 123, 125, 13, 0,
	158, 159, 160, 161, 162, 163, 164, 165, 166, 58, 34, 126, 0, 124,
	172, 173, 174, 175, 176, 177, 178, 60, 62, 63, 0, 0, 0, 32, 0,
	248, 249, 250, 251, 252, 253, 254, 255, 128, 129, 0, 0, 199, 200,
	201, 45, 203, 204, 205, 43, 207, 208, 209, 210, 211, 0, 0, 0, 0,
	0, 0, 0, 0
};

// The original opens with `sti` before it touches anything, so the rest of
// the handler runs with interrupts on and only the buffer update is fenced by
// cli/sti. A Borland `interrupt` function has no way to say that in its
// prologue, so this one stays inside the IF=0 the gate left. Latency only -
// no code here depends on being re-entered.
void interrupt kb_int9_handler(void) {
	legacy_u8 kbc, kbp;
	legacy_u16 kbval, kbdata;

	kbc = inp(0x60);
	kbp = inp(0x61);
	outp(0x61, kbp | 0x80);
	outp(0x61, kbp);

	if ((kbc & 0x80) == 0) {
		if (kbc >= 0x5a)
			kbc = 0;
		dos_kb_last_input = kbc;
		dos_kb_input[kbc] = 1;

		if (dos_kb_input[0x38] == 1) {
			kbval = dos_kb_keymap5[kbc];
		} else
		if (dos_kb_input[0x1D] == 1) {
			kbval = dos_kb_keymap4[kbc];
		} else
		if (dos_kb_input[0x2A] == 1) {
			kbval = dos_kb_keymap2[kbc];
		} else
		if (dos_kb_input[0x36] == 1) {
			kbval = dos_kb_keymap2[kbc];
		} else
		if (dos_kb_input[0x3A] == 1) {
			kbval = dos_kb_keymap3[kbc];
		} else {
			kbval = dos_kb_keymap1[kbc];
		}

		if ((kbval & 0x80) != 0) {
			if (kbval >= 0x85)
				kbval &= 0x7F;
			kbval <<= 8;
		}

		kbdata = dos_kb_buffer_write;
		dos_disable_interrupts();
		dos_kb_buffer[kbdata / 2] = kbval;
		kbdata+=2;
		if (kbdata >= dos_kb_buffer_size) // data3 = kb_buffer_pos
			kbdata = 0;
		dos_kb_buffer_write = kbdata;

		kbdata = dos_kb_buffer_count;
		kbdata+=2;
		if (kbdata > dos_kb_buffer_size) {
			kbdata = dos_kb_buffer_size;
			dos_kb_buffer_read = dos_kb_buffer_write;
		}
		dos_kb_buffer_count = kbdata;
		dos_enable_interrupts();

	} else {
		kbc &= 0x7F;
		if (kbc >= 0x5a) // 0x5a = 90, keymaps are 90 bytes?
			kbc = 0;
		dos_kb_input[kbc] = 0;
	}

	outp(0x20, 0x20);

}

// The original returns with the status flags left by its final XOR, SUB, CMP
// or OR, plus IF set by STI. Capture those operations directly instead of
// merging only ZF into the caller's saved flags; Borland's interrupt epilogue
// will IRET with the value assigned to the `flags` pseudo-parameter.
static legacy_u16 kb_flags_after_zero(void) {
	legacy_u16 result;
	__asm {
		xor ax, ax
		pushf
		pop ax
		mov result, ax
	}
	return result;
}

static legacy_u16 kb_flags_after_subtract_two(legacy_u16 value) {
	legacy_u16 result;
	__asm {
		mov ax, value
		sub ax, 2
		pushf
		pop ax
		mov result, ax
	}
	return result;
}

static legacy_u16 kb_flags_after_compare_zero(legacy_u16 value) {
	legacy_u16 result;
	__asm {
		mov ax, value
		cmp ax, 0
		pushf
		pop ax
		mov result, ax
	}
	return result;
}

static legacy_u16 kb_flags_after_or(legacy_u8 left, legacy_u8 right) {
	legacy_u16 result;
	__asm {
		mov al, left
		or al, right
		pushf
		pop ax
		mov result, ax
	}
	return result;
}

#pragma argsused
#if defined(__WATCOMC__)
static void interrupt kb_int16_handler(union INTPACK registers) {
#define KB_INTERRUPT_AX registers.w.ax
#define KB_INTERRUPT_FLAGS registers.w.flags
#else
void interrupt kb_int16_handler(legacy_u16 bp, legacy_u16 di, legacy_u16 si,
	legacy_u16 ds, legacy_u16 es, legacy_u16 dx,
	legacy_u16 cx, legacy_u16 bx, legacy_u16 ax,
	legacy_u16 ip, legacy_u16 cs, legacy_u16 flags) {
#define KB_INTERRUPT_AX ax
#define KB_INTERRUPT_FLAGS flags
#endif

	legacy_u16 result, kbdata;
	legacy_u8 shiftleft, shiftright;
	legacy_u8 bioscall = KB_INTERRUPT_AX >> 8;
	dos_disable_interrupts();
	if (bioscall == 0) {
		kbdata = dos_kb_buffer_count;
		if (kbdata == 0) {
			dos_enable_interrupts();
			KB_INTERRUPT_AX = 0;
			KB_INTERRUPT_FLAGS = kb_flags_after_zero();
			return ;
		}
		kbdata = dos_kb_buffer_read;
		result = dos_kb_buffer[kbdata / 2];
		kbdata+=2;
		if (kbdata >= dos_kb_buffer_size)
			kbdata = 0;
		dos_kb_buffer_read = kbdata;
		kbdata = dos_kb_buffer_count;
		dos_kb_buffer_count = kbdata - 2;
		dos_enable_interrupts();
		KB_INTERRUPT_AX = result;
		KB_INTERRUPT_FLAGS = kb_flags_after_subtract_two(kbdata);
		return ;
	}

	if (bioscall == 1) {
		kbdata = dos_kb_buffer_count;
		if (kbdata == 0) {
			dos_enable_interrupts();
			KB_INTERRUPT_AX = 0;
			KB_INTERRUPT_FLAGS = kb_flags_after_zero();
			return ;
		}
		result = dos_kb_buffer[dos_kb_buffer_read / 2];
		dos_enable_interrupts();
		KB_INTERRUPT_AX = result;
		KB_INTERRUPT_FLAGS = kb_flags_after_compare_zero(kbdata);
		return ;
	}

	if (bioscall == 2) {
		shiftleft = dos_kb_input[0x2A];
		shiftright = dos_kb_input[0x36];
		result = shiftleft | shiftright;
		dos_enable_interrupts();
		KB_INTERRUPT_AX = result & 0xFF;
		KB_INTERRUPT_FLAGS = kb_flags_after_or(shiftleft, shiftright);
		return ;
	}
	dos_enable_interrupts();
	KB_INTERRUPT_AX = 0;
	KB_INTERRUPT_FLAGS = kb_flags_after_zero();
	//return 0;
}
#undef KB_INTERRUPT_AX
#undef KB_INTERRUPT_FLAGS

void kb_init_interrupt(void) {
	legacy_u8 irqmask;
	legacy_s16 i;
	voidinterruptfunctype current_kb_int9_handler;

	irqmask = inp(0x21);
	outp(0x21, irqmask | 0x3);

	// The original compares only the offset word read from vector 9.
	current_kb_int9_handler = dos_getvect(9);
	if (FP_OFF(current_kb_int9_handler) != FP_OFF(kb_int9_handler)) {
		old_kb_int9_handler = current_kb_int9_handler;
		dos_setvect(9, kb_int9_handler);

		old_kb_int16_handler = dos_getvect(0x16);
		dos_setvect(0x16, kb_int16_handler);
	}

	outp(0x21, irqmask);

	// `mov di, offset dos_kb_input / mov cx, 5Ah / xor ax, ax / cld / rep stosb`
	for (i = 0; i < 0x5A; i++) {
		dos_kb_input[i] = 0;
	}

	add_exit_handler(kb_exit_handler);
}

void kb_exit_handler(void) {
	legacy_u8 irqmask;

	irqmask = inp(0x21);
	outp(0x21, irqmask | 0x3);

	// The original guards this block with the saved offset word alone.
	if (FP_OFF(old_kb_int9_handler) != 0) {
		dos_setvect(9, old_kb_int9_handler);
		dos_setvect(0x16, old_kb_int16_handler);
		dos_poke_byte(0, 0x417, dos_peek_byte(0, 0x417) & 0xf0);
	}

	outp(0x21, irqmask);
}

legacy_s16 kb_get_key_state(legacy_s16 key) {
	return dos_kb_input[key];
}

legacy_s16 dos_kb_get_char(void)
{
	union REGS inregs;
	union REGS outregs;

	inregs.h.ah = 1;
	if ((dos_kb_interrupt(&inregs, &outregs) & 0x0040U) != 0)
		return 0;

	/* A timer callback may ask while a foreign stack is active.  The original
	 * reports the pending key but postpones consuming and dispatching it. */
	if (dos_data_stack_segments_match() == 0)
		return outregs.x.ax;

	inregs.h.ah = 0;
	(void)dos_kb_interrupt(&inregs, &outregs);
	return kb_parse_key(outregs.x.ax);
}

void dos_kb_set_numlock(void)
{
	dos_poke_byte(0x40U, 0x17U,
		dos_peek_byte(0x40U, 0x17U) | 0x20U);
	(void)kb_checking();
}

void dos_kb_clear_numlock(void)
{
	dos_poke_byte(0x40U, 0x17U,
		dos_peek_byte(0x40U, 0x17U) & 0xDFU);
	(void)kb_checking();
}

legacy_s16 kb_call_readchar_callback(void) {
	// the orginal code uses a (hard-coded, non-changing) callback for
	// reading chars.. we just call kb_read_char() directly:
	return kb_read_char();
}

legacy_s16 kb_read_char(void) {
	// we could've called kb_int16_handler_c() directly
	union REGS inregs;
	union REGS outregs;

	inregs.h.ah = 1;
	(void)dos_kb_interrupt(&inregs, &outregs);
	if (!outregs.x.ax) return 0;

	inregs.h.ah = 0;
	(void)dos_kb_interrupt(&inregs, &outregs);
	// AL == 0 marks an extended key, whose scancode kb_int9_handler put in AH.
	// The original keeps the whole word in that case (or al,al / jz, skipping
	// the xor ah,ah) and returns AL alone otherwise.
	if (outregs.h.al == 0)
		return outregs.x.ax;
	return outregs.h.al;
}

legacy_s16 kb_checking(void) {
	union REGS inregs;
	union REGS outregs;

	inregs.h.ah = 1;
	(void)dos_kb_interrupt(&inregs, &outregs);
	if (outregs.h.al == 0)
		return outregs.x.ax;
	return outregs.h.al;
}

void flush_stdin(void) {
	legacy_s16 result;
	do {
		result = kb_call_readchar_callback();
	} while (result == 0);
}

legacy_s16 kb_check(void) {
	union REGS inregs;
	union REGS outregs;

	while (1) {
		inregs.h.ah = 1;
		(void)dos_kb_interrupt(&inregs, &outregs);
		if (!outregs.x.ax) return 0;

		inregs.h.ah = 0;
		(void)dos_kb_interrupt(&inregs, &outregs);
	}
}
