#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../c/ui_dialog.h"
#include "../c/ui_dialog_internal.h"
#include "../c/ui_text.h"
#include "../c/shape2d.h"
#include "../c/timing.h"
#include "../c/game_input.h"
#include "../c/keyboard.h"
#include "../c/externs.h"

legacy_s16 font_glyph_height;
legacy_s16 dialog_fnt_colour;
legacy_s16 dialog_background_color;
legacy_s16 performGraphColor;
void *mainresptr;
legacy_s8 pause_dialog_id[] = "pau";
legacy_s8 disk_retry_dialog_id[] = "dea";
legacy_s8 disk_error_dialog_id[] = "der";
legacy_s8 g_is_busy;
legacy_u16 dialog_border_color;

static unsigned int dialog_lifecycle_active;
static unsigned int background_depth;
static unsigned int background_releases;
static unsigned int input_status_depth;
static legacy_s16 timer_suspended;
static legacy_s16 audio_suspended;

static uint64_t trace_hash = UINT64_C(1469598103934665603);
static legacy_u16 scripted_keys[16];
static legacy_s16 scripted_hits[16];
static unsigned int input_index;
static unsigned int timer_calls;
static legacy_s16 save_succeeds;

static void trace_word(legacy_u16 value)
{
	trace_hash = (trace_hash ^ (value & 255U)) * UINT64_C(1099511628211);
	trace_hash = (trace_hash ^ (value >> 8)) * UINT64_C(1099511628211);
}

static void trace_rectangle(legacy_s16 left, legacy_s16 right, legacy_s16 top, legacy_s16 bottom)
{
	trace_word((legacy_u16)left);
	trace_word((legacy_u16)right);
	trace_word((legacy_u16)top);
	trace_word((legacy_u16)bottom);
}

void mouse_draw_opaque_check(void)
{
	trace_word(1);
}
void mouse_draw_transparent_check(void)
{
	trace_word(2);
}
void sprite_pop_background(void)
{
	trace_word(3);
	if (dialog_lifecycle_active != 0U) {
		assert(background_depth == 1U);
		if (dialog_lifecycle_active == 1U) {
			assert(timer_suspended != 0 && audio_suspended != 0);
		}
		background_depth--;
		background_releases++;
	}
}
void sprite_select_screen(void)
{
	trace_word(4);
}
void check_input(void)
{
	trace_word(5);
}
void sprite_clear_target(legacy_u8 color)
{
	trace_word(6);
	trace_word(color);
}

legacy_s16 sprite_push_background(legacy_s16 left, legacy_s16 right, legacy_s16 top,
								  legacy_s16 bottom)
{
	trace_word(7);
	trace_rectangle(left, right, top, bottom);
	if (dialog_lifecycle_active != 0U && save_succeeds != 0) {
		assert(background_depth == 0U);
		background_depth++;
	}
	return save_succeeds;
}

void sprite_set_target_clip_bounds(legacy_u16 left, legacy_u16 right, legacy_u16 top,
								   legacy_u16 bottom)
{
	trace_word(8);
	trace_rectangle(left, right, top, bottom);
}

void sprite_draw_rect_outline(legacy_s16 left, legacy_s16 top, legacy_s16 right, legacy_s16 bottom,
							  legacy_s16 color)
{
	trace_word(9);
	trace_rectangle(left, right, top, bottom);
	trace_word(color);
}

void font_set_colors(legacy_s16 color, legacy_s16 background_color)
{
	trace_word(10);
	trace_word(color);
	trace_word(background_color);
}

legacy_s16 font_text_width(const legacy_s8 *text)
{
	legacy_s16 width = 0;
	trace_word(11);
	while (*text != 0) {
		trace_word((legacy_u8)*text);
		width += 2 + ((legacy_u8)*text++ % 5);
	}
	trace_word(0);
	return width;
}

void font_draw_text_opaque(const legacy_s8 *text, legacy_s16 x, legacy_s16 y)
{
	trace_word(12);
	trace_word(x);
	trace_word(y);
	while (*text != 0) {
		trace_word((legacy_u8)*text++);
	}
	trace_word(0);
}

legacy_u32 timer_get_delta_alt(void)
{
	trace_word(13);
	timer_calls++;
	return 3U * timer_calls;
}

legacy_u32 slow_timer_wait_ticks(legacy_u32 ticks)
{
	trace_word(14);
	trace_word((legacy_u16)ticks);
	return ticks;
}

legacy_s16 input_checking(legacy_s16 delta)
{
	trace_word(15);
	trace_word(delta);
	assert(input_index < 16U);
	if (dialog_lifecycle_active != 0U) {
		assert(background_depth == 1U && input_status_depth == 1U);
		if (dialog_lifecycle_active == 1U) {
			assert(timer_suspended != 0 && audio_suspended != 0);
		}
	}
	return (legacy_s16)scripted_keys[input_index++];
}

legacy_s16 mouse_multi_hittest(legacy_s16 count, const struct BUTTON_AREA *buttons)
{
	legacy_s16 i;
	trace_word(16);
	trace_word(count);
	for (i = 0; i < count; i++) {
		trace_rectangle(buttons[i].x1, buttons[i].x2, buttons[i].y1, buttons[i].y2);
	}
	return scripted_hits[input_index - 1U];
}

legacy_s8 *locate_text_res(legacy_s8 *data, const legacy_s8 *name)
{
	static legacy_s8 pause_text[] = "Paused]Press any key]";

	(void)data;
	assert(name == pause_dialog_id || name == disk_error_dialog_id);
	return pause_text;
}

void input_push_status(void)
{
	assert(input_status_depth == 0U);
	input_status_depth++;
}

void input_pop_status(void)
{
	assert(input_status_depth == 1U);
	assert(background_depth == 0U);
	input_status_depth--;
}

void dos_timer_set_callbacks_suspended(legacy_s16 suspended)
{
	if (suspended == 0) {
		assert(background_depth == 0U);
	}
	timer_suspended = suspended;
}

void audio_suspend(void)
{
	audio_suspended = 1;
}

void audio_resume(void)
{
	assert(background_depth == 0U);
	audio_suspended = 0;
}

static void test_pause_lifecycle(void)
{
	static const legacy_u16 resume_keys[] = {KEY_ENTER, KEY_ESCAPE, KEY_SPACE};
	unsigned int index;

	/* Original do_pau_restext passes acknowledgement type 1. A draw-only
	 * message returns immediately and leaves its saved window above the race
	 * window, causing the later race teardown to fail its LIFO check. */
	dialog_lifecycle_active = 1U;
	save_succeeds = 1;
	font_glyph_height = 8;
	for (index = 0; index < sizeof(resume_keys) / sizeof(resume_keys[0]); index++) {
		input_index = 0;
		scripted_keys[0] = 0;
		scripted_keys[1] = 0;
		scripted_keys[2] = resume_keys[index];
		show_pause_dialog();
		assert(input_index == 3U);
		assert(background_depth == 0U && background_releases == index + 1U);
		assert(input_status_depth == 0U);
		assert(timer_suspended == 0 && audio_suspended == 0);
	}
	dialog_lifecycle_active = 0U;
}

static void test_disk_error_lifecycle(void)
{
	static const legacy_u16 acknowledgement_keys[] = {KEY_ENTER, KEY_ESCAPE, KEY_SPACE};
	unsigned int index;
	unsigned int previous_releases;

	/* The non-retry disk error uses the same acknowledgement lifecycle as
	 * pause. Its caller must not return with a saved window still allocated. */
	dialog_lifecycle_active = 2U;
	save_succeeds = 1;
	g_is_busy = 0;
	previous_releases = background_releases;
	for (index = 0; index < sizeof(acknowledgement_keys) / sizeof(acknowledgement_keys[0]);
		 index++) {
		input_index = 0;
		scripted_keys[0] = 0;
		scripted_keys[1] = acknowledgement_keys[index];
		assert(show_disk_error_dialog() == 1);
		assert(input_index == 2U);
		assert(background_depth == 0U);
		assert(background_releases == previous_releases + index + 1U);
		assert(input_status_depth == 0U);
	}
	dialog_lifecycle_active = 0U;
}

static void configure_input(unsigned int scenario, legacy_s16 choice_count)
{
	static const legacy_u16 navigation[] = {0, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, 'X'};
	unsigned int i;
	for (i = 0; i < 16U; i++) {
		scripted_keys[i] = KEY_ENTER;
		scripted_hits[i] = -1;
	}
	for (i = 0; i < 6U; i++) {
		scripted_keys[i] = navigation[(scenario + i) % 6U];
		if ((scenario + i) % 3U == 0U) {
			scripted_hits[i] = (scenario + i) % choice_count;
		}
	}
	if (scenario % 4U == 0U) {
		scripted_keys[6] = KEY_ESCAPE;
	}
	if (scenario % 4U == 1U) {
		scripted_keys[6] = KEY_SPACE;
	}
	if (scenario % 4U == 2U) {
		scripted_keys[6] = 'a';
	}
	if (scenario % 4U == 3U) {
		scripted_keys[6] = 'B';
	}
	input_index = 0;
	timer_calls = 0;
}

static void run_dialog_case(unsigned int scenario)
{
	static const legacy_s8 *texts[] = {(const legacy_s8 *)"Heading]Text @ here}@[ Alpha][ Beta]",
									   (const legacy_s8 *)"Heading]Text} [ Alpha] [ Beta] [ Gamma]",
									   (const legacy_s8 *)"Prompt}[ Alpha] [ Beta]",
									   (const legacy_s8 *)"Heading] [ Alpha][ Beta][ Gamma]"};
	legacy_s16 choices[40];
	unsigned int i;
	legacy_u16 result;
	legacy_s16 type = scenario % 7U;
	legacy_s16 count = scenario % 2U == 0U ? 2 : 3;
	for (i = 0; i < 40U; i++) {
		choices[i] = 0;
	}
	if (scenario % 5U == 0U) {
		choices[1] = 1;
	}
	configure_input(scenario, count);
	save_succeeds = scenario % 11U != 0U;
	font_glyph_height = 5 + scenario % 8U;
	dialog_fnt_colour = scenario % 16U;
	dialog_background_color = 7;
	performGraphColor = 9;
	trace_word((legacy_u16)scenario);
	result = show_dialog(type, scenario % 2U, (void *)texts[scenario % 4U],
						 scenario % 3U == 0U ? DIALOG_AUTO_POSITION : (legacy_u16)(scenario % 400U),
						 scenario % 3U == 1U ? DIALOG_AUTO_POSITION : (legacy_u16)(scenario % 250U),
						 3, scenario % 3U == 0U && type != DIALOG_TYPE_PLACEHOLDERS ? 0 : choices,
						 scenario % count);
	trace_word(result);
	for (i = 0; i < 40U; i++) {
		trace_word(choices[i]);
	}
	trace_word(dialog_background_color);
}

int main(void)
{
	unsigned int scenario;
	for (scenario = 0; scenario < 420U; scenario++) {
		run_dialog_case(scenario);
	}
	/* Captured from the original dialog implementation. The trace includes drawing,
	 * geometry, disabled choices, placeholders, input polling and background lifetime. */
	assert(trace_hash == UINT64_C(0x268e59aba981d897));
	test_pause_lifecycle();
	test_disk_error_lifecycle();
	puts("Dialog snapshots and pause/disk-error lifecycles passed (420 scenarios).");
	return 0;
}
