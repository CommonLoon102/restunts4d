#include "../../c/legacy.h"

/* Borland's DOS interrupt wrapper records failures here.  The original
 * executable obtained this word from its C startup module; the assembly-free
 * target supplies the same runtime storage explicitly. */
legacy_s16 _errno;

legacy_s16 dos_write_stderr(const legacy_s8* text, legacy_u16 length)
{
	legacy_s16 result;

	__asm {
		push    ds
		mov     ah, 40h
		mov     bx, 2
		mov     cx, length
		mov     dx, text
		int     21h
		pop     ds
		jnc     write_finished
		mov     ax, -1
	write_finished:
		mov     result, ax
	}

	return result;
}

void dos_process_exit(legacy_s16 status)
{
	__asm {
		mov     ax, status
		mov     ah, 4Ch
		int     21h
	}

	for (;;) {
	}
}

legacy_s16 dos_data_stack_segments_match(void)
{
	/* Interrupt callbacks can arrive while foreign code owns SS.  Game code
	 * may only touch near data when Borland's medium-model DS and SS agree. */
	__asm {
		xor     ax, ax
		mov     bx, ss
		mov     dx, ds
		cmp     bx, dx
		jne     segments_differ
		inc     ax
	segments_differ:
	}
}
