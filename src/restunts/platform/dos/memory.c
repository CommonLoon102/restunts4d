#include "platform.h"

#define DOS_MEMORY_INTERRUPT 33
#define DOS_MEMORY_GET_PSP_FUNCTION 98
#define DOS_MEMORY_ALLOCATE_FUNCTION 72
#define DOS_MEMORY_RESIZE_FUNCTION 74

void far* dos_memory_get_psp(void)
{
	legacy_u16 segment;
	legacy_u16 offset;

	__asm {
		push ds
		mov ah, DOS_MEMORY_GET_PSP_FUNCTION
		int DOS_MEMORY_INTERRUPT
		mov segment, ds
		mov offset, bx
		pop ds
	}
	return dos_memory_make_pointer(segment, offset);
}

legacy_u16 dos_memory_allocate(legacy_u16 paragraphs)
{
	legacy_u16 segment;

	__asm {
		mov bx, paragraphs
		mov ah, DOS_MEMORY_ALLOCATE_FUNCTION
		int DOS_MEMORY_INTERRUPT
		mov segment, ax
	}
	return segment;
}

legacy_u16 dos_memory_resize(legacy_u16 segment, legacy_u16 paragraphs)
{
	legacy_u16 maximum;

	__asm {
		mov bx, paragraphs
		mov es, segment
		mov ah, DOS_MEMORY_RESIZE_FUNCTION
		int DOS_MEMORY_INTERRUPT
		mov maximum, bx
	}
	return maximum;
}
