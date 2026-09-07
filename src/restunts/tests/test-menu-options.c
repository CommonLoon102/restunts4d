#include <string.h>
#define main car_snapshot_main
#define locate_text_res car_fixture_locate_text_res
#include "test-car-menu.c"
#undef main
#undef locate_text_res
#include "../c/audio_control.h"
#include "../c/fatal.h"
#include "../c/ui_dialog_internal.h"
#include "../c/replay_viewer_internal.h"

static legacy_s16 dialog_answers[24];
static unsigned dialog_index, joystick_index, joystick_limit;
static legacy_u8 joystick_enabled;
static legacy_s16 keyboard_stop;
static legacy_s8 graphics_text[] = "[ ]0[ ]1[ ]2[ ]3[ ]4[ ]5[ ]6[ ]7[ ]8";

legacy_s8 *locate_text_res(legacy_s8 *data, const legacy_s8 *name)
{
	trace_word(2000);
	trace_pointer(data);
	trace_text(name);
	return _strcmp(name, graphics_options_dialog_id) == 0 ? graphics_text : (legacy_s8 *)name;
}
void input_push_status(void)
{
	trace_word(2001);
}
void input_pop_status(void)
{
	trace_word(2002);
}
void dos_timer_set_callbacks_suspended(legacy_s16 flag)
{
	trace_word(2003);
	trace_word(flag);
}
void audio_suspend(void)
{
	trace_word(2004);
}
void audio_resume(void)
{
	trace_word(2005);
}
legacy_s16 audio_toggle_music(void)
{
	trace_word(2006);
	return scenario % 2U;
}
legacy_s16 audio_toggle_effects(void)
{
	trace_word(2007);
	return scenario % 2U;
}
legacy_s16 kb_check(void)
{
	trace_word(2008);
	return 0;
}
legacy_s16 kb_read_char(void)
{
	trace_word(2009);
	return keyboard_stop != 0 && joystick_index == joystick_limit ? KEY_ESCAPE : 0;
}
void dos_joystick_set_enabled(legacy_u8 enabled)
{
	trace_word(2010);
	trace_word(enabled);
	joystick_enabled = enabled;
}
legacy_u8 dos_joystick_is_enabled(void)
{
	trace_word(2011);
	return joystick_enabled;
}
void joystick_reset_calibration(void)
{
	trace_word(2012);
}
legacy_s16 dos_get_joy_flags(void)
{
	trace_word(2013);
	if (joystick_index == joystick_limit) {
		return 16;
	}
	return (legacy_s16)(joystick_index++ / 2U % 9U);
}
legacy_s16 input_direction_from_flags(legacy_s16 flags)
{
	trace_word(2014);
	trace_word(flags);
	return flags;
}
void sprite_pop_background(void)
{
	trace_word(2015);
}
void sprite_fill_rect(legacy_s16 x, legacy_s16 y, legacy_s16 w, legacy_s16 h, legacy_s16 color)
{
	trace_word(2016);
	trace_word(x);
	trace_word(y);
	trace_word(w);
	trace_word(h);
	trace_word(color);
}
legacy_u16 show_dialog(legacy_s16 type, legacy_s16 save, void *text, legacy_u16 x, legacy_u16 y,
					   legacy_s16 border, legacy_s16 *positions, legacy_s16 selected)
{
	unsigned i;
	trace_word(2017);
	trace_word(type);
	trace_word(save);
	trace_text(text);
	trace_word(x);
	trace_word(y);
	trace_word(border);
	trace_word(selected);
	if (positions != 0) {
		for (i = 0; i < 15U; i++) {
			positions[i] = LEGACY_S16_FROM_BITS(32750U + i * 43U);
		}
	}
	assert(dialog_index < 24U);
	return (legacy_u16)dialog_answers[dialog_index++];
}
void call_exitlist2(void)
{
	trace_word(2018);
}
void copy_string(legacy_s8 *dst, legacy_s8 *src)
{
	_strcpy(dst, src);
}
legacy_s8 *locate_shape_alt(legacy_s8 *data, const legacy_s8 *name)
{
	trace_word(2019);
	trace_pointer(data);
	trace_text(name);
	return (legacy_s8 *)name;
}
struct RECTANGLE *intro_draw_text(legacy_s8 *text, legacy_s16 x, legacy_s16 y, legacy_s16 color,
								  legacy_s16 mode)
{
	trace_word(2020);
	trace_text(text);
	trace_word(x);
	trace_word(y);
	trace_word(color);
	trace_word(mode);
	return 0;
}
legacy_s16 font_centered_text_x(const legacy_s8 *text)
{
	trace_word(2021);
	trace_text(text);
	return 100;
}
legacy_s8 do_fileselect_dialog(legacy_s8 *dir, legacy_s8 *name, legacy_s8 *ext, legacy_s8 *prompt)
{
	trace_word(2022);
	trace_text(dir);
	trace_text(name);
	trace_text(ext);
	trace_text(prompt);
	return scenario % 2U;
}
void show_waiting(void)
{
	trace_word(2023);
	trace_word(waitflag);
}
legacy_s16 file_load_replay(const legacy_s8 *dir, const legacy_s8 *name)
{
	trace_word(2024);
	trace_text(dir);
	trace_text(name);
	return 0;
}

static void reset_options(unsigned index)
{
	unsigned i;
	scenario = index;
	allocation_index = 0;
	dialog_index = 0;
	joystick_index = 0;
	joystick_limit = 18;
	keyboard_stop = 0;
	joystick_enabled = index % 2U;
	mouse_driving_enabled = index % 3U;
	detail_level = index % 5U;
	slow_video_mgmt = index % 2U;
	configured_frame_rate = index % 2U ? GAME_FRAME_RATE_LOW : GAME_FRAME_RATE_NORMAL;
	mainresptr = resource_bytes[50];
	dialog_border_color = 3;
	dialog_fnt_colour = 7;
	dialog_background_color = 9;
	performGraphColor = 10;
	graphics_menu_background_color = 11;
	waitflag = 65530;
	replay_directory[0] = 0;
	replay_filename_input[0] = 0;
	for (i = 0; i < 24U; i++) {
		dialog_answers[i] = -1;
	}
	trace_word(index);
}
static void record_options(void)
{
	trace_word(joystick_enabled);
	trace_word(mouse_driving_enabled);
	trace_word(detail_level);
	trace_word(slow_video_mgmt);
	trace_word(configured_frame_rate);
	trace_word(waitflag);
}
static void test_calibration(void)
{
	unsigned i;
	for (i = 0; i < 6U; i++) {
		reset_options(i);
		dialog_answers[0] = i < 2U ? (legacy_s16)i - 1 : 1;
		joystick_limit = i == 2U ? 0 : i == 3U ? 16 : 18;
		keyboard_stop = i % 2U;
		calibrate_joystick_driving();
		record_options();
	}
}
static void test_graphics(void)
{
	unsigned i, j;
	for (i = 0; i < 10U; i++) {
		reset_options(i + 10U);
		for (j = 0; j < 9U; j++) {
			dialog_answers[j] = (legacy_s16)((i + j) % 9U);
		}
		dialog_answers[9] = i % 2U ? 9 : -1;
		show_graphic_levels_menu();
		record_options();
	}
}
static void test_options(void)
{
	unsigned i;
	for (i = 0; i < 16U; i++) {
		reset_options(i + 20U);
		dialog_answers[0] = (legacy_s16)(i / 2U) - 1;
		if (dialog_answers[0] == 0) {
			dialog_answers[1] = i % 2U ? 2 : 0;
		} else if (dialog_answers[0] == 5) {
			dialog_answers[1] = i % 2U;
		}
		trace_word(run_option_menu());
		record_options();
	}
	reset_options(40);
	dialog_answers[0] = 0;
	dialog_answers[1] = 1;
	dialog_answers[2] = 1;
	trace_word(run_option_menu());
	record_options();
}
int main(void)
{
	test_calibration();
	test_graphics();
	test_options();
	assert(trace_hash == UINT64_C(0x52549b0fa5c3a024));
	printf("test-menu-options: passed\n");
	return 0;
}
