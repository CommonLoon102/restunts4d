#ifndef RESTUNTS_DOSCOMPAT_H
#define RESTUNTS_DOSCOMPAT_H

#include "../../c/legacy.h"

#include <dos.h>

#if defined(__WATCOMC__)
#include <conio.h>
#endif

/* Interrupt handlers receive a compiler-defined saved-register frame.  Keep
 * the parameter list unspecified so handlers with either compiler's native
 * frame signature can be installed through the same vector API. */
typedef void interrupt (far* dos_interrupt_handler_type)();

#if defined(__BORLANDC__)
extern void interrupt (far* _CType _getvect(
	legacy_s16 interrupt_number))();
extern void _CType _setvect(legacy_s16 interrupt_number,
	dos_interrupt_handler_type handler);
extern legacy_s16 _Cdecl _int86(legacy_s16 interrupt_number,
	union REGS far* input, union REGS far* output);

#define dos_getvect _getvect
#define dos_setvect _setvect
#define dos_int86 _int86
#define dos_disable_interrupts disable
#define dos_enable_interrupts enable
#define DOS_FILE_ATTRIBUTE_NORMAL FA_NORMAL
#define DOS_FILE_ATTRIBUTE_HIDDEN FA_HIDDEN
#define DOS_FILE_ATTRIBUTE_SYSTEM FA_SYSTEM
#define dos_inport_word inport
#else
#define dos_getvect _dos_getvect
#define dos_setvect _dos_setvect
#define dos_int86 int86
#define dos_disable_interrupts _disable
#define dos_enable_interrupts _enable
#define DOS_FILE_ATTRIBUTE_NORMAL _A_NORMAL
#define DOS_FILE_ATTRIBUTE_HIDDEN _A_HIDDEN
#define DOS_FILE_ATTRIBUTE_SYSTEM _A_SYSTEM
#define dos_inport_word inpw
#endif

#define dos_peek_byte(segment, offset) \
	(*(const legacy_u8 far*)MK_FP((segment), (offset)))
#define dos_poke_byte(segment, offset, value) \
	(*(legacy_u8 far*)MK_FP((segment), (offset)) = (legacy_u8)(value))
#define dos_peek_word(segment, offset) \
	(*(const legacy_u16 far*)MK_FP((segment), (offset)))
#define dos_poke_word(segment, offset, value) \
	(*(legacy_u16 far*)MK_FP((segment), (offset)) = (legacy_u16)(value))

#endif
