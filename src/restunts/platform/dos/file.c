#include <dos.h>

#include "platform.h"

#define DOS_FILE_INTERRUPT 33
#define DOS_FILE_LFN_OPEN_FUNCTION 29036
#define DOS_FILE_CREATE_FUNCTION 60
#define DOS_FILE_OPEN_FUNCTION 61
#define DOS_FILE_CLOSE_FUNCTION 62
#define DOS_FILE_READ_FUNCTION 63
#define DOS_FILE_WRITE_FUNCTION 64
#define DOS_FILE_REMOVE_FUNCTION 65
#define DOS_FILE_SEEK_FUNCTION 66
#define DOS_FILE_SET_DTA_FUNCTION 26
#define DOS_FILE_FIND_FIRST_FUNCTION 78
#define DOS_FILE_FIND_NEXT_FUNCTION 79
#define DOS_FILE_READ_ONLY_ACCESS 0
#define DOS_FILE_READ_WRITE_ACCESS 2U
#define DOS_FILE_OPEN_EXISTING_ACTION 1U
#define DOS_FILE_CREATE_OR_TRUNCATE_ACTION 18U
#define DOS_FILE_DEFAULT_ATTRIBUTES 0
#define DOS_FILE_INVALID_HANDLE 0
#define DOS_FILE_IO_ERROR 1
#define DOS_FILE_SEEK_COMMAND_BASE 16896U
#define DOS_FILE_SEEK_CURRENT_ORIGIN 1

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
	access_mode = create != 0 ? DOS_FILE_READ_WRITE_ACCESS :
		DOS_FILE_READ_ONLY_ACCESS;
	action = create != 0 ? DOS_FILE_CREATE_OR_TRUNCATE_ACTION :
		DOS_FILE_OPEN_EXISTING_ACTION;
	handle = 0;
	__asm {
		push si
		push di
		mov ax, DOS_FILE_LFN_OPEN_FUNCTION
		mov bx, access_mode
		mov cx, DOS_FILE_DEFAULT_ATTRIBUTES
		mov dx, action
		mov si, path_offset
		mov di, DOS_FILE_INVALID_HANDLE
		int DOS_FILE_INTERRUPT
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
			mov ah, DOS_FILE_CREATE_FUNCTION
			mov cx, DOS_FILE_DEFAULT_ATTRIBUTES
			mov dx, path_offset
			int DOS_FILE_INTERRUPT
			jnc short create_ok
			mov ax, DOS_FILE_INVALID_HANDLE
			mov dos_file_errno, DOS_FILE_IO_ERROR
		create_ok:
			mov handle, ax
		}
	} else {
		__asm {
			mov ah, DOS_FILE_OPEN_FUNCTION
			mov al, DOS_FILE_READ_ONLY_ACCESS
			mov dx, path_offset
			int DOS_FILE_INTERRUPT
			jnc short open_ok
			mov ax, DOS_FILE_INVALID_HANDLE
			mov dos_file_errno, DOS_FILE_IO_ERROR
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
		mov ah, DOS_FILE_CLOSE_FUNCTION
		mov bx, handle
		int DOS_FILE_INTERRUPT
		jnc short close_ok
		mov dos_file_errno, DOS_FILE_IO_ERROR
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
		mov ah, DOS_FILE_READ_FUNCTION
		mov bx, handle
		mov ds, segment
		mov dx, buffer_offset
		mov cx, length
		int DOS_FILE_INTERRUPT
		jc short read_failed
		mov result, ax
		pop ds
		jmp short read_done
	read_failed:
		pop ds
		mov dos_file_errno, DOS_FILE_IO_ERROR
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
		mov ah, DOS_FILE_WRITE_FUNCTION
		mov bx, handle
		mov ds, segment
		mov dx, buffer_offset
		mov cx, length
		int DOS_FILE_INTERRUPT
		jc short write_failed
		mov result, ax
		pop ds
		jmp short write_done
	write_failed:
		pop ds
		mov dos_file_errno, DOS_FILE_IO_ERROR
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
	high = (legacy_u16)((legacy_u32)offset >> LEGACY_WORD_BITS);
	command = (legacy_u16)(DOS_FILE_SEEK_COMMAND_BASE |
		(legacy_u16)origin);
	__asm {
		mov ax, command
		mov bx, handle
		mov cx, high
		mov dx, low
		int DOS_FILE_INTERRUPT
		jnc short seek_ok
		mov dos_file_errno, DOS_FILE_IO_ERROR
	seek_ok:
	}
	return 0;
}

legacy_s32 dos_file_tell(legacy_u16 handle)
{
	legacy_u16 low;
	legacy_u16 high;

	__asm {
		mov ah, DOS_FILE_SEEK_FUNCTION
		mov al, DOS_FILE_SEEK_CURRENT_ORIGIN
		mov bx, handle
		mov cx, 0
		mov dx, 0
		int DOS_FILE_INTERRUPT
		jnc short tell_ok
		mov dos_file_errno, DOS_FILE_IO_ERROR
		tell_ok:
		mov high, dx
		mov low, ax
	}
	return ((legacy_s32)high << LEGACY_WORD_BITS) | low;
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
		mov ah, DOS_FILE_REMOVE_FUNCTION
		mov ds, segment
		mov dx, path_offset
		int DOS_FILE_INTERRUPT
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
		mov ah, DOS_FILE_SET_DTA_FUNCTION
		mov dx, offset dos_find_data
		int DOS_FILE_INTERRUPT

		mov ah, DOS_FILE_FIND_FIRST_FUNCTION
		mov cl, attributes
		mov dx, query
		int DOS_FILE_INTERRUPT

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
		mov ah, DOS_FILE_SET_DTA_FUNCTION
		mov dx, offset dos_find_data
		int DOS_FILE_INTERRUPT

		mov ah, DOS_FILE_FIND_NEXT_FUNCTION
		int DOS_FILE_INTERRUPT

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
