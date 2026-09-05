#include "audio_internal.h"
#include "externs.h"
#include "game_input.h"
#include "keyboard.h"
#include "memmgr.h"
#include "platform.h"
#include "replay_record.h"
#include "timing.h"

#define INPUT_STEERING_HISTORY_SIZE 64U
#define INPUT_STEERING_HISTORY_MASK 63U
#define AUDIO_REPLAY_MODE_UNINITIALIZED 255U
#define JOYSTICK_STEERING_TABLE_SIZE 34U
#define AUDIO_STATE_CHUNK_NAME_SIZE 12U
#define FRAME_CALLBACK_TIMER_TICKS 10UL
#define REPLAY_MODE_LIVE 0U
#define REPLAY_MODE_PAUSED 1U
#define REPLAY_MODE_PLAYBACK 2U
#define REPLAY_PLAYBACK_SLOW_MODE 2
#define REPLAY_PLAYBACK_FAST_MODE 3
#define REPLAY_SLOW_CALLBACK_DIVISOR 2U
#define REPLAY_SECURITY_GRACE_SECONDS 4U
#define MOUSE_HORIZONTAL_CENTER 160
#define MOUSE_STEERING_DEAD_ZONE 18
#define MOUSE_LEFT_BUTTON_MASK 1U
#define MOUSE_RIGHT_BUTTON_MASK 2U
#define KEY_SCAN_GEAR_UP 30
#define KEY_SCAN_GEAR_DOWN 44
#define REPLAY_TOTAL_LIMIT_SECONDS 1500U
#define REPLAY_INPUT_BUFFER_FRAME_COUNT 12000U
#define REPLAY_HISTORY_SHIFT_SECONDS 30U
#define STEERING_RESPONSE_SPEED_SHIFT 10U
#define STEERING_RESPONSE_SPEED_MASK 252U
#define STEERING_RESPONSE_TABLE_OFFSET 1U
#define STEERING_RESPONSE_RECENTER_SHIFT 2U

static legacy_u8 input_steering_history[INPUT_STEERING_HISTORY_SIZE];
static legacy_u8 input_steering_history_valid[INPUT_STEERING_HISTORY_SIZE];
legacy_u16 frame_callback_count;
static legacy_u8 frame_callback_active;
legacy_s16 audio_car_state_read_index;
legacy_s16 audio_car_state_write_index;
legacy_s16 audio_car_state_interval;
struct AUDIO_CAR_STATE far* audio_car_state_records;
legacy_u8 audio_previous_replay_mode = AUDIO_REPLAY_MODE_UNINITIALIZED;
static const legacy_s8 far joystick_steering_table[
	JOYSTICK_STEERING_TABLE_SIZE] = {
	0, 0, 0, 0, 0, 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40,
	44, 48, 52, 56, 60, 64, 68, 72, 76, 84, 90, 98, 106, 114,
	121, 127, 127, 127
};
static legacy_s8 input_steering_value;

void audio_allocate_car_state_records(void)
{
	static const legacy_s8 chunk_name[AUDIO_STATE_CHUNK_NAME_SIZE] =
		{ 'a', 'u', 'd', 'i', 'o', 's', 't', 'a', 't', 'e', 0, 0 };
	legacy_u8 far* bytes;
	legacy_u16 index;

	audio_car_state_records =
		(struct AUDIO_CAR_STATE far*)mmgr_alloc_resbytes(
			chunk_name, (legacy_s32)AUDIO_CAR_STATE_RECORD_COUNT *
			AUDIO_CAR_STATE_RECORD_SIZE);
	bytes = (legacy_u8 far*)audio_car_state_records;
	for (index = 0;
		index < AUDIO_CAR_STATE_RECORD_COUNT * AUDIO_CAR_STATE_RECORD_SIZE;
		index++)
		bytes[index] = 0;
}

void set_frame_callback(void)
{
	frame_callback_count = 0;
	timer_reg_callback(&frame_callback);
	frame_callback_active = 0;
}

void remove_frame_callback(void)
{
	timer_get_counter_unk(FRAME_CALLBACK_TIMER_TICKS);
	timer_remove_callback(&frame_callback);
}

void frame_callback(void)
{
	if (dos_data_stack_segments_match() == 0 || frame_callback_active != 0)
		return;

	frame_callback_active = 1;
	audio_car_state_interval = LEGACY_S16_WRAP_ADD(audio_car_state_interval, 1);
	if (audio_car_state_interval >= word_4499C && audio_car_state_read_index != audio_car_state_write_index) {
		sub_18D06((legacy_u8 far*)
			&audio_car_state_records[audio_car_state_read_index],
			audio_car_state_interval);
		audio_car_state_interval = 0;
		audio_car_state_read_index = LEGACY_S16_WRAP_ADD(audio_car_state_read_index, 1);
		if (audio_car_state_read_index == AUDIO_CAR_STATE_RECORD_COUNT)
			audio_car_state_read_index = 0;
	}

	if (byte_449DA == 0 && byte_46467 == 0 &&
		(is_in_replay == 0 || game_replay_mode != REPLAY_MODE_PLAYBACK)) {
		if (game_replay_mode == REPLAY_MODE_LIVE &&
			LEGACY_S16_FROM_BITS(state.game_frame_in_sec) >=
			LEGACY_S16_FROM_BITS(state.game_frames_per_sec)) {
			is_in_replay = 1;
			audio_carstate();
		} else {
			byte_44A8A = (legacy_u8)(byte_44A8A - 1U);
			if (byte_44A8A == 0) {
				byte_44A8A = (legacy_u8)word_4499C;
				frame_callback_count = LEGACY_U16_WRAP_ADD(frame_callback_count, 1U);
				if (game_replay_mode == REPLAY_MODE_PLAYBACK &&
					LEGACY_S8_FROM_BITS(byte_449E6) ==
						REPLAY_PLAYBACK_SLOW_MODE) {
					byte_4552F = (legacy_u8)(byte_4552F - 1U);
					if (byte_4552F == 0) {
						replay_unk2(0);
						byte_4552F = REPLAY_SLOW_CALLBACK_DIVISOR;
					}
				} else {
					if (game_replay_mode == REPLAY_MODE_PLAYBACK &&
						LEGACY_S8_FROM_BITS(byte_449E6) ==
							REPLAY_PLAYBACK_FAST_MODE)
						replay_unk2(0);
					replay_unk2(0);
				}
			}
		}
	}

	frame_callback_active--;
}

void replay_unk2(legacy_s16 mode)
{
	legacy_s16 input_flags;
	legacy_s16 steering;
	legacy_s16 snapshot_index;
	legacy_s16 snapshot_count;
	legacy_u16 history_index;
	legacy_u16 recording_chunk;
	legacy_u16 recording_limit;
	legacy_u16 elapsed_total;
	legacy_u16 input_index;
	legacy_s8 mapped_steering;

	if (mode != 0) {
		input_flags = 0;
	} else if (game_replay_mode == REPLAY_MODE_PLAYBACK) {
		if (gameconfig.game_recordedframes > elapsed_time2) {
			elapsed_time2++;
			return;
		}
		if (byte_449DA != 0)
			return;
		is_in_replay = 1;
		audio_carstate();
		byte_449DA = 1;
		return;
	} else if (byte_449DA == 0 &&
		state.game_3F6autoLoadEvalFlag == 0 &&
		game_replay_mode != REPLAY_MODE_PAUSED) {
		if (passed_security == 0 && byte_4393C == 0 &&
			(legacy_u16)state.game_frame >
				LEGACY_U16_WRAP_MUL(framespersec,
					REPLAY_SECURITY_GRACE_SECONDS))
			update_crash_state(1, 0);

		if (byte_3B8F2 != 0 || dos_joystick_is_enabled() != 0) {
			if (byte_3B8F2 != 0) {
				dos_mouse_get_state(
					&mouse_butstate, &mouse_xpos, &mouse_ypos);
				steering = LEGACY_S16_WRAP_SUB(mouse_xpos,
					MOUSE_HORIZONTAL_CENTER);
				if (steering > -MOUSE_STEERING_DEAD_ZONE &&
					steering < MOUSE_STEERING_DEAD_ZONE) {
					steering = 0;
				} else if (steering > 0) {
					steering = LEGACY_S16_WRAP_SUB(steering,
						MOUSE_STEERING_DEAD_ZONE);
				} else {
					steering = LEGACY_S16_WRAP_ADD(steering,
						MOUSE_STEERING_DEAD_ZONE);
				}
				input_steering_value = LEGACY_S8_FROM_BITS(steering);
				if (((legacy_u16)mouse_butstate &
					MOUSE_LEFT_BUTTON_MASK) != 0)
					input_flags = INPUT_BRAKE_FLAG;
				else if (((legacy_u16)mouse_butstate &
					MOUSE_RIGHT_BUTTON_MASK) != 0)
					input_flags = INPUT_ACCELERATE_FLAG;
				else
					input_flags = 0;
			} else {
				mapped_steering = LEGACY_S8_FROM_BITS(sub_307E3());
				input_steering_value = mapped_steering;
				if (mapped_steering > 0) {
					input_steering_value = joystick_steering_table[
						(legacy_u8)mapped_steering];
				} else if (mapped_steering < 0) {
					input_steering_value = LEGACY_S8_FROM_BITS(
						(legacy_u8)(0U - (legacy_u8)joystick_steering_table[
							(legacy_u8)(0U -
								(legacy_u8)mapped_steering)]));
				}
				input_flags = (legacy_s16)
					((legacy_u16)get_kb_or_joy_flags() &
						INPUT_NON_STEERING_MASK);
			}
			history_index = (legacy_u16)elapsed_time2 &
				INPUT_STEERING_HISTORY_MASK;
			input_steering_history[history_index] = (legacy_u8)input_steering_value;
			input_steering_history_valid[history_index] = 1;
		} else {
			input_flags = get_kb_or_joy_flags();
		}

		if (kb_get_key_state(KEY_SCAN_GEAR_UP) != 0)
			input_flags = (legacy_s16)
				((legacy_u16)input_flags | INPUT_SHIFT_UP_FLAG);
		if (kb_get_key_state(KEY_SCAN_GEAR_DOWN) != 0)
			input_flags = (legacy_s16)
				((legacy_u16)input_flags | INPUT_SHIFT_DOWN_FLAG);
	} else {
		input_flags = 0;
	}

	recording_limit = LEGACY_U16_WRAP_MUL(REPLAY_TOTAL_LIMIT_SECONDS,
		framespersec);
	elapsed_total = LEGACY_U16_WRAP_ADD(elapsed_time2, elapsed_time1);
	if (recording_limit <= elapsed_total) {
		update_crash_state(4, 0);
		byte_449DA = 1;
		return;
	}

	if (elapsed_time2 == REPLAY_INPUT_BUFFER_FRAME_COUNT) {
		if (elapsed_time1 == 0 &&
			LEGACY_U16_LOW_BYTE(word_45D3E) == 0) {
			word_45D3E = LEGACY_S16_FROM_BITS(
				LEGACY_U16_REPLACE_LOW_BYTE(word_45D3E, 1U));
			byte_46467 = 1;
			return;
		}

		recording_chunk = LEGACY_U16_WRAP_MUL(REPLAY_HISTORY_SHIFT_SECONDS,
			framespersec);
		snapshot_count = LEGACY_S16_WRAP_SUB(
			LEGACY_S16_FROM_BITS(LEGACY_U16_DIV_OR_ZERO(
				REPLAY_INPUT_BUFFER_FRAME_COUNT, recording_chunk)), 1);
		for (snapshot_index = 0;
			snapshot_index < snapshot_count;
			snapshot_index++) {
			cvxptr[snapshot_index + 1].game_frame =
				LEGACY_S16_WRAP_SUB(
					cvxptr[snapshot_index + 1].game_frame,
					recording_chunk);
			fmemcpy(&cvxptr[snapshot_index],
				&cvxptr[snapshot_index + 1],
				sizeof(struct GAMESTATE));
		}
		for (input_index = 0;
			input_index < (legacy_u16)(REPLAY_INPUT_BUFFER_FRAME_COUNT -
				recording_chunk);
			input_index++)
			td16_rpl_buffer[input_index] =
				td16_rpl_buffer[input_index + recording_chunk];
		elapsed_time2 = LEGACY_U16_WRAP_SUB(
			elapsed_time2, recording_chunk);
		gameconfig.game_recordedframes = LEGACY_U16_WRAP_SUB(
			gameconfig.game_recordedframes, recording_chunk);
		elapsed_time1 = LEGACY_U16_WRAP_ADD(
			elapsed_time1, recording_chunk);
		state.game_frame = LEGACY_S16_WRAP_SUB(
			state.game_frame, recording_chunk);
	}

	td16_rpl_buffer[elapsed_time2] = (legacy_u8)input_flags;
	elapsed_time2++;
	gameconfig.game_recordedframes++;
}

void replay_unk(void)
{
	legacy_s16 steering_angle;
	legacy_s16 target_angle;
	legacy_s16 response;
	legacy_s16 adjusted_angle;
	legacy_u16 frame;
	legacy_u16 history_index;
	legacy_u16 speed_index;
	legacy_u8 action;
	legacy_s8* response_table;

	frame = state.game_frame;
	history_index = frame & INPUT_STEERING_HISTORY_MASK;
	if (input_steering_history_valid[history_index] == 0)
		return;

	target_angle = LEGACY_S8_FROM_BITS(input_steering_history[history_index]);
	steering_angle = state.playerstate.car_steeringAngle;
	speed_index = (state.playerstate.car_speed2 >>
		STEERING_RESPONSE_SPEED_SHIFT) & STEERING_RESPONSE_SPEED_MASK;
	response_table = steerWhlRespTable_ptr;
	response = response_table[speed_index + STEERING_RESPONSE_TABLE_OFFSET];
	if ((steering_angle < target_angle && steering_angle < -1) ||
		(steering_angle > target_angle && steering_angle > 1)) {
		response = LEGACY_S8_FROM_BITS(
			(legacy_u8)((legacy_u8)response <<
				STEERING_RESPONSE_RECENTER_SHIFT));
	}

	action = 0;
	if (steering_angle > target_angle) {
		adjusted_angle = LEGACY_S16_WRAP_SUB(steering_angle, response);
		if (adjusted_angle >= target_angle)
			action = INPUT_STEER_LEFT_FLAG;
	} else if (steering_angle < target_angle) {
		adjusted_angle = LEGACY_S16_WRAP_ADD(steering_angle, response);
		if (adjusted_angle <= target_angle)
			action = INPUT_STEER_RIGHT_FLAG;
	}
	if (action != 0)
		td16_rpl_buffer[frame] |= action;
	input_steering_history_valid[history_index] = 0;
}
