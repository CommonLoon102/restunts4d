#ifdef RESTUNTS_DOS
#include <dos.h>
#include "keyboard.h"

// need these since we are referncing external symbols without an underscore
#define getvect _getvect
#define setvect _setvect
#define int86 _int86
extern void _CType _setvect( int __interruptno, void interrupt( far *__isr )( ) );
extern void interrupt( far * _CType _getvect( int __interruptno ))( );
int _Cdecl _int86( int __intno, union REGS _FAR *__inregs, union REGS _FAR *__outregs );

typedef void interrupt (far* voidinterruptfunctype)();
typedef void (far* voidfunctype)();

voidinterruptfunctype old_kb_int9_handler;
voidinterruptfunctype old_kb_int16_handler;

extern void add_exit_handler(voidfunctype exitfunc);
extern void interrupt kb_int16_handler(); // our asm proxy, simply redirects to kb_int16_handler_c

// these data are local to keyboard.c, but kept in their original slots for convencience:
extern unsigned char kbinput[];
extern unsigned int kb_intr_data;
extern unsigned int kb_intr_data2;
extern unsigned int kb_intr_data3;
extern unsigned int kb_intr_data4;
extern unsigned int kb_intr_data_array[];
extern unsigned char keymap1[];
extern unsigned char keymap2[];
extern unsigned char keymap3[];
extern unsigned char keymap4[];
extern unsigned char keymap5[];
extern unsigned int kblastinput;

void interrupt kb_int9_handler(void) {
	unsigned char kbc, kbp;
	unsigned int kbval, kbdata;

	kbc = inp(0x60);
	kbp = inp(0x61);
	outp(0x61, kbp | 0x80);
	outp(0x61, kbp);

	if ((kbc & 0x80) == 0) {
		if (kbc >= 0x5a) 
			kbc = 0;
		kblastinput = kbc;
		kbinput[kbc] = 1;
		
		if (kbinput[0x38] == 1) {
			kbval = keymap5[kbc];
		} else
		if (kbinput[0x1D] == 1) {
			kbval = keymap4[kbc];
		} else
		if (kbinput[0x2A] == 1) {
			kbval = keymap2[kbc];
		} else
		if (kbinput[0x36] == 1) {
			kbval = keymap2[kbc];
		} else
		if (kbinput[0x3A] == 1) {
			kbval = keymap3[kbc];
		} else {
			kbval = keymap1[kbc];
		}
		
		if ((kbval & 0x80) != 0) {
			if (kbval >= 0x85)
				kbval &= 0x7F;
			kbval <<= 8;
		}

		kbdata = kb_intr_data;
		disable();
		kb_intr_data_array[kbdata / 2] = kbval;
		kbdata+=2;
		if (kbdata >= kb_intr_data3) // data3 = kb_buffer_pos
			kbdata = 0;
		kb_intr_data = kbdata;
		
		kbdata = kb_intr_data4;
		kbdata+=2;
		if (kbdata > kb_intr_data3) {
			kbdata = kb_intr_data3;
			kb_intr_data2 = kb_intr_data;
		}
		kb_intr_data4 = kbdata;
		enable();
		
	} else {
		kbc &= 0x7F;
		if (kbc >= 0x5a) // 0x5a = 90, keymaps are 90 bytes?
			kbc = 0;
		kbinput[kbc] = 0;
	}
	
	outp(0x20, 0x20);
	
}

// FLAGS bits the INT 16h handler hands back to its caller. The original ends
// every arm with `sti` and `retf 2`, which discards the flags the INT pushed
// and leaves the handler's own IF and ZF in place; AH=01 answers "no key
// available" in ZF and nothing else. A Borland `interrupt` function ends in
// iret, which restores the caller's flags, so each exit has to write the
// `flags` pseudo-parameter itself. Live ASM caller: kb_get_char at
// asm/seg012.asm:3990 does `mov ah,1 / int 16h / jnz`.
#define FLAG_ZF 0x0040
#define FLAG_IF 0x0200

#pragma argsused   
void interrupt kb_int16_handler(unsigned bp, unsigned di, unsigned si,
                                     unsigned ds, unsigned es, unsigned dx,
                                     unsigned cx, unsigned bx, unsigned ax,
                                     unsigned ip, unsigned cs, unsigned flags) {

	unsigned int result, kbdata;
	unsigned char bioscall = ax >> 8;
	disable();
	if (bioscall == 0) {
		if (kb_intr_data4 == 0) {
			enable();
			ax = 0;
			flags = (flags | FLAG_IF) | FLAG_ZF;    // xor ax, ax
			return ;
		}
		kbdata = kb_intr_data2;
		result = kb_intr_data_array[kbdata / 2];
		kbdata+=2;
		if (kbdata >= kb_intr_data3)
			kbdata = 0;
		kb_intr_data2 = kbdata;
		kb_intr_data4 = kb_intr_data4 - 2;
		enable();
		ax = result;
		// last flag-setting instruction on this path is `sub bx, 2`
		flags = (flags | FLAG_IF) & ~FLAG_ZF;
		if (kb_intr_data4 == 0)
			flags |= FLAG_ZF;
		return ;
	}
	
	if (bioscall == 1) {
		if (kb_intr_data4 == 0) {
			enable();
			ax = 0;
			flags = (flags | FLAG_IF) | FLAG_ZF;    // no key pending
			return ;
		}
		result = kb_intr_data_array[kb_intr_data2 / 2];
		enable();
		ax = result;
		// ZF still carries `cmp kb_intr_data4, 0` finding a non-zero count
		flags = (flags | FLAG_IF) & ~FLAG_ZF;
		return ;
	}
	
	if (bioscall == 2) {
		result = kbinput[0x2A] | kbinput[0x36];
		enable();
		ax = result & 0xFF;
		// ZF comes from `mov al, kbinput+2Ah / or al, kbinput+36h`
		flags = (flags | FLAG_IF) & ~FLAG_ZF;
		if ((result & 0xFF) == 0)
			flags |= FLAG_ZF;
		return ;
	}
	enable();
	ax = 0;
	flags = (flags | FLAG_IF) | FLAG_ZF;            // xor ax, ax
	//return 0;
}

void kb_init_interrupt(void) {
	unsigned char irqmask;
	int i;

	irqmask = inp(0x21);
	outp(0x21, irqmask | 0x3);

	// `cmp bx, offset kb_int9_handler / jz short loc_30861` - if the vector
	// is already ours the whole install is skipped, so a second call cannot
	// overwrite the saved BIOS vectors with our own handlers.
	if (getvect(9) != (voidinterruptfunctype)kb_int9_handler) {
		old_kb_int9_handler = getvect(9);
		setvect(9, kb_int9_handler);

		old_kb_int16_handler = getvect(0x16);
		setvect(0x16, kb_int16_handler);
	}

	outp(0x21, irqmask);

	// `mov di, offset kbinput / mov cx, 5Ah / xor ax, ax / cld / rep stosb`
	for (i = 0; i < 0x5A; i++) {
		kbinput[i] = 0;
	}

	add_exit_handler(kb_exit_handler);
}

void kb_exit_handler(void) {
	unsigned char irqmask;

	irqmask = inp(0x21);
	outp(0x21, irqmask | 0x3);
	setvect(9, old_kb_int9_handler);
	setvect(0x16, old_kb_int16_handler);
	pokeb(0, 0x417, peekb(0, 0x417) & 0xf0);
	outp(0x21, irqmask);
}

int kb_get_key_state(int key) {
	return kbinput[key];
}

int kb_call_readchar_callback(void) {
	// the orginal code uses a (hard-coded, non-changing) callback for
	// reading chars.. we just call kb_read_char() directly:
	return kb_read_char();
}

int kb_read_char(void) {
	// we could've called kb_int16_handler_c() directly
	union REGS inregs;
	union REGS outregs;
	
	inregs.h.ah = 1;
	int86(0x16, &inregs, &outregs);
	if (!outregs.x.ax) return 0;

	inregs.h.ah = 0;
	int86(0x16, &inregs, &outregs);
	// AL == 0 marks an extended key, whose scancode kb_int9_handler put in AH.
	// The original keeps the whole word in that case (or al,al / jz, skipping
	// the xor ah,ah) and returns AL alone otherwise.
	if (outregs.h.al == 0)
		return outregs.x.ax;
	return outregs.h.al;
}

int kb_checking(void) {
	union REGS inregs;
	union REGS outregs;

	inregs.h.ah = 1;
	int86(0x16, &inregs, &outregs);
	if (outregs.h.al == 0)
		return outregs.x.ax;
	return outregs.h.al;
}

void flush_stdin(void) {
	int result;
	do {
		result = kb_call_readchar_callback();
	} while (result == 0);
}

int kb_check(void) {
	union REGS inregs;
	union REGS outregs;
	
	while (1) {
		inregs.h.ah = 1;
		int86(0x16, &inregs, &outregs);
		if (!outregs.x.ax) return 0;
	
		inregs.h.ah = 0;
		int86(0x16, &inregs, &outregs);
	}
}

#endif
