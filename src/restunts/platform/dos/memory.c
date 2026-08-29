#include <dos.h>

#include "memmgr.h"
#include "platform.h"

typedef void (far* exit_handler_type)(void);

extern void add_exit_handler(exit_handler_type exit_handler);

void far* dos_memory_get_psp(void)
{
	legacy_u16 segment;
	legacy_u16 offset;

	__asm {
		push ds
		mov ah, 62h
		int 21h
		mov segment, ds
		mov offset, bx
		pop ds
	}
	return MK_FP(segment, offset);
}

legacy_u16 dos_memory_allocate(legacy_u16 paragraphs)
{
	legacy_u16 segment;

	__asm {
		mov bx, paragraphs
		mov ah, 48h
		int 21h
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
		mov ah, 4ah
		int 21h
		mov maximum, bx
	}
	return maximum;
}

static void dos_memory_free(legacy_u16 segment)
{
	__asm {
		mov es, segment
		mov ah, 49h
		int 21h
	}
}

static legacy_u16 ems_handle;
static legacy_u8 ems_present;

void ems_shutdown(void)
{
	legacy_u16 handle;

	if (ems_present == 0)
		return;
	ems_present = 0;
	handle = ems_handle;
	ems_handle = 0;
	__asm {
		mov ah, 45h
		mov dx, handle
		int 67h
	}
}

static void ems_init(void)
{
	legacy_u16 vector_segment;
	legacy_u16 vector_offset;
	legacy_u16 frame_segment;
	legacy_u16 handle;
	legacy_u16 status;
	legacy_u8 page_number;
	legacy_s16 index;
	legacy_s8 far* device_name;
	static const legacy_s8 emm_name[8] = {
		'E', 'M', 'M', 'X', 'X', 'X', 'X', '0'
	};

	ems_present = 0;

	__asm {
		push es
		mov ax, 3567h
		int 21h
		mov vector_segment, es
		mov vector_offset, bx
		pop es
	}
	if (vector_segment == 0 && vector_offset == 0)
		return;

	/* The int 67h vector points into the EMM device driver; its header
	 * carries the device name at offset 0Ah. */
	device_name = MK_FP(vector_segment, 0x0A);
	for (index = 0; index < 8; ++index) {
		if (device_name[index] != emm_name[index])
			return;
	}

	__asm {
		mov ah, 40h
		int 67h
		mov status, ax
	}
	if (status & 0xFF00U)
		return;

	__asm {
		mov ah, 41h
		int 67h
		mov status, ax
		mov frame_segment, bx
	}
	if (status & 0xFF00U)
		return;

	__asm {
		mov ah, 43h
		mov bx, 4
		int 67h
		mov status, ax
		mov handle, dx
	}
	if (status & 0xFF00U)
		return;

	for (index = 0; index < 4; ++index) {
		page_number = (legacy_u8)index;
		__asm {
			mov ah, 44h
			mov al, page_number
			mov bl, page_number
			xor bh, bh
			mov dx, handle
			int 67h
			mov status, ax
		}
		if (status & 0xFF00U) {
			__asm {
				mov ah, 45h
				mov dx, handle
				int 67h
			}
			return;
		}
	}

	ems_handle = handle;
	ems_present = 1;
	add_exit_handler(ems_shutdown);
	highpool_add_block(frame_segment, 0x1000U, 0);
}

static void umb_init(void)
{
	legacy_u16 old_strategy;
	legacy_u16 old_link;
	legacy_u16 umb_size;
	legacy_u16 umb_segment;

	__asm {
		mov ax, 5800h
		int 21h
		mov old_strategy, ax
	}
	__asm {
		mov ax, 5802h
		int 21h
		xor ah, ah
		mov old_link, ax
	}
	__asm {
		mov ax, 5803h
		mov bx, 1
		int 21h
	}
	__asm {
		mov ax, 5801h
		mov bx, 40h
		int 21h
	}

	/* Probe with an impossible size so DOS reports the largest UMB. */
	umb_size = 0;
	__asm {
		mov ah, 48h
		mov bx, 0FFFFh
		int 21h
		mov umb_size, bx
	}

	umb_segment = 0;
	if (umb_size >= 0x100U)
		umb_segment = dos_memory_allocate(umb_size);

	__asm {
		mov ax, 5801h
		mov bx, old_strategy
		int 21h
	}
	__asm {
		mov ax, 5803h
		mov bx, old_link
		int 21h
	}

	if (umb_segment >= 0xA000U && umb_segment < 0xF000U &&
		(legacy_u32)umb_segment + umb_size <= 0xF000UL) {
		highpool_add_block(umb_segment, umb_size, 0);
	} else if (umb_segment > 0x10U) {
		dos_memory_free(umb_segment);
	}
}

void himem_init(void)
{
	ems_init();
	umb_init();
	highpool_reserve_window();
}
