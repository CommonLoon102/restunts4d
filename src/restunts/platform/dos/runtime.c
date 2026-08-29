#include "../../c/legacy.h"

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
