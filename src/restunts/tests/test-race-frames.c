/* Full frame-loop endpoint traces captured before extraction cover waiting,
 * live/replay/paused input, exit requests and sequential drawing decisions. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../c/race.c"
#undef printf
#undef memset

static uint32_t trace_hash = UINT32_C(2166136261);
static unsigned scenario, frames, keys;
static struct RECTANGLE dirty_rect;
static legacy_s8 text_resource[8];
static void trace(legacy_u32 value)
{
	for (unsigned i = 0; i < 4; i++) {
		trace_hash = (trace_hash ^ (value & 255U)) * UINT32_C(16777619);
		value >>= 8;
	}
}
static void trace_rect(const struct RECTANGLE *rect)
{
	trace(rect->left);
	trace(rect->right);
	trace(rect->top);
	trace(rect->bottom);
}
legacy_u8 dos_joystick_is_enabled(void)
{
	trace(1);
	return scenario & 1;
}
void replay_apply_analog_steering_history(void)
{
	trace(2);
}
void update_gamestate(void)
{
	trace(3);
	state.game_frame++;
}
void init_rect_arrays(void)
{
	trace(4);
}
void input_push_status(void)
{
	trace(5);
}
void input_pop_status(void)
{
	trace(6);
}
void audio_suspend(void)
{
	trace(7);
}
void audio_resume(void)
{
	trace(8);
}
legacy_s8 far *locate_text_res(legacy_s8 far *resource, const legacy_s8 *name)
{
	(void)resource;
	trace(9);
	trace(name[0]);
	return text_resource;
}
legacy_u16 show_dialog(legacy_s16 type, legacy_s16 save, void far *text, legacy_u16 x, legacy_u16 y,
					   legacy_s16 color, legacy_s16 *flags, legacy_s16 selected)
{
	assert(text == text_resource);
	assert(flags == 0);
	trace(10);
	trace(type);
	trace(save);
	trace(x);
	trace(y);
	trace(color);
	trace(selected);
	return scenario & 1 ? 65535U : scenario & 2;
}
void dos_timer_set_callbacks_suspended(legacy_s16 value)
{
	trace(11);
	trace(value);
}
void update_crash_state(legacy_s16 event, legacy_s16 car)
{
	trace(12);
	trace(event);
	trace(car);
}
void sprite_select_mcga_backbuffer(void)
{
	trace(13);
}
void sprite_select_render_window(void)
{
	trace(14);
}
void set_projection(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height)
{
	trace(15);
	trace(x);
	trace(y);
	trace(width);
	trace(height);
}
void sprite_set_target_clip_bounds(legacy_u16 left, legacy_u16 right, legacy_u16 top,
								   legacy_u16 bottom)
{
	trace(16);
	trace(left);
	trace(right);
	trace(top);
	trace(bottom);
}
void setup_car_shapes(legacy_s16 operation)
{
	trace(17);
	trace(operation);
}
void loop_game(legacy_s16 operation, legacy_s16 recorded, legacy_s16 current)
{
	trace(18);
	trace(operation);
	trace(recorded);
	trace(current);
}
void update_frame(legacy_s8 buffer, struct RECTANGLE *rect)
{
	trace(19);
	trace(buffer);
	trace_rect(rect);
	trace(state.game_frame);
	trace(gameconfig.game_recordedframes);
	assert(++frames <= 3);
	elapsed_time2 = ++state.game_frame;
	if (frames == 3) {
		race_exit_request = REPLAY_EXIT_REQUESTED;
	}
}
void rect_union(struct RECTANGLE *first, struct RECTANGLE *second, struct RECTANGLE *result)
{
	assert(first == result);
	trace(20);
	trace_rect(first);
	trace_rect(second);
}
void shape2d_render_bmp_as_mask(struct SHAPE2D far *shape)
{
	(void)shape;
	trace(21);
}
void shape2d_rle_or_far_pointer(legacy_u16 offset, legacy_u16 segment)
{
	trace(22);
	trace(offset);
	trace(segment);
}
void frame_present(struct RECTANGLE *rect)
{
	trace(23);
	trace_rect(rect);
}
void mouse_draw_opaque_check(void)
{
	trace(24);
}
void mouse_draw_transparent_check(void)
{
	trace(25);
}
void sprite_present_mcga_backbuffer(void)
{
	trace(26);
}
void init_game_state_with_frame_rate(legacy_u16 rate)
{
	trace(27);
	trace(rate);
}
void mouse_minmax_position(legacy_s16 inset)
{
	trace(28);
	trace(inset);
}
void audio_carstate(void)
{
	trace(29);
}
legacy_s16 dos_kb_get_char(void)
{
	trace(30);
	keys++;
	if (idle_expired) {
		return scenario & 1 ? 27 : 0;
	}
	return keys % 3 == 1 ? KEY_LEFT : 0;
}
legacy_s16 handle_ingame_kb_shortcuts(legacy_s16 key)
{
	trace(31);
	trace(key);
	return 0;
}
void dos_mouse_get_state(legacy_s16 *buttons, legacy_s16 *x, legacy_s16 *y)
{
	trace(32);
	*buttons = scenario & 1;
	*x = 90;
	*y = 150;
}
legacy_s16 get_kb_or_joy_flags(void)
{
	trace(33);
	return scenario & 2 ? INPUT_ACTION_BUTTON_MASK : 0;
}

int main(void)
{
	struct RACE_VIEWPORT_CACHE cache;
	for (scenario = 0; scenario < 192; scenario++) {
		trace(scenario);
		memset(&state, 0, sizeof(state));
		memset(&gameconfig, 0, sizeof(gameconfig));
		memset(&rect_windshield, 0, sizeof(rect_windshield));
		memset(&dirty_rect, 0, sizeof(dirty_rect));
		cache.roof_height = cache.dashboard_bottom = -1;
		frames = keys = 0;
		state.game_frame = 10;
		elapsed_time2 = (scenario & 1) ? 11 : 10;
		gameconfig.game_recordedframes = 40;
		state.game_inputmode = (scenario & 2) ? GAME_INPUT_MODE_ACTIVE : GAME_INPUT_MODE_WAITING;
		game_replay_mode = (scenario % 3) == 0	 ? REPLAY_MODE_LIVE
						   : (scenario % 3) == 1 ? REPLAY_MODE_PLAYBACK
												 : REPLAY_MODE_PAUSED;
		game_replay_mode_copy = -1;
		race_exit_request = scenario & 4 ? 1 : 0;
		race_start_sequence_state =
			scenario & 8 ? RACE_START_SEQUENCE_INACTIVE : RACE_START_SEQUENCE_FLAG_ANIMATION;
		idle_expired = scenario & 16;
		video_uses_page_flipping = scenario & 32;
		video_page_count = 2;
		slow_video_mgmt = scenario & 64;
		slow_video_mgmt_copy = -1;
		mouse_driving_enabled = scenario & 1;
		recording_limit_warning_requested = scenario & 128;
		frame_buffer_index = dashboard_buffer_index = 0;
		dashb_toggle = scenario & 2;
		replaybar_toggle = scenario & 4;
		is_in_replay = scenario & 8;
		followOpponentFlag = scenario & 1;
		dashbmp_y = 140;
		roofbmpheight = 12;
		dastbmp_y = 130;
		full_redraw_frames_remaining = 1;
		height_above_replaybar = 200;
		viewport_bottom_cache = -1;
		active_frame_rects = &dirty_rect;
		configured_frame_rate = 20;
		race_run_frames(&cache);
		trace(frames);
		trace(keys);
		trace(game_replay_mode);
		trace(race_exit_request);
		trace(state.game_frame);
		trace(elapsed_time2);
		trace(gameconfig.game_recordedframes);
		trace(frame_buffer_index);
		trace(dashboard_buffer_index);
		trace(full_redraw_frames_remaining);
		trace(recording_limit_warning_requested);
		trace(race_start_sequence_state);
		trace(is_in_replay);
	}
#ifdef RACE_FRAMES_RECORD_BASELINE
	printf("Race frame fingerprint: %08x\n", (unsigned)trace_hash);
#else
	assert(trace_hash == UINT32_C(0xe6335ceb));
#endif
	return 0;
}
