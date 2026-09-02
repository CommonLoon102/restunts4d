#include <dos.h>

#include "platform.h"

static legacy_s16 dos_file_errno;
static struct find_t dos_find_data;

static legacy_u16 dos_file_open_lfn(const legacy_s8* path,
	legacy_s16 create)
{
	legacy_u16 path_offset;
	legacy_u16 access_mode;
	legacy_u16 action;
	legacy_u16 handle;

	path_offset = FP_OFF(path);
	access_mode = create != 0 ? 2U : 0U;
	action = create != 0 ? 0x12U : 1U;
	handle = 0;
	__asm {
		push si
		push di
		mov ax, 716Ch
		mov bx, access_mode
		mov cx, 0
		mov dx, action
		mov si, path_offset
		mov di, 0
		int 21h
		jc short lfn_open_done
		mov handle, ax
	lfn_open_done:
		pop di
		pop si
	}
	return handle;
}

legacy_u16 dos_file_open(const legacy_s8* path, legacy_s16 create)
{
	legacy_u16 path_offset;
	legacy_u16 handle;

	path_offset = FP_OFF(path);
	dos_file_errno = 0;
	handle = dos_file_open_lfn(path, create);
	if (handle != 0)
		return handle;
	if (create != 0) {
		__asm {
			mov ah, 3Ch
			mov cx, 0
			mov dx, path_offset
			int 21h
			jnc short create_ok
			mov ax, 0
			mov dos_file_errno, 1
		create_ok:
			mov handle, ax
		}
	} else {
		__asm {
			mov ah, 3Dh
			mov al, 0
			mov dx, path_offset
			int 21h
			jnc short open_ok
			mov ax, 0
			mov dos_file_errno, 1
		open_ok:
			mov handle, ax
		}
	}
	return handle;
}

legacy_s16 dos_file_close(legacy_u16 handle)
{
	legacy_s16 result;

	result = 0;
	__asm {
		mov ah, 3Eh
		mov bx, handle
		int 21h
		jnc short close_ok
		mov dos_file_errno, 1
		mov result, -1
	close_ok:
	}
	return result;
}

legacy_u16 dos_file_read(legacy_u16 handle, void far* destination,
	legacy_u16 length)
{
	legacy_u16 segment;
	legacy_u16 buffer_offset;
	legacy_u16 result;

	segment = FP_SEG(destination);
	buffer_offset = FP_OFF(destination);
	__asm {
		push ds
		mov ah, 3Fh
		mov bx, handle
		mov ds, segment
		mov dx, buffer_offset
		mov cx, length
		int 21h
		jc short read_failed
		mov result, ax
		pop ds
		jmp short read_done
	read_failed:
		pop ds
		mov dos_file_errno, 1
		mov result, 0
	read_done:
	}
	return result;
}

legacy_u16 dos_file_write(legacy_u16 handle, const void far* source,
	legacy_u16 length)
{
	legacy_u16 segment;
	legacy_u16 buffer_offset;
	legacy_u16 result;

	segment = FP_SEG(source);
	buffer_offset = FP_OFF(source);
	__asm {
		push ds
		mov ah, 40h
		mov bx, handle
		mov ds, segment
		mov dx, buffer_offset
		mov cx, length
		int 21h
		jc short write_failed
		mov result, ax
		pop ds
		jmp short write_done
	write_failed:
		pop ds
		mov dos_file_errno, 1
		mov result, 0
	write_done:
	}
	return result;
}

legacy_s16 dos_file_seek(legacy_u16 handle, legacy_s32 offset,
	legacy_s16 origin)
{
	legacy_u16 low;
	legacy_u16 high;
	legacy_u16 command;

	low = (legacy_u16)offset;
	high = (legacy_u16)((legacy_u32)offset >> 16);
	command = (legacy_u16)(0x4200U | (legacy_u16)origin);
	__asm {
		mov ax, command
		mov bx, handle
		mov cx, high
		mov dx, low
		int 21h
		jnc short seek_ok
		mov dos_file_errno, 1
	seek_ok:
	}
	return 0;
}

legacy_s32 dos_file_tell(legacy_u16 handle)
{
	legacy_u16 low;
	legacy_u16 high;

	__asm {
		mov ah, 42h
		mov al, 1
		mov bx, handle
		mov cx, 0
		mov dx, 0
		int 21h
		jnc short tell_ok
		mov dos_file_errno, 1
		tell_ok:
		mov high, dx
		mov low, ax
	}
	return ((legacy_s32)high << 16) | low;
}

legacy_s16 dos_file_error(void)
{
	legacy_s16 result;

	result = dos_file_errno;
	dos_file_errno = 0;
	return result;
}

legacy_s16 dos_file_remove(const legacy_s8* path)
{
	legacy_u16 segment;
	legacy_u16 path_offset;
	legacy_s16 result;

	segment = FP_SEG(path);
	path_offset = FP_OFF(path);
	__asm {
		push ds
		mov ah, 41h
		mov ds, segment
		mov dx, path_offset
		int 21h
		jnc short unlink_ok
		mov result, -1
		mov dos_file_errno, ax
		jmp short unlink_done
	unlink_ok:
		mov result, 0
	unlink_done:
		pop ds
	}
	return result;
}

const legacy_s8* dos_file_find_first(const legacy_s8* query)
{
	legacy_u8 attributes;
	legacy_s16 result;

	attributes = FA_NORMAL | FA_HIDDEN | FA_SYSTEM;
	__asm {
		mov ah, 1Ah
		mov dx, offset dos_find_data
		int 21h

		mov ah, 4Eh
		mov cl, attributes
		mov dx, query
		int 21h

		jnc short find_ok
		mov result, -1
		jmp short find_done
	find_ok:
		mov result, 0
	find_done:
	}
	if (result != 0)
		return 0;
	return dos_find_data.name;
}

const legacy_s8* dos_file_find_next(void)
{
	legacy_s16 result;

	__asm {
		mov ah, 1Ah
		mov dx, offset dos_find_data
		int 21h

		mov ah, 4Fh
		int 21h

		jnc short find_next_ok
		mov result, -1
		jmp short find_next_done
	find_next_ok:
		mov result, 0
	find_next_done:
	}
	if (result != 0)
		return 0;
	return dos_find_data.name;
}
