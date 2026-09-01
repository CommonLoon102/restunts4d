#ifndef RESTUNTS_PLATFORM_H
#define RESTUNTS_PLATFORM_H

#include "legacy.h"

struct AUDIO_CONTEXT;

void far* dos_memory_get_psp(void);
#if defined(__BORLANDC__)
#define dos_memory_make_pointer(segment, offset) \
	((void _seg*)(segment) + (void near*)(offset))
#define dos_memory_make_near_pointer(offset) ((void near*)(offset))
#define dos_memory_pointer_segment(pointer) \
	((legacy_u16)(void _seg*)(void far*)(pointer))
#define dos_memory_pointer_offset(pointer) ((legacy_u16)(pointer))
#else
void far* dos_memory_make_pointer(legacy_u16 segment, legacy_u16 offset);
void* dos_memory_make_near_pointer(legacy_u16 offset);
legacy_u16 dos_memory_pointer_segment(const void far* pointer);
legacy_u16 dos_memory_pointer_offset(const void far* pointer);
#endif
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

void dos_interrupts_disable(void);
void dos_interrupts_enable(void);
void dos_set_critical_error_handler(legacy_s16 (far* callback)(void));

legacy_s16 dos_write_stdout(const legacy_s8* text, legacy_u16 length);
legacy_s16 dos_write_stderr(const legacy_s8* text, legacy_u16 length);
void dos_process_exit(legacy_s16 status);
legacy_s16 dos_data_stack_segments_match(void);

legacy_s16 dos_get_joy_flags(void);
void dos_joystick_reset_calibration(void);
void dos_joystick_set_enabled(legacy_u8 enabled);
legacy_u8 dos_joystick_is_enabled(void);
legacy_s16 dos_joystick_get_scaled_axis(legacy_u16 axis_index);

legacy_s16 dos_mouse_init(legacy_s16 width, legacy_s16 height);
void dos_mouse_set_minmax(legacy_s16 minimum_x, legacy_s16 minimum_y,
	legacy_s16 maximum_x, legacy_s16 maximum_y);
void dos_mouse_set_position(legacy_s16 x, legacy_s16 y);
void dos_mouse_get_state(legacy_s16* buttons, legacy_s16* x,
	legacy_s16* y);
legacy_u16 dos_mouse_get_button_count(void);

legacy_s16 dos_timer_register_callback(void (far* callback)(void));
void dos_timer_unregister_callback(void (far* callback)(void));
void dos_timer_reset_counter(void);
void dos_timer_set_callbacks_suspended(legacy_s16 suspended);
legacy_u32 timer_get_counter(void);
legacy_u32 timer_get_delta(void);
legacy_u32 timer_get_slow_counter(void);
void dos_timer_setup_interrupt(void);
void dos_timer_shutdown(void);

legacy_s16 dos_video_get_status(void);
void dos_video_set_palette(legacy_u16 start, legacy_u16 count,
	legacy_u8* palette);
void dos_video_set_mode_13h(void);
void dos_video_set_mode4(void);
void dos_video_set_mode7(void);

legacy_u8 dos_audio_driver_initialize(void);
void dos_audio_driver_load_bank(void far* bank);
void dos_audio_driver_prepare_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u8* timer,
	void far* resource);
void dos_audio_driver_set_context_value(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u16 value);
void dos_audio_driver_set_control(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u16 control,
	legacy_u16 value);
void dos_audio_driver_set_pitch(legacy_u8* timer, legacy_s16 pitch,
	legacy_s16 driver_channel);
void dos_audio_driver_send_data(legacy_u16 length, legacy_u8* data);
void dos_audio_driver_activate_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u8* timer, legacy_s16 pitch,
	legacy_u16 parameter, void far* resource);
void dos_audio_driver_release_channel(legacy_s16 driver_channel);
void dos_audio_driver_start_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context);
void dos_audio_driver_end_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context);
void dos_audio_driver_reset(void);
void dos_audio_driver_start(void);
void dos_audio_driver_suspend_context(legacy_s16 driver_channel,
	struct AUDIO_CONTEXT* driver_context, legacy_u16 value,
	void far* resource);
void dos_audio_driver_suspend_all(struct AUDIO_CONTEXT* contexts);
void dos_audio_driver_set_master_state(legacy_s16 operation,
	void far* state);
void dos_audio_bind_channel_context(legacy_s16 channel,
	void far* resource);
void dos_audio_set_channel_volume(legacy_s16 channel,
	legacy_s16 volume);
void dos_audio_set_context_pitch(legacy_s16 context_index,
	legacy_s16 pitch);
void dos_audio_shutdown(void);

#endif
