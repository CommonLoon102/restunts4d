/* Regression fingerprints captured from the original routines before extraction. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef INPUT_SOURCE
#define INPUT_SOURCE "../c/game_input.c"
#endif
#ifndef RECORD_SOURCE
#define RECORD_SOURCE "../c/replay_record.c"
#endif
#include INPUT_SOURCE
#include RECORD_SOURCE
#undef memcpy

static legacy_u32 trace_hash, random_state = 1;
static legacy_u16 keyboard_char, joystick_flags;
static legacy_s16 key_states[128], mouse_samples[4][3], joystick_axis;
static unsigned mouse_sample_index;
static legacy_u8 joystick_enabled, segments_match, nested_callback;
static legacy_s8 replay_bytes[12000], response_table[256];
static struct GAMESTATE snapshots[41];
static struct AUDIO_CAR_STATE audio_samples[AUDIO_CAR_STATE_RECORD_COUNT];

static void hash_word(legacy_u16 value)
{
	trace_hash = (trace_hash ^ value) * 16777619UL;
}

static legacy_u16 random_word(void)
{
	random_state = random_state * 1664525UL + 1013904223UL;
	return (legacy_u16)(random_state >> 16);
}

legacy_s16 dos_kb_get_char(void)
{
	hash_word(1);
	return keyboard_char;
}
legacy_s16 kb_read_char(void)
{
	return keyboard_char;
}
legacy_s16 kb_get_key_state(legacy_s16 key)
{
	hash_word(0x100 | key);
	return key_states[key];
}
legacy_s16 dos_get_joy_flags(void)
{
	hash_word(2);
	return joystick_flags;
}
legacy_u8 dos_joystick_is_enabled(void)
{
	hash_word(3);
	return joystick_enabled;
}
legacy_s16 dos_joystick_get_scaled_axis(legacy_u16 axis)
{
	hash_word(0x200 | axis);
	return joystick_axis;
}
legacy_s16 dos_data_stack_segments_match(void)
{
	hash_word(4);
	return segments_match;
}
void dos_mouse_get_state(legacy_s16 *buttons, legacy_s16 *x, legacy_s16 *y)
{
	unsigned index = mouse_sample_index;
	hash_word(5);
	*buttons = mouse_samples[index][0];
	*x = mouse_samples[index][1];
	*y = mouse_samples[index][2];
	if (mouse_sample_index < 3) {
		mouse_sample_index++;
	}
}
void mouse_draw_opaque(void)
{
	hash_word(6);
}
void mouse_draw_transparent(void)
{
	hash_word(7);
}
void dos_mouse_set_minmax(legacy_s16 x1, legacy_s16 y1, legacy_s16 x2, legacy_s16 y2)
{
	hash_word(8);
	hash_word(x1);
	hash_word(y1);
	hash_word(x2);
	hash_word(y2);
}
void dos_mouse_set_position(legacy_s16 x, legacy_s16 y)
{
	hash_word(9);
	hash_word(x);
	hash_word(y);
}
void select_mouse_driving(void)
{
	hash_word(10);
	mouse_driving_enabled ^= 1;
}
void init_game_state_with_frame_rate_byte(legacy_u16 rate)
{
	hash_word(11);
	hash_word(rate);
}
void update_crash_state(legacy_s16 event, legacy_s16 car)
{
	hash_word(12);
	hash_word(event);
	hash_word(car);
	state.game_end_event = event;
}
void audio_carstate(void)
{
	hash_word(13);
	hash_word(is_in_replay);
}
void audio_apply_car_state_sample(const legacy_u8 far *sample, legacy_s16 interval)
{
	hash_word(14);
	hash_word((legacy_u16)(sample - (const legacy_u8 *)audio_samples));
	hash_word(interval);
	if (nested_callback != 0) {
		nested_callback = 0;
		frame_callback();
	}
}
legacy_u32 timer_get_delta_alt(void)
{
	hash_word(15);
	return 3;
}
void sprite_fill_rect(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height,
					  legacy_s16 color)
{
	hash_word(16);
	hash_word(x);
	hash_word(y);
	hash_word(width);
	hash_word(height);
	hash_word(color);
}

void far *__fmemcpy(void far *destination, const void far *source, legacy_u16 count)
{
	return memcpy(destination, source, count);
}

static void reset_inputs(void)
{
	memset(&state, 0, sizeof(state));
	memset(&gameconfig, 0, sizeof(gameconfig));
	memset(key_states, 0, sizeof(key_states));
	memset(mouse_samples, 0, sizeof(mouse_samples));
	keyboard_char = joystick_flags = 0;
	mouse_sample_index = 0;
	joystick_axis = 0;
	joystick_enabled = 0;
	segments_match = 1;
	nested_callback = 0;
	mouse_xpos = mouse_ypos = mouse_butstate = 0;
	mouse_driving_enabled = 0;
	input_elapsed_frames = input_mouse_repeat_at = input_joystick_repeat_at = 0;
	input_mouse_idle_frames = input_combined_flags = input_joystick_flags = 0;
	input_new_joystick_flags = input_joystick_keycode = input_mouse_keycode = 0;
	input_mouse_previous_x = input_mouse_previous_y = input_mouse_previous_buttons = 0;
	mouse_transparent_mode = mouse_background_dirty = kbormouse = h_key_toggle = 0;
	frame_callback_active = 0;
	frame_callback_count = 0;
	audio_car_state_interval = 0;
	audio_car_state_read_index = audio_car_state_write_index = 0;
	audio_car_state_records = audio_samples;
	audio_previous_replay_mode = 255;
	frame_callback_countdown = 1;
	slow_replay_countdown = 1;
	timer_ticks_per_frame = 6;
	framespersec = 20;
	game_replay_mode = REPLAY_MODE_LIVE;
	is_in_replay = 0;
	race_exit_request = recording_limit_warning_requested = 0;
	passed_security = 1;
	race_start_sequence_state = RACE_START_SEQUENCE_INACTIVE;
	elapsed_time1 = elapsed_time2 = 0;
	replay_overflow_acknowledged_word = 0;
	replay_playback_speed = REPLAY_PLAYBACK_NORMAL;
	replay_recording_flags = 0;
	replay_input_buffer = replay_bytes;
	cvxptr = snapshots;
	steerWhlRespTable_ptr = response_table;
	memset(input_steering_history, 0, sizeof(input_steering_history));
	memset(input_steering_history_valid, 0, sizeof(input_steering_history_valid));
	input_steering_value = 0;
}

static void hash_input_state(legacy_s16 result)
{
	hash_word(result);
	hash_word(input_elapsed_frames);
	hash_word(input_mouse_repeat_at);
	hash_word(input_joystick_repeat_at);
	hash_word(input_mouse_idle_frames);
	hash_word(input_combined_flags);
	hash_word(input_joystick_flags);
	hash_word(input_new_joystick_flags);
	hash_word(input_joystick_keycode);
	hash_word(input_mouse_keycode);
	hash_word(kbormouse);
	hash_word(mouse_xpos);
	hash_word(mouse_ypos);
	hash_word(mouse_butstate);
}

static legacy_u32 input_fingerprint(void)
{
	static const legacy_s16 deltas[] = {-32768, -1, 0, 1, 20, 21, 500, 501, 10000, 20001, 32767};
	unsigned sample, i, step;
	trace_hash = 2166136261UL;
	for (sample = 0; sample < 2048; sample++) {
		reset_inputs();
		input_elapsed_frames = deltas[sample % 11];
		input_mouse_repeat_at = deltas[(sample / 11) % 11];
		input_joystick_repeat_at = deltas[(sample / 121) % 11];
		mouse_transparent_mode = sample & 1;
		mouse_background_dirty = (sample >> 1) & 1;
		for (step = 0; step < 4; step++) {
			keyboard_char = random_word() % 3 == 0 ? KEY_ESCAPE : 0;
			joystick_flags = random_word() & 63;
			for (i = 0; i < 10; i++) {
				key_states[input_key_scancodes[i]] = random_word() % 8 == 0;
			}
			mouse_samples[step][0] = random_word() & 3;
			mouse_samples[step][1] = random_word() % 320;
			mouse_samples[step][2] = random_word() % 200;
			hash_input_state(input_checking(deltas[(sample + step) % 11]));
		}
	}
	return trace_hash;
}

static legacy_u32 scrollbar_fingerprint(void)
{
	unsigned horizontal, operation, position, drag;
	legacy_s16 x, y, result;
	trace_hash = 2166136261UL;
	for (horizontal = 0; horizontal < 2; horizontal++) {
		for (operation = 0; operation < 3; operation++) {
			for (position = 0; position < 8; position++) {
				for (drag = 0; drag < 3; drag++) {
					reset_inputs();
					x = 20;
					y = 30;
					mouse_xpos = x + (horizontal ? position * 15 : 2);
					mouse_ypos = y + (horizontal ? 2 : position * 15);
					mouse_samples[0][0] = 1;
					mouse_samples[1][0] = 0;
					mouse_samples[0][1] = mouse_samples[1][1] =
						mouse_xpos + (horizontal ? (int)drag * 70 - 70 : 0);
					mouse_samples[0][2] = mouse_samples[1][2] =
						mouse_ypos + (horizontal ? 0 : (int)drag * 70 - 70);
					result = scrollbar_update(operation, x, horizontal ? 120 : 6, y,
											  horizontal ? 6 : 120, 3, 2, 10);
					hash_input_state(result);
				}
			}
		}
	}
	return trace_hash;
}

static void configure_record_sample(unsigned sample)
{
	unsigned i;
	reset_inputs();
	framespersec = sample & 1 ? 10 : 20;
	game_replay_mode = sample % 3;
	elapsed_time1 = sample & 2 ? 600 : 0;
	elapsed_time2 = sample & 4 ? 12000 : (sample & 8 ? 11999 : sample % 30);
	gameconfig.game_recordedframes = elapsed_time2 + (sample & 16 ? 1 : 0);
	state.game_frame = elapsed_time2;
	passed_security = (sample >> 5) & 1;
	race_exit_request = (sample >> 6) & 1;
	state.game_end_event = sample & 128 ? 2 : 0;
	mouse_driving_enabled = (sample >> 8) & 1;
	joystick_enabled = (sample >> 9) & 1;
	joystick_axis = sample % 67 - 33;
	replay_overflow_acknowledged_word = sample & 1024 ? 0x5501 : 0x5500;
	key_states[KEY_SCAN_GEAR_UP] = sample & 1;
	key_states[KEY_SCAN_GEAR_DOWN] = sample & 2;
	mouse_samples[0][0] = sample % 4;
	mouse_samples[0][1] = sample % 321;
	for (i = 0; i < 12000; i++) {
		replay_bytes[i] = (legacy_s8)(i * 31 + sample);
	}
	for (i = 0; i < 41; i++) {
		memset(&snapshots[i], 0x5a, sizeof(snapshots[i]));
		snapshots[i].game_frame = i * 300;
	}
}

static void hash_record_state(void)
{
	unsigned i;
	hash_word(elapsed_time1);
	hash_word(elapsed_time2);
	hash_word(gameconfig.game_recordedframes);
	hash_word(replay_overflow_acknowledged_word);
	hash_word(recording_limit_warning_requested);
	hash_word(state.game_frame);
	hash_word(state.game_end_event);
	hash_word(race_exit_request);
	hash_word(is_in_replay);
	hash_word(frame_callback_count);
	hash_word(frame_callback_countdown);
	hash_word(slow_replay_countdown);
	hash_word(audio_car_state_interval);
	hash_word(audio_car_state_read_index);
	hash_word(frame_callback_active);
	for (i = 0; i < 12000; i++) {
		hash_word((legacy_u8)replay_bytes[i]);
	}
	for (i = 0; i < 41; i++) {
		hash_word(snapshots[i].game_frame);
	}
	for (i = 0; i < 64; i++) {
		hash_word(input_steering_history[i]);
		hash_word(input_steering_history_valid[i]);
	}
}

static legacy_u32 record_fingerprint(void)
{
	unsigned sample;
	trace_hash = 2166136261UL;
	for (sample = 0; sample < 2048; sample++) {
		configure_record_sample(sample);
		replay_update_input_tick(sample % 7 == 0);
		hash_record_state();
	}
	return trace_hash;
}

static legacy_u32 callback_fingerprint(void)
{
	unsigned sample, tick;
	trace_hash = 2166136261UL;
	for (sample = 0; sample < 512; sample++) {
		configure_record_sample(sample);
		elapsed_time1 = 0;
		elapsed_time2 = 20;
		gameconfig.game_recordedframes = 25;
		state.game_frame_in_sec = sample & 1 ? 2 : 0;
		state.game_frames_per_sec = 1;
		audio_car_state_read_index = sample & 2 ? AUDIO_CAR_STATE_RECORD_COUNT - 1 : 0;
		audio_car_state_write_index = 1;
		audio_car_state_interval = 5;
		replay_playback_speed = (legacy_u8)((sample % 3) - 1);
		segments_match = (sample >> 2) & 1;
		nested_callback = (sample >> 3) & 1;
		is_in_replay = (sample >> 4) & 1;
		for (tick = 0; tick < 8; tick++) {
			frame_callback();
		}
		hash_record_state();
	}
	return trace_hash;
}

/* All key values exercise the pause fallback as well as the recognized shortcuts. */
static legacy_u32 shortcut_fingerprint(void)
{
	legacy_u32 key;
	unsigned mode;
	trace_hash = 2166136261UL;
	for (mode = 0; mode < 3; mode++) {
		for (key = 0; key < 65536UL; key++) {
			reset_inputs();
			game_replay_mode = mode;
			gameconfig.game_opponenttype = key & 1;
			cameramode = key & 3;
			dashb_toggle = replaybar_toggle = followOpponentFlag = 0;
			hash_word(handle_ingame_kb_shortcuts(LEGACY_S16_FROM_BITS(key)));
			hash_word(game_replay_mode);
			hash_word(race_exit_request);
			hash_word(cameramode);
			hash_word(dashb_toggle);
			hash_word(replaybar_toggle);
			hash_word(h_key_toggle);
			hash_word(followOpponentFlag);
			hash_word(mouse_driving_enabled);
			hash_word(race_start_sequence_state);
		}
	}
	return trace_hash;
}

static void test_event_priority(void)
{
	unsigned index;
	reset_inputs();
	keyboard_char = 'a';
	joystick_flags = 63;
	for (index = 0; index < 4; index++) {
		mouse_samples[index][0] = 3;
	}
	assert(input_checking(0) == 'a');
	keyboard_char = 0;
	assert(input_checking(0) == KEY_ENTER);
	assert(input_checking(0) == KEY_SPACE);
	assert(input_checking(0) == 0);
	reset_inputs();
	key_states[input_key_scancodes[0]] = 1;
	joystick_flags = INPUT_ACCELERATE_FLAG;
	assert(get_kb_or_joy_flags() == INPUT_PRIMARY_ACTION_FLAG);
	reset_inputs();
	joystick_flags = INPUT_PRIMARY_ACTION_FLAG;
	assert(input_checking(0) == KEY_SPACE);
	assert(input_checking(INPUT_REPEAT_DELAY_FRAMES) == 0);
	assert(input_checking(1) == KEY_SPACE);
}

static void test_recording_input_modes(void)
{
	static const legacy_s16 positions[] = {141, 142, 143, 160, 177, 178, 179};
	static const legacy_s8 steering[] = {-1, 0, 0, 0, 0, 0, 1};
	unsigned index;
	for (index = 0; index < 7; index++) {
		reset_inputs();
		mouse_driving_enabled = joystick_enabled = 1;
		mouse_samples[0][0] = 3;
		mouse_samples[0][1] = positions[index];
		replay_update_input_tick(0);
		assert(replay_bytes[0] == INPUT_BRAKE_FLAG);
		assert(LEGACY_S8_FROM_BITS(input_steering_history[0]) == steering[index]);
		assert(input_steering_history_valid[0] == 1);
	}
	reset_inputs();
	game_replay_mode = REPLAY_MODE_PLAYBACK;
	gameconfig.game_recordedframes = 5;
	replay_bytes[0] = 63;
	replay_update_input_tick(0);
	assert(elapsed_time2 == 1 && gameconfig.game_recordedframes == 5 && replay_bytes[0] == 63);
	replay_update_input_tick(1);
	assert(elapsed_time2 == 2 && gameconfig.game_recordedframes == 6 && replay_bytes[1] == 0);
}

int main(void)
{
	legacy_u32 input_hash, scrollbar_hash, record_hash, callback_hash, shortcut_hash;
	input_hash = input_fingerprint();
	scrollbar_hash = scrollbar_fingerprint();
	record_hash = record_fingerprint();
	callback_hash = callback_fingerprint();
	shortcut_hash = shortcut_fingerprint();
	test_event_priority();
	test_recording_input_modes();
#ifdef INPUT_RECORD_BASELINE
	fprintf(stdout, "%08lx %08lx %08lx %08lx %08lx\n", (unsigned long)input_hash,
			(unsigned long)scrollbar_hash, (unsigned long)record_hash, (unsigned long)callback_hash,
			(unsigned long)shortcut_hash);
#else
	assert(input_hash == 0x2a5d4036UL);
	assert(scrollbar_hash == 0x207b3fe7UL);
	assert(record_hash == 0x0fac5847UL);
	assert(callback_hash == 0x9bd7fd3eUL);
	assert(shortcut_hash == 0xd2825d76UL);
#endif
	return 0;
}
