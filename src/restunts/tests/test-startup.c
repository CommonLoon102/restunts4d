/* Startup timing, option handling, menu actions and race cleanup use traces
 * captured from the pre-refactor implementation with stubbed DOS endpoints. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include "../c/restunts.c"
#undef printf
#undef strlen
#undef strcpy
#undef memcpy
static uint32_t trace_hash = UINT32_C(2166136261);
static unsigned timer_calls, status_calls;
static legacy_u32 geometry_ticks, clear_ticks, partial_ticks;
static legacy_s16 audio_failure;
static jmp_buf exit_jump;

static void trace(legacy_u32 value)
{
	for (unsigned i = 0; i < 4; i++) {
		trace_hash = (trace_hash ^ (value & 255U)) * UINT32_C(16777619);
		value >>= 8;
	}
}
void kb_init_interrupt(void)
{
	trace(1);
}
void dos_kb_clear_numlock(void)
{
	trace(2);
}
legacy_s16 kb_call_readchar_callback(void)
{
	trace(3);
	return 0;
}
void kb_reg_callback(legacy_s16 code, void(far *callback)(void))
{
	void (*callbacks[])(void) = {show_graphic_levels_menu,	calibrate_joystick_driving,
								 select_keyboard_driving,	toggle_music_with_dialog,
								 show_pause_dialog,			show_exit_to_dos_dialog,
								 toggle_effects_with_dialog};
	trace(4);
	trace(code);
	for (unsigned i = 0; i < 7; i++) {
		if (callback == callbacks[i]) {
			trace(i);
			return;
		}
	}
	assert(0);
}
void show_graphic_levels_menu(void)
{
	assert(0);
}
void calibrate_joystick_driving(void)
{
	assert(0);
}
void select_keyboard_driving(void)
{
	assert(0);
}
void toggle_music_with_dialog(void)
{
	assert(0);
}
void show_pause_dialog(void)
{
	assert(0);
}
void show_exit_to_dos_dialog(void)
{
	assert(0);
}
void toggle_effects_with_dialog(void)
{
	assert(0);
}
void init_video_geometry_flags(void)
{
	trace(5);
}
void mmgr_init_conventional_arena(void)
{
	trace(6);
}
void audio_allocate_car_state_records(void)
{
	trace(7);
}
void dos_video_set_mode_13h(void)
{
	trace(8);
}
void dos_video_set_mode4(void)
{
	trace(9);
}
void dos_timer_setup_interrupt(void)
{
	trace(10);
}
void sprite_select_screen_and_clear(void)
{
	trace(11);
}
legacy_s16 dos_mouse_init(legacy_s16 width, legacy_s16 height)
{
	trace(12);
	trace(width);
	trace(height);
	return 0;
}
legacy_s16 audio_load_dos_driver(const legacy_s8 *name, legacy_s16 a, legacy_s16 b)
{
	trace(13);
	trace(name[0]);
	trace(name[1]);
	trace(a);
	trace(b);
	return audio_failure;
}
void dos_timer_shutdown(void)
{
	trace(14);
}
void dos_process_exit(legacy_s16 status)
{
	trace(15);
	trace(status);
	longjmp(exit_jump, 1);
}
legacy_s16 audio_toggle_music(void)
{
	trace(16);
	return 0;
}
legacy_s16 audio_toggle_effects(void)
{
	trace(17);
	return 0;
}
legacy_s16 show_disk_error_dialog(void)
{
	assert(0);
	return 0;
}
void dos_set_critical_error_handler(legacy_s16(far *callback)(void))
{
	assert(callback == show_disk_error_dialog);
	trace(18);
}
void load_palandcursor(void)
{
	trace(19);
}
void sprite_select_screen(void)
{
	trace(20);
}
void sprite_set_target_clip_bounds(legacy_u16 left, legacy_u16 right, legacy_u16 top,
								   legacy_u16 bottom)
{
	trace(21);
	trace(left);
	trace(right);
	trace(top);
	trace(bottom);
}
void sprite_clear_target(legacy_u8 color)
{
	trace(22);
	trace(color);
}
legacy_u32 timer_get_delta_alt(void)
{
	trace(23);
	switch (timer_calls++) {
		case 0:
			return 99;
		case 1:
			return clear_ticks;
		case 2:
			return partial_ticks;
		default:
			assert(timer_calls == 4);
			return geometry_ticks;
	}
}
legacy_s16 dos_video_get_status(void)
{
	trace(24);
	return status_calls++ & 1;
}
legacy_s16 _rand(void)
{
	trace(25);
	return 19;
}
legacy_s16 get_kevinrandom(void)
{
	trace(26);
	return 21;
}
legacy_u16 _strlen(const legacy_s8 *text)
{
	return (legacy_u16)strlen((const char *)text);
}

static unsigned menu_scenario, menu_calls, intro_calls, game_calls, score_calls;
static legacy_u8 menu_track_data[REPLAY_TRACK_SIZE];
static legacy_s8 backup_memory[REPLAY_TRACK_SIZE + 162];
static legacy_s8 menu_resource[64];
static legacy_u8 checkpoint_memory[32];

static void trace_text(const legacy_s8 *text)
{
	while (*text) {
		trace((legacy_u8)*text++);
	}
	trace(0);
}
legacy_s8 *_strcpy(legacy_s8 *destination, const legacy_s8 *source)
{
	return (legacy_s8 *)strcpy((char *)destination, (const char *)source);
}
void *_memcpy(void *destination, const void *source, legacy_u16 count)
{
	return memcpy(destination, source, count);
}
void dos_install_divide_error_handler(void)
{
	trace(30);
}
void init_row_tables(void)
{
	trace(31);
}
void far *file_load_resfile(const legacy_s8 *name)
{
	trace(32);
	trace_text(name);
	return menu_resource;
}
void far *file_load_resource(legacy_s16 type, const legacy_s8 *name)
{
	trace(33);
	trace(type);
	trace_text(name);
	return menu_resource;
}
void font_set_fontdef(void)
{
	trace(34);
}
void init_polyinfo(void)
{
	trace(35);
}
void init_trackdata(void)
{
	trace(36);
}
void reset_race_loop_state(void)
{
	trace(37);
}
void init_kevinrandom(const legacy_s8 *seed)
{
	trace(38);
	trace_text(seed);
}
legacy_s16 input_do_checking(legacy_s16 ticks)
{
	trace(39);
	trace(ticks);
	return 0;
}
void mouse_draw_opaque_check(void)
{
	trace(40);
}
void ensure_file_exists(legacy_s16 disk)
{
	trace(41);
	trace(disk);
}
void file_build_path(const legacy_s8 *directory, const legacy_s8 *name, const legacy_s8 *ext,
					 legacy_s8 *destination)
{
	trace(42);
	trace_text(directory);
	trace_text(name);
	trace_text(ext);
	strcpy((char *)destination, "fixture.trk");
}
void far *file_read_fatal(const legacy_s8 *name, void far *destination)
{
	trace(43);
	trace_text(name);
	assert(destination == menu_track_data);
	return destination;
}
legacy_s16 run_intro_looped(void)
{
	trace(44);
	assert(intro_calls < 3);
	return ++intro_calls == 1 ? 0 : 27;
}
legacy_s8 far *locate_text_res(legacy_s8 far *resource, const legacy_s8 *name)
{
	assert(resource == menu_resource);
	trace(45);
	trace_text(name);
	return menu_resource;
}
legacy_u16 show_dialog(legacy_s16 type, legacy_s16 save, void far *text, legacy_u16 x, legacy_u16 y,
					   legacy_s16 color, legacy_s16 *flags, legacy_s16 selected)
{
	assert(text == menu_resource);
	assert(flags == 0);
	trace(46);
	trace(type);
	trace(save);
	trace(x);
	trace(y);
	trace(color);
	trace(selected);
	return 1;
}
void dos_audio_shutdown(void)
{
	trace(47);
}
void kb_exit_handler(void)
{
	trace(48);
}
void dos_kb_set_numlock(void)
{
	trace(49);
}
void dos_video_set_mode7(void)
{
	trace(50);
}
void file_load_audiores(const legacy_s8 *song, const legacy_s8 *voice, const legacy_s8 *id)
{
	trace(51);
	trace_text(song);
	trace_text(voice);
	trace_text(id);
	is_audioloaded = 1;
}
legacy_s8 run_menu(void)
{
	unsigned call = menu_calls++;
	trace(52);
	assert(menu_calls < 10);
	if (menu_scenario & 1) {
		idle_expired = 1;
	}
	static const legacy_s8 actions[] = {1, 2, 3, 4, 7, 0, -1};
	return actions[call];
}
void audio_unload(void)
{
	trace(53);
	is_audioloaded = 0;
}
void check_input(void)
{
	trace(54);
}
void show_waiting(void)
{
	trace(55);
}
void run_car_menu(legacy_s8 *id, legacy_s8 *material, legacy_s8 *transmission, legacy_u16 opponent)
{
	trace(56);
	trace(id[0]);
	trace(*material);
	trace(*transmission);
	trace(opponent);
	*material = 3;
}
void run_opponent_menu(void)
{
	trace(57);
	gameconfig.game_opponenttype = 2;
}
void run_tracks_menu(legacy_s16 reload)
{
	trace(58);
	trace(reload);
}
legacy_u16 run_option_menu(void)
{
	trace(59);
	return menu_scenario & 2;
}
legacy_s16 track_setup(void)
{
	trace(60);
	return menu_scenario & 4;
}
const legacy_s8 *file_find(const legacy_s8 *query)
{
	trace(61);
	trace_text(query);
	return menu_scenario & 4 ? 0 : (legacy_s8 *)"tedit.trk";
}
void fatal_error(const legacy_s8 *format, ...)
{
	(void)format;
	assert(0);
}
void far *mmgr_alloc_resbytes(const legacy_s8 *name, legacy_s32 size)
{
	trace(62);
	trace_text(name);
	trace(size);
	return checkpoint_memory;
}
void init_game_state(legacy_s16 mode)
{
	trace(63);
	trace(mode);
}
void run_game(void)
{
	trace(64);
	trace(gameconfig.game_recordedframes);
	trace(replay_recording_flags);
	assert(++game_calls < 8);
	gameconfig.game_recordedframes = 77;
	replay_recording_flags = menu_scenario & 8 ? 0 : 1;
	menu_track_data[0] = 222;
	track_directory[0] = 'X';
	replay_directory[0] = 'Y';
}
legacy_u16 end_hiscore(void)
{
	trace(65);
	return score_calls++ < 2 ? score_calls - 1 : 2;
}
void mmgr_release(void far *pointer)
{
	assert(pointer == checkpoint_memory);
	trace(66);
}

static void test_menu_lifecycle(void)
{
	legacy_s8 *arguments[] = {(legacy_s8 *)"game"};
	for (menu_scenario = 0; menu_scenario < 16; menu_scenario++) {
		trace(1000 + menu_scenario);
		memset(&gameconfig, 0, sizeof(gameconfig));
		for (unsigned i = 0; i < REPLAY_TRACK_SIZE; i++) {
			menu_track_data[i] = (legacy_u8)i;
		}
		track_element_map = menu_track_data;
		track_and_directory_backup = backup_memory;
		memset(track_directory, 0, 81);
		memset(replay_directory, 0, 81);
		track_directory[0] = 'A';
		replay_directory[0] = 'B';
		menu_calls = intro_calls = game_calls = score_calls = 0;
		timer_calls = status_calls = 0;
		audio_failure = 0;
		is_audioloaded = 0;
		replay_recording_flags = 0;
		geometry_ticks = 55;
		clear_ticks = 15;
		partial_ticks = 16;
		trace(run_main_menu_loop(1, arguments));
		trace(menu_calls);
		trace(intro_calls);
		trace(game_calls);
		trace(score_calls);
		for (unsigned i = 0; i < REPLAY_TRACK_SIZE; i++) {
			trace(menu_track_data[i]);
		}
		trace(track_directory[0]);
		trace(replay_directory[0]);
		trace(gameconfig.game_recordedframes);
		trace(gameconfigcopy.game_recordedframes);
	}
}

int main(void)
{
	static legacy_s8 *arguments[][8] = {
		{(legacy_s8 *)"game"},
		{(legacy_s8 *)"game", (legacy_s8 *)"/h", (legacy_s8 *)"/ns", (legacy_s8 *)"/nd"},
		{(legacy_s8 *)"game", (legacy_s8 *)"/sSB", (legacy_s8 *)"/unknown"},
		{(legacy_s8 *)"game", (legacy_s8 *)"/ssb", (legacy_s8 *)"/sAD", (legacy_s8 *)"/s"},
		{(legacy_s8 *)"game", (legacy_s8 *)"/sxy", (legacy_s8 *)"/sSb", (legacy_s8 *)"ignore"},
	};
	static legacy_u32 boundaries[] = {0, 34, 35, 54, 55, 74, 75, 99, 100, 65535};
	static unsigned scenario;
	static legacy_s16 counts[] = {1, 4, 3, 4, 4};
	for (scenario = 0; scenario < 110; scenario++) {
		trace(scenario);
		geometry_ticks = boundaries[scenario % 10];
		clear_ticks = 15;
		partial_ticks = (scenario & 1) ? 15 : 16;
		audio_failure = scenario >= 100;
		timer_calls = status_calls = 0;
		audiodriverstring[0] = 's';
		audiodriverstring[1] = 't';
		detail_level = slow_video_mgmt = -7;
		if (setjmp(exit_jump) == 0) {
			init_main(counts[scenario % 5], arguments[scenario % 5]);
			trace(27);
		}
		trace(timer_calls);
		trace(status_calls);
		trace(detail_level);
		trace(slow_video_mgmt);
		if (!audio_failure) {
			trace(framespersec);
			trace(configured_frame_rate);
			trace(slow_video_mgmt_copy);
			trace(video_uses_page_flipping);
			trace(video_page_count);
			trace(textresprefix);
		}
	}
	test_menu_lifecycle();
#ifdef STARTUP_RECORD_BASELINE
	printf("Startup fingerprint: %08x\n", (unsigned)trace_hash);
#else
	assert(trace_hash == UINT32_C(0x00524607));
#endif
	return 0;
}
