#include "../../c/legacy.h"

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
