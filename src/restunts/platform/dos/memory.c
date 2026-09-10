#include "platform.h"

#define DOS_MEMORY_INTERRUPT 33
#define DOS_MEMORY_GET_PSP_FUNCTION 98
#define DOS_MEMORY_ALLOCATE_FUNCTION 72
#define DOS_MEMORY_RESIZE_FUNCTION 74

void far *dos_memory_get_psp(void)
{
	legacy_u16 memory_segment;
	legacy_u16 memory_offset;

	__asm {
		push ds
		mov ah, DOS_MEMORY_GET_PSP_FUNCTION
		int DOS_MEMORY_INTERRUPT
		mov memory_segment, ds
		mov memory_offset, bx
		pop ds
	}
	return dos_memory_make_pointer(memory_segment, memory_offset);
}

legacy_u16 dos_memory_allocate(legacy_u16 paragraphs)
{
	legacy_u16 memory_segment;

	__asm {
		mov bx, paragraphs
		mov ah, DOS_MEMORY_ALLOCATE_FUNCTION
		int DOS_MEMORY_INTERRUPT
		mov memory_segment, ax
	}
	return memory_segment;
}

legacy_u16 dos_memory_resize(legacy_u16 memory_segment, legacy_u16 paragraphs)
{
	legacy_u16 maximum;

	__asm {
		mov bx, paragraphs
		mov es, memory_segment
		mov ah, DOS_MEMORY_RESIZE_FUNCTION
		int DOS_MEMORY_INTERRUPT
		mov maximum, bx
	}
	return maximum;
}
