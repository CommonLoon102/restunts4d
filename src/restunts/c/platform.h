#ifndef RESTUNTS_PLATFORM_H
#define RESTUNTS_PLATFORM_H

#include "legacy.h"

void far* dos_memory_get_psp(void);
legacy_u16 dos_memory_allocate(legacy_u16 paragraphs);
legacy_u16 dos_memory_resize(legacy_u16 segment, legacy_u16 paragraphs);

legacy_u16 dos_file_open(const legacy_s8* path, legacy_s16 create);
legacy_s16 dos_file_close(legacy_u16 handle);
legacy_u16 dos_file_read(legacy_u16 handle, void far* destination,
	legacy_u16 length);
legacy_u16 dos_file_write(legacy_u16 handle, const void far* source,
	legacy_u16 length);
legacy_s16 dos_file_seek(legacy_u16 handle, legacy_s32 offset,
	legacy_s16 origin);
legacy_s32 dos_file_tell(legacy_u16 handle);
legacy_s16 dos_file_error(void);
legacy_s16 dos_file_remove(const legacy_s8* path);
const legacy_s8* dos_file_find_first(const legacy_s8* query);
const legacy_s8* dos_file_find_next(void);

void dos_install_divide_error_handler(void);

legacy_s16 dos_write_stdout(const legacy_s8* text, legacy_u16 length);
legacy_s16 dos_write_stderr(const legacy_s8* text, legacy_u16 length);
void dos_process_exit(legacy_s16 status);
legacy_s16 dos_data_stack_segments_match(void);

legacy_s16 dos_timer_register_callback(void (far* callback)(void));
void dos_timer_unregister_callback(void (far* callback)(void));

#endif
