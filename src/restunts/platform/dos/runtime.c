#include "../../c/legacy.h"

#define DOS_RUNTIME_INTERRUPT 33
#define DOS_RUNTIME_WRITE_FUNCTION 64
#define DOS_RUNTIME_EXIT_FUNCTION 76
#define DOS_RUNTIME_STDOUT_HANDLE 1U
#define DOS_RUNTIME_STDERR_HANDLE 2U

/* Borland's DOS interrupt wrapper records failures here.  The original
 * executable obtained this word from its C startup module; the assembly-free
 * target supplies the same runtime storage explicitly. */
legacy_s16 _errno;

static legacy_s16 dos_write_handle(legacy_u16 handle,
	const legacy_s8* text, legacy_u16 length)
{
	legacy_s16 result;

	__asm {
		push    ds
		mov     ah, DOS_RUNTIME_WRITE_FUNCTION
		mov     bx, handle
		mov     cx, length
		mov     dx, text
		int     DOS_RUNTIME_INTERRUPT
		pop     ds
		jnc     write_finished
		mov     ax, -1
	write_finished:
		mov     result, ax
	}

	return result;
}

legacy_s16 dos_write_stdout(const legacy_s8* text, legacy_u16 length)
{
	return dos_write_handle(DOS_RUNTIME_STDOUT_HANDLE, text, length);
}

legacy_s16 dos_write_stderr(const legacy_s8* text, legacy_u16 length)
{
	return dos_write_handle(DOS_RUNTIME_STDERR_HANDLE, text, length);
}

void dos_process_exit(legacy_s16 status)
{
	__asm {
		mov     ax, status
		mov     ah, DOS_RUNTIME_EXIT_FUNCTION
		int     DOS_RUNTIME_INTERRUPT
	}

	for (;;) {
	}
}

legacy_s16 dos_data_stack_segments_match(void)
{
	legacy_s16 result;

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
		mov     result, ax
	}

	return result;
}
