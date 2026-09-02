#include "audio_internal.h"
#include "externs.h"
#include "game_input.h"
#include "keyboard.h"
#include "memmgr.h"
#include "platform.h"
#include "replay_record.h"
#include "timing.h"

legacy_s16 get_kb_or_joy_flags(void);
legacy_s16 sub_307E3(void);
void timer_reg_callback(void (far* callback)(void));
void timer_remove_callback(void (far* callback)(void));
legacy_u32 timer_get_counter_unk(legacy_u32 ticks);

static legacy_u8 input_steering_history[64];
static legacy_u8 input_steering_history_valid[64];
legacy_u16 frame_callback_count;
static legacy_u8 frame_callback_active;
legacy_s16 audio_car_state_read_index;
legacy_s16 audio_car_state_write_index;
legacy_s16 audio_car_state_interval;
legacy_u8 far* audio_car_state_records;
legacy_u8 audio_previous_replay_mode = 0xFFU;
static const legacy_s8 far joystick_steering_table[34] = {
	0, 0, 0, 0, 0, 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40,
	44, 48, 52, 56, 60, 64, 68, 72, 76, 84, 90, 98, 106, 114,
	121, 127, 127, 127
};
static legacy_s8 input_steering_value;

void audio_allocate_car_state_records(void)
{
	static const legacy_s8 chunk_name[12] =
		{ 'a', 'u', 'd', 'i', 'o', 's', 't', 'a', 't', 'e', 0, 0 };
	legacy_u16 index;

	audio_car_state_records = (legacy_u8 far*)mmgr_alloc_resbytes(
		chunk_name, (legacy_s32)AUDIO_CAR_STATE_RECORD_COUNT *
		AUDIO_CAR_STATE_RECORD_SIZE);
	for (index = 0;
		index < AUDIO_CAR_STATE_RECORD_COUNT * AUDIO_CAR_STATE_RECORD_SIZE;
		index++)
		audio_car_state_records[index] = 0;
}

void set_frame_callback(void)
{
	frame_callback_count = 0;
	timer_reg_callback(&frame_callback);
	frame_callback_active = 0;
}

void remove_frame_callback(void)
{
	timer_get_counter_unk(10UL);
	timer_remove_callback(&frame_callback);
}

void frame_callback(void)
{
	if (dos_data_stack_segments_match() == 0 || frame_callback_active != 0)
		return;

	frame_callback_active = 1;
	audio_car_state_interval = LEGACY_S16_WRAP_ADD(audio_car_state_interval, 1);
	if (audio_car_state_interval >= word_4499C && audio_car_state_read_index != audio_car_state_write_index) {
		sub_18D06(audio_car_state_records + AUDIO_CAR_STATE_RECORD_SIZE *
			(legacy_u16)audio_car_state_read_index,
			audio_car_state_interval);
		audio_car_state_interval = 0;
		audio_car_state_read_index = LEGACY_S16_WRAP_ADD(audio_car_state_read_index, 1);
		if (audio_car_state_read_index == AUDIO_CAR_STATE_RECORD_COUNT)
			audio_car_state_read_index = 0;
	}

	if (byte_449DA == 0 && byte_46467 == 0 &&
		(is_in_replay == 0 || game_replay_mode != 2)) {
		if (game_replay_mode == 0 &&
			LEGACY_S16_FROM_BITS(state.game_frame_in_sec) >=
			LEGACY_S16_FROM_BITS(state.game_frames_per_sec)) {
			is_in_replay = 1;
			audio_carstate();
		} else {
			byte_44A8A = (legacy_u8)(byte_44A8A - 1U);
			if (byte_44A8A == 0) {
				byte_44A8A = (legacy_u8)word_4499C;
				frame_callback_count = LEGACY_U16_WRAP_ADD(frame_callback_count, 1U);
				if (game_replay_mode == 2 &&
					LEGACY_S8_FROM_BITS(byte_449E6) == 2) {
					byte_4552F = (legacy_u8)(byte_4552F - 1U);
					if (byte_4552F == 0) {
						replay_unk2(0);
						byte_4552F = 2;
					}
				} else {
					if (game_replay_mode == 2 &&
						LEGACY_S8_FROM_BITS(byte_449E6) == 3)
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
	} else if (game_replay_mode == 2) {
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
		game_replay_mode != 1) {
		if (passed_security == 0 && byte_4393C == 0 &&
			(legacy_u16)state.game_frame >
				LEGACY_U16_WRAP_MUL(framespersec, 4U))
			update_crash_state(1, 0);

		if (byte_3B8F2 != 0 || dos_joystick_is_enabled() != 0) {
			if (byte_3B8F2 != 0) {
				dos_mouse_get_state(
					&mouse_butstate, &mouse_xpos, &mouse_ypos);
				steering = LEGACY_S16_WRAP_SUB(mouse_xpos, 0xA0);
				if (steering > -0x12 && steering < 0x12) {
					steering = 0;
				} else if (steering > 0) {
					steering = LEGACY_S16_WRAP_SUB(steering, 0x12);
				} else {
					steering = LEGACY_S16_WRAP_ADD(steering, 0x12);
				}
				input_steering_value = LEGACY_S8_FROM_BITS(steering);
				if (((legacy_u16)mouse_butstate & 1U) != 0)
					input_flags = 2;
				else if (((legacy_u16)mouse_butstate & 2U) != 0)
					input_flags = 1;
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
					((legacy_u16)get_kb_or_joy_flags() & 0x33U);
			}
			history_index = (legacy_u16)elapsed_time2 & 0x3FU;
			input_steering_history[history_index] = (legacy_u8)input_steering_value;
			input_steering_history_valid[history_index] = 1;
		} else {
			input_flags = get_kb_or_joy_flags();
		}

		if (kb_get_key_state(0x1E) != 0)
			input_flags = (legacy_s16)
				((legacy_u16)input_flags | 0x10U);
		if (kb_get_key_state(0x2C) != 0)
			input_flags = (legacy_s16)
				((legacy_u16)input_flags | 0x20U);
	} else {
		input_flags = 0;
	}

	recording_limit = LEGACY_U16_WRAP_MUL(0x5DCU, framespersec);
	elapsed_total = LEGACY_U16_WRAP_ADD(elapsed_time2, elapsed_time1);
	if (recording_limit <= elapsed_total) {
		update_crash_state(4, 0);
		byte_449DA = 1;
		return;
	}

	if (elapsed_time2 == 0x2EE0U) {
		if (elapsed_time1 == 0 &&
			LEGACY_U16_LOW_BYTE(word_45D3E) == 0) {
			word_45D3E = LEGACY_S16_FROM_BITS(
				LEGACY_U16_REPLACE_LOW_BYTE(word_45D3E, 1U));
			byte_46467 = 1;
			return;
		}

		recording_chunk = LEGACY_U16_WRAP_MUL(0x1EU, framespersec);
		snapshot_count = LEGACY_S16_WRAP_SUB(
			LEGACY_S16_FROM_BITS(LEGACY_U16_DIV_OR_ZERO(
				0x2EE0U, recording_chunk)), 1);
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
			input_index < (legacy_u16)(0x2EE0U - recording_chunk);
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
	history_index = frame & 0x3FU;
	if (input_steering_history_valid[history_index] == 0)
		return;

	target_angle = LEGACY_S8_FROM_BITS(input_steering_history[history_index]);
	steering_angle = state.playerstate.car_steeringAngle;
	speed_index = (state.playerstate.car_speed2 >> 10) & 0xFCU;
	response_table = steerWhlRespTable_ptr;
	response = response_table[speed_index + 1U];
	if ((steering_angle < target_angle && steering_angle < -1) ||
		(steering_angle > target_angle && steering_angle > 1)) {
		response = LEGACY_S8_FROM_BITS(
			(legacy_u8)((legacy_u8)response << 2));
	}

	action = 0;
	if (steering_angle > target_angle) {
		adjusted_angle = LEGACY_S16_WRAP_SUB(steering_angle, response);
		if (adjusted_angle >= target_angle)
			action = 8;
	} else if (steering_angle < target_angle) {
		adjusted_angle = LEGACY_S16_WRAP_ADD(steering_angle, response);
		if (adjusted_angle <= target_angle)
			action = 4;
	}
	if (action != 0)
		td16_rpl_buffer[frame] |= action;
	input_steering_history_valid[history_index] = 0;
}
