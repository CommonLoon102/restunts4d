/* Regression fingerprints captured from the original routines before extraction. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef VIEWER_SOURCE
#define VIEWER_SOURCE "../c/replay_viewer.c"
#endif
#include VIEWER_SOURCE
#undef memcpy
#undef printf

static legacy_u32 trace_hash;
static unsigned scenario, dialog_count, save_count, write_count, check_count;
static legacy_s16 menu_action;
static struct SHAPE2D shapes[23];
static legacy_u8 track_bytes[901];

static void hash_word(legacy_u16 value)
{
	trace_hash = (trace_hash ^ value) * 16777619UL;
}
static void event(legacy_u16 id)
{
	hash_word(id);
	hash_word(is_in_replay);
	hash_word(game_replay_mode);
	hash_word(replay_recording_flags);
	hash_word(elapsed_time2);
	hash_word(gameconfig.game_recordedframes);
	hash_word(g_is_busy);
}
void mouse_draw_opaque_check(void)
{
	event(1);
}
void mouse_draw_transparent_check(void)
{
	event(2);
}
void shape2d_rle_copy_at_position(struct SHAPE2D far *shape)
{
	event(3);
	hash_word((legacy_u16)(shape - shapes));
}
void format_frame_as_string(legacy_s8 *text, legacy_s16 frame, legacy_s16 fraction)
{
	event(4);
	hash_word(frame);
	hash_word(fraction);
	text[0] = '0';
	text[1] = 0;
}
void font_set_colors(legacy_s16 color, legacy_s16 background)
{
	event(5);
	hash_word(color);
	hash_word(background);
}
void font_set_fontdef2(void far *data)
{
	(void)data;
	event(6);
}
void font_set_fontdef(void)
{
	event(7);
}
void font_draw_text_opaque(const legacy_s8 *text, legacy_s16 x, legacy_s16 y)
{
	event(8);
	hash_word(text[0]);
	hash_word(x);
	hash_word(y);
}
void sprite_fill_rect(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
					  legacy_s16 color)
{
	event(9);
	hash_word(x);
	hash_word(y);
	hash_word(width);
	hash_word(height);
	hash_word(color);
}
void sprite_draw_rect_outline(legacy_s16 x1, legacy_s16 y1, legacy_s16 x2, legacy_s16 y2,
							  legacy_s16 color)
{
	event(10);
	hash_word(x1);
	hash_word(y1);
	hash_word(x2);
	hash_word(y2);
	hash_word(color);
}
void audio_carstate(void)
{
	event(11);
}
void check_input(void)
{
	event(12);
	check_count++;
}
void mouse_minmax_position(legacy_s16 enabled)
{
	event(13);
	hash_word(enabled);
}
void init_game_state_with_frame_rate_byte(legacy_u16 rate)
{
	event(14);
	hash_word(rate);
	state.game_frame = 0;
}
void update_crash_state(legacy_s16 crash, legacy_s16 car)
{
	event(15);
	hash_word(crash);
	hash_word(car);
	state.game_end_event = crash;
}
legacy_s8 far *locate_text_res(legacy_s8 far *resource, const legacy_s8 *name)
{
	(void)resource;
	event(16);
	hash_word(name[0]);
	hash_word(name[1]);
	hash_word(name[2]);
	return (legacy_s8 *)name;
}
legacy_u16 show_dialog(legacy_s16 type, legacy_s16 save, void far *text, legacy_u16 x, legacy_u16 y,
					   legacy_s16 color, legacy_s16 *disabled, legacy_s16 initial)
{
	unsigned index, count = text == replay_pause_menu_id ? 8 : 5;
	(void)text;
	event(17);
	hash_word(type);
	hash_word(save);
	hash_word(x);
	hash_word(y);
	hash_word(color);
	hash_word(initial);
	if (disabled != 0) {
		for (index = 0; index < count; index++) {
			hash_word(disabled[index]);
		}
	}
	dialog_count++;
	if (text == replay_pause_menu_id) {
		return menu_action;
	}
	if (menu_action == REPLAY_PAUSE_ACTION_DISPLAY_OPTIONS) {
		return scenario % 7 - 1;
	}
	return (scenario / 4) % 4 - 1;
}
legacy_s8 do_fileselect_dialog(legacy_s8 *directory, legacy_s8 *filename, legacy_s8 *extension,
							   legacy_s8 far *prompt)
{
	(void)directory;
	(void)filename;
	(void)extension;
	(void)prompt;
	event(18);
	return scenario & 1;
}
legacy_s16 do_savefile_dialog(legacy_s8 *directory, legacy_s8 *filename, legacy_s8 far *prompt)
{
	(void)directory;
	(void)filename;
	(void)prompt;
	event(19);
	return save_count++ == 0 && (scenario & 1);
}
void file_build_path(const legacy_s8 *directory, const legacy_s8 *name, const legacy_s8 *ext,
					 legacy_s8 *path)
{
	(void)directory;
	(void)name;
	(void)ext;
	event(20);
	path[0] = 'x';
	path[1] = 0;
}
const legacy_s8 *file_find(const legacy_s8 *query)
{
	event(21);
	return scenario & 2 ? query : 0;
}
legacy_s16 file_write_replay(const legacy_s8 *path)
{
	(void)path;
	event(22);
	write_count++;
	return (scenario >> 2) & 1;
}
legacy_s16 file_load_replay(const legacy_s8 *directory, const legacy_s8 *name)
{
	(void)directory;
	(void)name;
	event(23);
	if (scenario & 2) {
		gameconfig.game_playercarid[3] ^= 1;
	}
	if (scenario & 4) {
		gameconfig.game_opponenttype ^= 1;
	}
	if (scenario & 8) {
		gameconfig.game_opponentcarid[3] ^= 1;
	}
	gameconfig.game_framespersec = (scenario & 16) ? 0x128a : 20;
	return (scenario >> 5) & 1;
}
void show_waiting(void)
{
	event(24);
}
legacy_s16 track_setup(void)
{
	event(25);
	if (scenario & 64) {
		track_bytes[900] ^= 1;
	}
	return 0;
}
void ensure_file_exists(legacy_s16 argument)
{
	event(26);
	hash_word(argument);
}
void load_opponent_data(void)
{
	event(27);
}
void free_player_cars(void)
{
	event(28);
}
legacy_s16 setup_player_cars(void)
{
	event(29);
	return 0;
}
void init_game_state(legacy_s16 mode)
{
	event(30);
	hash_word(mode);
	hash_word(framespersec);
}
void show_graphic_levels_menu(void)
{
	event(31);
}

static void reset_viewer(void)
{
	unsigned index;
	memset(&state, 0, sizeof(state));
	memset(&gameconfig, 0, sizeof(gameconfig));
	memset(replay_controls_drawn, 0, 2 * sizeof(replay_controls_drawn[0]));
	memset(replay_control_active, 0, sizeof(replay_control_active));
	memset(replay_control_active_cache, 0, sizeof(replay_control_active_cache));
	memset(replay_camera_mode_cache, 0, sizeof(replay_camera_mode_cache));
	memset(replay_selection_cache, 0, sizeof(replay_selection_cache));
	memset(replay_displayed_time_cache, 0, sizeof(replay_displayed_time_cache));
	memset(replay_recorded_position_cache, 0, sizeof(replay_recorded_position_cache));
	memset(replay_current_position_cache, 0, sizeof(replay_current_position_cache));
	for (index = 0; index < 23; index++) {
		rplyshapes[index] = &shapes[index];
	}
	dialog_count = save_count = write_count = check_count = 0;
	track_element_map = track_bytes;
	memset(track_bytes, 0, sizeof(track_bytes));
	game_replay_mode = REPLAY_MODE_PAUSED;
	is_in_replay = 0;
	dashboard_buffer_index = 0;
	cameramode = CAMERA_MODE_CUSTOM;
	replay_selected_control = REPLAY_CONTROL_PLAY;
	elapsed_time1 = 0;
	elapsed_time2 = 100;
	gameconfig.game_recordedframes = 100;
	gameconfig.game_framespersec = 20;
	state.game_frame = 60;
	state.game_frame_in_sec = 3;
	state.game_end_event = 2;
	dashb_toggle = 0;
	show_penalty_counter = 1;
	followOpponentFlag = 1;
	replay_playback_speed = REPLAY_PLAYBACK_FAST;
	replaybar_toggle = 1;
	race_exit_request = 0;
	g_is_busy = 0;
	kbormouse = 1;
	mouse_driving_enabled = 1;
	passed_security = 1;
	replay_recording_flags = REPLAY_RECORDING_ACTIVE_FLAG;
	replay_overflow_acknowledged_word = 0x5a01;
	waitflag = 0;
	framespersec = 20;
	full_redraw_frames_remaining = 0;
	video_page_count = 2;
	configured_frame_rate = 20;
}
static void hash_viewer_state(void)
{
	unsigned index;
	event(100);
	hash_word(state.game_frame);
	hash_word(state.game_frame_in_sec);
	hash_word(state.game_end_event);
	hash_word(dashb_toggle);
	hash_word(show_penalty_counter);
	hash_word(followOpponentFlag);
	hash_word(cameramode);
	hash_word(replay_playback_speed);
	hash_word(race_exit_request);
	hash_word(replay_overflow_acknowledged_word);
	hash_word(waitflag);
	hash_word(framespersec);
	hash_word(full_redraw_frames_remaining);
	hash_word(kbormouse);
	hash_word(dialog_count);
	hash_word(save_count);
	hash_word(write_count);
	hash_word(replaybar_toggle);
	hash_word(replay_selected_control);
	for (index = 0; index < 2; index++) {
		hash_word(replay_controls_drawn[index]);
		hash_word(replay_camera_mode_cache[index]);
		hash_word(replay_selection_cache[index]);
		hash_word(replay_displayed_time_cache[index]);
		hash_word(replay_recorded_position_cache[index]);
		hash_word(replay_current_position_cache[index]);
	}
	for (index = 0; index < 18; index++) {
		hash_word(replay_control_active_cache[index]);
	}
	for (index = 0; index < 9; index++) {
		hash_word(replay_control_active[index]);
	}
}
static legacy_u32 menu_fingerprint(void)
{
	trace_hash = 2166136261UL;
	for (menu_action = -1; menu_action <= 7; menu_action++) {
		for (scenario = 0; scenario < 256; scenario++) {
			reset_viewer();
			replay_recording_flags = scenario & 15;
			passed_security = (scenario >> 4) & 1;
			state.playerstate.car_crashBmpFlag = (scenario >> 5) & 1;
			gameconfig.game_opponenttype = (scenario >> 6) & 1;
			if (scenario & 128) {
				gameconfig.game_recordedframes = 0;
				elapsed_time1 = 10;
			}
			replay_pause_menu();
			hash_viewer_state();
			assert(g_is_busy == 0);
		}
	}
	return trace_hash;
}
static legacy_u32 draw_fingerprint(void)
{
	unsigned index, tick;
	trace_hash = 2166136261UL;
	for (index = 0; index < 256; index++) {
		reset_viewer();
		gameconfig.game_recordedframes = index % 3 ? 65535 : 0;
		elapsed_time1 = index * 257;
		for (tick = 0; tick < 8; tick++) {
			dashboard_buffer_index = tick & 1;
			cameramode = (index + tick / 2) & 3;
			replay_selected_control = tick % 3 ? tick % 9 : REPLAY_NO_SELECTION;
			replay_control_active[tick % 7] ^= 1;
			replay_controls_draw(index * 257 + tick, tick * 8192);
			hash_viewer_state();
			replay_controls_draw(index * 257 + tick, tick * 8192);
			hash_viewer_state();
		}
	}
	return trace_hash;
}
static void test_pause_cleanup(void)
{
	reset_viewer();
	menu_action = REPLAY_PAUSE_ACTION_CONTINUE;
	scenario = 0;
	elapsed_time2 = 20;
	replay_pause_menu();
	assert(game_replay_mode == REPLAY_MODE_PAUSED &&
		   replay_recording_flags == REPLAY_RECORDING_ACTIVE_FLAG);
	assert(elapsed_time2 == 20 && gameconfig.game_recordedframes == 100 && check_count == 1);
	reset_viewer();
	menu_action = REPLAY_PAUSE_ACTION_CONTINUE;
	scenario = 8;
	elapsed_time2 = 20;
	replay_pause_menu();
	assert(game_replay_mode == REPLAY_MODE_LIVE && is_in_replay == 0 && check_count == 2);
	assert(replay_recording_flags ==
		   (REPLAY_RECORDING_ACTIVE_FLAG | REPLAY_RECORDING_MODIFIED_FLAG));
	assert(elapsed_time2 == 60 && gameconfig.game_recordedframes == 60);
	assert(cameramode == CAMERA_MODE_COCKPIT && followOpponentFlag == 0 &&
		   show_penalty_counter == 0);
	reset_viewer();
	menu_action = REPLAY_PAUSE_ACTION_RESTART;
	scenario = 0;
	replay_pause_menu();
	assert(game_replay_mode == REPLAY_MODE_LIVE && check_count == 3);
	assert(elapsed_time2 == 0 && gameconfig.game_recordedframes == 0);
	assert(replay_overflow_acknowledged_word == 0x5a00);
}

static void test_save_cleanup(void)
{
	reset_viewer();
	menu_action = REPLAY_PAUSE_ACTION_SAVE;
	scenario = 1;
	replay_pause_menu();
	assert(save_count == 1 && write_count == 1 && check_count == 1 && g_is_busy == 0);
	reset_viewer();
	menu_action = REPLAY_PAUSE_ACTION_SAVE;
	scenario = 0;
	replay_pause_menu();
	assert(save_count == 1 && write_count == 0 && check_count == 1 && g_is_busy == 0);
	reset_viewer();
	menu_action = REPLAY_PAUSE_ACTION_SAVE;
	scenario = 3;
	replay_pause_menu();
	assert(save_count == 1 && write_count == 0 && dialog_count == 4 && g_is_busy == 0);
	reset_viewer();
	menu_action = REPLAY_PAUSE_ACTION_SAVE;
	scenario = 5;
	replay_pause_menu();
	assert(save_count == 2 && write_count == 1 && dialog_count == 3 && g_is_busy == 0);
}

int main(void)
{
	legacy_u32 menu = menu_fingerprint(), draw = draw_fingerprint();
	test_pause_cleanup();
	test_save_cleanup();
#ifdef REPLAY_MENU_BASELINE
	printf("%08lx %08lx\n", (unsigned long)menu, (unsigned long)draw);
#else
	assert(menu == 0x393def8eUL);
	assert(draw == 0x9b2a836aUL);
#endif
	return 0;
}
