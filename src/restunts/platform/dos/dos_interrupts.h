#ifndef RESTUNTS_DOS_INTERRUPTS_H
#define RESTUNTS_DOS_INTERRUPTS_H

#include <dos.h>
#ifdef __WATCOMC__
#include <conio.h>
#endif
#include "../../c/legacy.h"

typedef void interrupt(far *dos_interrupt_handler_type)();

#ifdef __WATCOMC__
#define _setvect _dos_setvect
#define _getvect _dos_getvect
#define disable _disable
#define enable _enable
#define inport inpw
#define outport outpw
#define peek(segment, offset) (*(legacy_u16 far *)MK_FP(segment, offset))
#define poke(segment, offset, value) (peek(segment, offset) = (legacy_u16)(value))
#define peekb(segment, offset) (*(legacy_u8 far *)MK_FP(segment, offset))
#define pokeb(segment, offset, value) (peekb(segment, offset) = (legacy_u8)(value))

/* INTPACK describes Watcom's saved-register interrupt frame, including the
 * reserved GS/FS slots when generating 8086 instructions. */
#define DOS_INTERRUPT_REGISTERS union INTPACK registers
#define DOS_INTERRUPT_AX registers.w.ax
#define DOS_INTERRUPT_IP registers.w.ip
#define DOS_INTERRUPT_CS registers.w.cs
#define DOS_INTERRUPT_FLAGS registers.w.flags
#else
#define DOS_INTERRUPT_REGISTERS                                                                    \
	legacy_u16 bp, legacy_u16 di, legacy_u16 si, legacy_u16 ds, legacy_u16 es, legacy_u16 dx,      \
		legacy_u16 cx, legacy_u16 bx, legacy_u16 ax, legacy_u16 ip, legacy_u16 cs,                 \
		legacy_u16 flags
#define DOS_INTERRUPT_AX ax
#define DOS_INTERRUPT_IP ip
#define DOS_INTERRUPT_CS cs
#define DOS_INTERRUPT_FLAGS flags

/* Unprefixed C symbols for the Borland runtime's interrupt-vector entry points. */
void _CType _setvect(legacy_s16 interrupt_number, dos_interrupt_handler_type handler);
dos_interrupt_handler_type _CType _getvect(legacy_s16 interrupt_number);

#endif

#endif
