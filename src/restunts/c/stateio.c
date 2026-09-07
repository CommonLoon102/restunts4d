#include "gamestate.h"

struct GAMESTATE_WRITER {
	legacy_u8 far *destination;
	legacy_u16 offset;
};

static void gamestate_write_u8(struct GAMESTATE_WRITER *writer, legacy_u8 value)
{
	writer->destination[writer->offset++] = value;
}

static void gamestate_write_s8(struct GAMESTATE_WRITER *writer, legacy_s8 value)
{
	gamestate_write_u8(writer, (legacy_u8)value);
}

static void gamestate_write_u16(struct GAMESTATE_WRITER *writer, legacy_u16 value)
{
	LEGACY_WRITE_U16_LE(writer->destination + writer->offset, value);
	writer->offset += LEGACY_WORD_BYTES;
}

static void gamestate_write_s16(struct GAMESTATE_WRITER *writer, legacy_s16 value)
{
	gamestate_write_u16(writer, (legacy_u16)value);
}

static void gamestate_write_s32(struct GAMESTATE_WRITER *writer, legacy_s32 value)
{
	LEGACY_WRITE_U32_LE(writer->destination + writer->offset, (legacy_u32)value);
	writer->offset += LEGACY_DWORD_BYTES;
}

static void gamestate_write_vector(struct GAMESTATE_WRITER *writer, const struct VECTOR *vector)
{
	gamestate_write_s16(writer, vector->x);
	gamestate_write_s16(writer, vector->y);
	gamestate_write_s16(writer, vector->z);
}

static void gamestate_write_vectorlong(struct GAMESTATE_WRITER *writer,
									   const struct VECTORLONG *vector)
{
	gamestate_write_s32(writer, vector->lx);
	gamestate_write_s32(writer, vector->ly);
	gamestate_write_s32(writer, vector->lz);
}

static void gamestate_write_s8_array(struct GAMESTATE_WRITER *writer, const legacy_s8 *values,
									 legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++) {
		gamestate_write_s8(writer, values[index]);
	}
}

static void gamestate_write_s16_array(struct GAMESTATE_WRITER *writer, const legacy_s16 *values,
									  legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++) {
		gamestate_write_s16(writer, values[index]);
	}
}

static void gamestate_write_s32_array(struct GAMESTATE_WRITER *writer, const legacy_s32 *values,
									  legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++) {
		gamestate_write_s32(writer, values[index]);
	}
}

static void gamestate_write_vector_array(struct GAMESTATE_WRITER *writer,
										 const struct VECTOR *vectors, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++) {
		gamestate_write_vector(writer, &vectors[index]);
	}
}

static void gamestate_write_carstate(struct GAMESTATE_WRITER *writer,
									 const struct CARSTATE *carstate)
{
	gamestate_write_vectorlong(writer, &carstate->car_position);
	gamestate_write_vectorlong(writer, &carstate->car_previous_position);
	gamestate_write_vector(writer, &carstate->car_rotate);
	gamestate_write_s16(writer, carstate->car_pseudoGravity);
	gamestate_write_s16(writer, carstate->car_steeringAngle);
	gamestate_write_s16(writer, carstate->car_currpm);
	gamestate_write_s16(writer, carstate->car_lastrpm);
	gamestate_write_s16(writer, carstate->car_initial_rpm);
	gamestate_write_s16(writer, carstate->car_speeddiff);
	gamestate_write_u16(writer, carstate->car_rev_speed);
	gamestate_write_u16(writer, carstate->car_actual_speed);
	gamestate_write_u16(writer, carstate->car_lastspeed);
	gamestate_write_u16(writer, carstate->car_gearratio);
	gamestate_write_u16(writer, carstate->car_gearratioshr8);
	gamestate_write_s16(writer, carstate->car_knob_x);
	gamestate_write_s16(writer, carstate->car_velocity_heading_offset);
	gamestate_write_s16(writer, carstate->car_knob_y);
	gamestate_write_s16(writer, carstate->car_knob_x2);
	gamestate_write_s16(writer, carstate->car_knob_y2);
	gamestate_write_s16(writer, carstate->car_slide_yaw_delta);
	gamestate_write_s16(writer, carstate->car_front_wheel_response_angle);
	gamestate_write_s16(writer, carstate->car_slip_angle);
	gamestate_write_s16(writer, carstate->car_demandedGrip);
	gamestate_write_s16(writer, carstate->car_surfacegrip_sum);
	gamestate_write_s16(writer, carstate->car_route_heading_error);
	gamestate_write_s16(writer, carstate->car_route_index);
	gamestate_write_s16_array(writer, carstate->car_wheel_vertical_speed, CARSTATE_WHEEL_COUNT);
	gamestate_write_s16_array(writer, carstate->car_suspension_deflection, CARSTATE_WHEEL_COUNT);
	gamestate_write_s16_array(writer, carstate->car_reserved_wheel_state, CARSTATE_WHEEL_COUNT);
	gamestate_write_s16_array(writer, carstate->car_reserved_contact_state, CARSTATE_WHEEL_COUNT);
	gamestate_write_s16_array(writer, carstate->car_suspension_target, CARSTATE_WHEEL_COUNT);
	gamestate_write_vector_array(writer, carstate->car_wheel_contact_positions,
								 CARSTATE_WHEEL_COUNT);
	gamestate_write_vector_array(writer, carstate->car_body_corner_positions, CARSTATE_WHEEL_COUNT);
	gamestate_write_vector(writer, &carstate->car_route_target);
	gamestate_write_vector(writer, &carstate->car_route_first_edge);
	gamestate_write_vector(writer, &carstate->car_route_second_edge);
	gamestate_write_s16(writer, carstate->car_route_has_reverse_path);
	gamestate_write_s16(writer, carstate->car_reserved_route_word1);
	gamestate_write_s16(writer, carstate->car_reserved_route_word2);
	gamestate_write_s8(writer, carstate->car_is_braking);
	gamestate_write_s8(writer, carstate->car_is_accelerating);
	gamestate_write_s8(writer, carstate->car_current_gear);
	gamestate_write_s8(writer, carstate->car_sumSurfFrontWheels);
	gamestate_write_s8(writer, carstate->car_sumSurfRearWheels);
	gamestate_write_s8(writer, carstate->car_sumSurfAllWheels);
	gamestate_write_s8_array(writer, carstate->car_surfaceWhl, CARSTATE_WHEEL_COUNT);
	gamestate_write_s8(writer, carstate->car_engineLimiterTimer);
	gamestate_write_s8(writer, carstate->car_slidingFlag);
	gamestate_write_s8(writer, carstate->car_collision_latch);
	gamestate_write_s8(writer, carstate->car_crashBmpFlag);
	gamestate_write_s8(writer, carstate->car_changing_gear);
	gamestate_write_s8(writer, carstate->car_gear_change_delay);
	gamestate_write_s8(writer, carstate->car_transmission);
	gamestate_write_s8(writer, carstate->car_lap_count);
	gamestate_write_s8(writer, carstate->car_route_point_index);
	gamestate_write_s8(writer, carstate->car_sound_flags);
}

legacy_u16 gamestate_serialize(legacy_u8 far *destination, const struct GAMESTATE *source)
{
	struct GAMESTATE_WRITER writer;

	writer.destination = destination;
	writer.offset = 0U;

	gamestate_write_s32_array(&writer, source->game_particle_x, GAMESTATE_PARTICLE_SLOT_COUNT);
	gamestate_write_s32_array(&writer, source->game_particle_y, GAMESTATE_PARTICLE_SLOT_COUNT);
	gamestate_write_s32_array(&writer, source->game_particle_z, GAMESTATE_PARTICLE_SLOT_COUNT);
	gamestate_write_vector_array(&writer, source->game_follow_camera_position,
								 GAMESTATE_CAR_VECTOR_COUNT);
	gamestate_write_vector(&writer, &source->game_player_camera_previous);
	gamestate_write_vector(&writer, &source->game_opponent_camera_previous);
	gamestate_write_s16(&writer, source->game_frame_in_sec);
	gamestate_write_s16(&writer, source->game_frames_per_sec);
	gamestate_write_s32(&writer, source->game_travDist);
	gamestate_write_s16(&writer, source->game_frame);
	gamestate_write_s16(&writer, source->game_total_finish);
	gamestate_write_s16(&writer, source->game_opponent_finish_time);
	gamestate_write_s16(&writer, source->game_pEndFrame);
	gamestate_write_s16(&writer, source->game_oEndFrame);
	gamestate_write_s16(&writer, source->game_penalty);
	gamestate_write_u16(&writer, source->game_impactSpeed);
	gamestate_write_u16(&writer, source->game_topSpeed);
	gamestate_write_s16(&writer, source->game_jumpCount);
	gamestate_write_carstate(&writer, &source->playerstate);
	gamestate_write_carstate(&writer, &source->opponentstate);
	gamestate_write_s16(&writer, source->game_player_confirmed_route);
	gamestate_write_s16(&writer, source->game_player_previous_route);
	gamestate_write_s16(&writer, source->game_startcol);
	gamestate_write_s16(&writer, source->game_startcol2);
	gamestate_write_s16(&writer, source->game_startrow);
	gamestate_write_s16(&writer, source->game_startrow2);
	gamestate_write_s16_array(&writer, source->game_particle_rotation_x,
							  GAMESTATE_PARTICLE_SLOT_COUNT);
	gamestate_write_s16_array(&writer, source->game_particle_rotation_y,
							  GAMESTATE_PARTICLE_SLOT_COUNT);
	gamestate_write_s16_array(&writer, source->game_particle_heading,
							  GAMESTATE_PARTICLE_SLOT_COUNT);
	gamestate_write_s16_array(&writer, source->game_particle_forward_speed,
							  GAMESTATE_PARTICLE_SLOT_COUNT);
	gamestate_write_s8_array(&writer, source->game_particle_vertical_speed,
							 GAMESTATE_PARTICLE_VELOCITY_BYTES);
	gamestate_write_s8_array(&writer, source->kevinseed, GAMESTATE_RANDOM_SEED_SIZE);
	gamestate_write_s8(&writer, source->game_checkpoint_valid);
	gamestate_write_s8(&writer, source->game_inputmode);
	gamestate_write_s8(&writer, source->game_end_event);
	gamestate_write_s8_array(&writer, source->game_trackside_camera_index,
							 GAMESTATE_CAMERA_INDEX_COUNT);
	gamestate_write_s8(&writer, source->game_opponent_target_speed);
	gamestate_write_s8_array(&writer, source->game_object_destroyed,
							 GAMESTATE_BREAKABLE_OBJECT_COUNT);
	gamestate_write_s8(&writer, source->game_particles_active);
	gamestate_write_s8_array(&writer, source->game_particle_shape_index,
							 GAMESTATE_PARTICLE_SLOT_COUNT);
	gamestate_write_s8_array(&writer, source->game_particle_owner, GAMESTATE_PARTICLE_SLOT_COUNT);
	gamestate_write_s8(&writer, source->game_player_route_status);
	gamestate_write_s8(&writer, source->game_route_confirmation_count);
	gamestate_write_s8(&writer, source->game_player_route_indicator);
	gamestate_write_s8(&writer, source->game_opponent_route_indicator);
	gamestate_write_s8(&writer, source->game_reserved_trailing_byte);

	return writer.offset;
}
