#include <dos.h>

#include "memmgr.h"
#include "platform.h"

typedef void (far* exit_handler_type)(void);

extern void add_exit_handler(exit_handler_type exit_handler);

#define DOS_MEMORY_INTERRUPT 33
#define DOS_MEMORY_GET_PSP_FUNCTION 98
#define DOS_MEMORY_ALLOCATE_FUNCTION 72
#define DOS_MEMORY_RESIZE_FUNCTION 74
#define DOS_MEMORY_FREE_FUNCTION 73
#define DOS_MEMORY_GET_EMS_VECTOR_FUNCTION 13671
#define DOS_MEMORY_EMS_INTERRUPT 103
#define DOS_MEMORY_EMS_STATUS_FUNCTION 64
#define DOS_MEMORY_EMS_PAGE_FRAME_FUNCTION 65
#define DOS_MEMORY_EMS_ALLOCATE_FUNCTION 67
#define DOS_MEMORY_EMS_MAP_PAGE_FUNCTION 68
#define DOS_MEMORY_EMS_RELEASE_FUNCTION 69
#define DOS_MEMORY_EMS_DEVICE_NAME_SIZE 8
#define DOS_MEMORY_EMS_DEVICE_NAME_OFFSET 10
#define DOS_MEMORY_EMS_STATUS_MASK 65280U
#define DOS_MEMORY_EMS_PAGE_COUNT 4
#define DOS_MEMORY_EMS_FRAME_PARAGRAPHS 4096U
#define DOS_MEMORY_GET_ALLOCATION_STRATEGY_FUNCTION 22528
#define DOS_MEMORY_SET_ALLOCATION_STRATEGY_FUNCTION 22529
#define DOS_MEMORY_GET_UMB_LINK_FUNCTION 22530
#define DOS_MEMORY_SET_UMB_LINK_FUNCTION 22531
#define DOS_MEMORY_UMB_LINK_ENABLED 1
#define DOS_MEMORY_UMB_HIGH_FIRST_STRATEGY 64
#define DOS_MEMORY_MAX_PARAGRAPH_REQUEST 65535
#define DOS_MEMORY_UMB_MIN_PARAGRAPHS 256U
#define DOS_MEMORY_VIDEO_SEGMENT_MIN 40960U
#define DOS_MEMORY_ROM_SEGMENT_MIN 61440U
#define DOS_MEMORY_LOWEST_ALLOCATED_SEGMENT 16U

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

static void dos_memory_free(legacy_u16 segment)
{
	__asm {
		mov es, segment
		mov ah, DOS_MEMORY_FREE_FUNCTION
		int DOS_MEMORY_INTERRUPT
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
		mov ah, DOS_MEMORY_EMS_RELEASE_FUNCTION
		mov dx, handle
		int DOS_MEMORY_EMS_INTERRUPT
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
	static const legacy_s8 emm_name[DOS_MEMORY_EMS_DEVICE_NAME_SIZE] = {
		'E', 'M', 'M', 'X', 'X', 'X', 'X', '0'
	};

	ems_present = 0;

	__asm {
		push es
		mov ax, DOS_MEMORY_GET_EMS_VECTOR_FUNCTION
		int DOS_MEMORY_INTERRUPT
		mov vector_segment, es
		mov vector_offset, bx
		pop es
	}
	if (vector_segment == 0 && vector_offset == 0)
		return;

	/* The EMS vector points into the EMM device driver; its header carries
	 * the device name at byte offset 10. */
	device_name = MK_FP(vector_segment, DOS_MEMORY_EMS_DEVICE_NAME_OFFSET);
	for (index = 0; index < DOS_MEMORY_EMS_DEVICE_NAME_SIZE; ++index) {
		if (device_name[index] != emm_name[index])
			return;
	}

	__asm {
		mov ah, DOS_MEMORY_EMS_STATUS_FUNCTION
		int DOS_MEMORY_EMS_INTERRUPT
		mov status, ax
	}
	if (status & DOS_MEMORY_EMS_STATUS_MASK)
		return;

	__asm {
		mov ah, DOS_MEMORY_EMS_PAGE_FRAME_FUNCTION
		int DOS_MEMORY_EMS_INTERRUPT
		mov status, ax
		mov frame_segment, bx
	}
	if (status & DOS_MEMORY_EMS_STATUS_MASK)
		return;

	__asm {
		mov ah, DOS_MEMORY_EMS_ALLOCATE_FUNCTION
		mov bx, DOS_MEMORY_EMS_PAGE_COUNT
		int DOS_MEMORY_EMS_INTERRUPT
		mov status, ax
		mov handle, dx
	}
	if (status & DOS_MEMORY_EMS_STATUS_MASK)
		return;

	for (index = 0; index < DOS_MEMORY_EMS_PAGE_COUNT; ++index) {
		page_number = (legacy_u8)index;
		__asm {
			mov ah, DOS_MEMORY_EMS_MAP_PAGE_FUNCTION
			mov al, page_number
			mov bl, page_number
			xor bh, bh
			mov dx, handle
			int DOS_MEMORY_EMS_INTERRUPT
			mov status, ax
		}
		if (status & DOS_MEMORY_EMS_STATUS_MASK) {
			__asm {
				mov ah, DOS_MEMORY_EMS_RELEASE_FUNCTION
				mov dx, handle
				int DOS_MEMORY_EMS_INTERRUPT
			}
			return;
		}
	}

	ems_handle = handle;
	ems_present = 1;
	add_exit_handler(ems_shutdown);
	highpool_add_block(frame_segment, DOS_MEMORY_EMS_FRAME_PARAGRAPHS, 0);
}

static void umb_init(void)
{
	legacy_u16 old_strategy;
	legacy_u16 old_link;
	legacy_u16 umb_size;
	legacy_u16 umb_segment;

	__asm {
		mov ax, DOS_MEMORY_GET_ALLOCATION_STRATEGY_FUNCTION
		int DOS_MEMORY_INTERRUPT
		mov old_strategy, ax
	}
	__asm {
		mov ax, DOS_MEMORY_GET_UMB_LINK_FUNCTION
		int DOS_MEMORY_INTERRUPT
		xor ah, ah
		mov old_link, ax
	}
	__asm {
		mov ax, DOS_MEMORY_SET_UMB_LINK_FUNCTION
		mov bx, DOS_MEMORY_UMB_LINK_ENABLED
		int DOS_MEMORY_INTERRUPT
	}
	__asm {
		mov ax, DOS_MEMORY_SET_ALLOCATION_STRATEGY_FUNCTION
		mov bx, DOS_MEMORY_UMB_HIGH_FIRST_STRATEGY
		int DOS_MEMORY_INTERRUPT
	}

	/* Probe with an impossible size so DOS reports the largest UMB. */
	umb_size = 0;
	__asm {
		mov ah, DOS_MEMORY_ALLOCATE_FUNCTION
		mov bx, DOS_MEMORY_MAX_PARAGRAPH_REQUEST
		int DOS_MEMORY_INTERRUPT
		mov umb_size, bx
	}

	umb_segment = 0;
	if (umb_size >= DOS_MEMORY_UMB_MIN_PARAGRAPHS)
		umb_segment = dos_memory_allocate(umb_size);

	__asm {
		mov ax, DOS_MEMORY_SET_ALLOCATION_STRATEGY_FUNCTION
		mov bx, old_strategy
		int DOS_MEMORY_INTERRUPT
	}
	__asm {
		mov ax, DOS_MEMORY_SET_UMB_LINK_FUNCTION
		mov bx, old_link
		int DOS_MEMORY_INTERRUPT
	}

	if (umb_segment >= DOS_MEMORY_VIDEO_SEGMENT_MIN &&
		umb_segment < DOS_MEMORY_ROM_SEGMENT_MIN &&
		(legacy_u32)umb_segment + umb_size <=
		DOS_MEMORY_ROM_SEGMENT_MIN) {
		highpool_add_block(umb_segment, umb_size, 0);
	} else if (umb_segment > DOS_MEMORY_LOWEST_ALLOCATED_SEGMENT) {
		dos_memory_free(umb_segment);
	}
}

void himem_init(void)
{
	ems_init();
	umb_init();
	highpool_reserve_window();
}
