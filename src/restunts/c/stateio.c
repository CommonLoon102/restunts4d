#include "gamestate.h"

struct GAMESTATE_WRITER {
	legacy_u8 far* destination;
	legacy_u16 offset;
};

static void gamestate_write_u8(struct GAMESTATE_WRITER* writer,
	legacy_u8 value)
{
	writer->destination[writer->offset++] = value;
}

static void gamestate_write_s8(struct GAMESTATE_WRITER* writer,
	legacy_s8 value)
{
	gamestate_write_u8(writer, (legacy_u8)value);
}

static void gamestate_write_u16(struct GAMESTATE_WRITER* writer,
	legacy_u16 value)
{
	LEGACY_WRITE_U16_LE(writer->destination + writer->offset, value);
	writer->offset += 2U;
}

static void gamestate_write_s16(struct GAMESTATE_WRITER* writer,
	legacy_s16 value)
{
	gamestate_write_u16(writer, (legacy_u16)value);
}

static void gamestate_write_s32(struct GAMESTATE_WRITER* writer,
	legacy_s32 value)
{
	LEGACY_WRITE_U32_LE(writer->destination + writer->offset,
		(legacy_u32)value);
	writer->offset += 4U;
}

static void gamestate_write_vector(struct GAMESTATE_WRITER* writer,
	const struct VECTOR* vector)
{
	gamestate_write_s16(writer, vector->x);
	gamestate_write_s16(writer, vector->y);
	gamestate_write_s16(writer, vector->z);
}

static void gamestate_write_vectorlong(struct GAMESTATE_WRITER* writer,
	const struct VECTORLONG* vector)
{
	gamestate_write_s32(writer, vector->lx);
	gamestate_write_s32(writer, vector->ly);
	gamestate_write_s32(writer, vector->lz);
}

static void gamestate_write_s8_array(struct GAMESTATE_WRITER* writer,
	const legacy_s8* values, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++) {
		gamestate_write_s8(writer, values[index]);
	}
}

static void gamestate_write_s16_array(struct GAMESTATE_WRITER* writer,
	const legacy_s16* values, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++) {
		gamestate_write_s16(writer, values[index]);
	}
}

static void gamestate_write_s32_array(struct GAMESTATE_WRITER* writer,
	const legacy_s32* values, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++) {
		gamestate_write_s32(writer, values[index]);
	}
}

static void gamestate_write_vector_array(struct GAMESTATE_WRITER* writer,
	const struct VECTOR* vectors, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++) {
		gamestate_write_vector(writer, &vectors[index]);
	}
}

static void gamestate_write_carstate(struct GAMESTATE_WRITER* writer,
	const struct CARSTATE* carstate)
{
	gamestate_write_vectorlong(writer, &carstate->car_posWorld1);
	gamestate_write_vectorlong(writer, &carstate->car_posWorld2);
	gamestate_write_vector(writer, &carstate->car_rotate);
	gamestate_write_s16(writer, carstate->car_pseudoGravity);
	gamestate_write_s16(writer, carstate->car_steeringAngle);
	gamestate_write_s16(writer, carstate->car_currpm);
	gamestate_write_s16(writer, carstate->car_lastrpm);
	gamestate_write_s16(writer, carstate->car_idlerpm2);
	gamestate_write_s16(writer, carstate->car_speeddiff);
	gamestate_write_u16(writer, carstate->car_speed);
	gamestate_write_u16(writer, carstate->car_speed2);
	gamestate_write_u16(writer, carstate->car_lastspeed);
	gamestate_write_u16(writer, carstate->car_gearratio);
	gamestate_write_u16(writer, carstate->car_gearratioshr8);
	gamestate_write_s16(writer, carstate->car_knob_x);
	gamestate_write_s16(writer, carstate->car_36MwhlAngle);
	gamestate_write_s16(writer, carstate->car_knob_y);
	gamestate_write_s16(writer, carstate->car_knob_x2);
	gamestate_write_s16(writer, carstate->car_knob_y2);
	gamestate_write_s16(writer, carstate->car_angle_z);
	gamestate_write_s16(writer, carstate->car_40MfrontWhlAngle);
	gamestate_write_s16(writer, carstate->field_42);
	gamestate_write_s16(writer, carstate->car_demandedGrip);
	gamestate_write_s16(writer, carstate->car_surfacegrip_sum);
	gamestate_write_s16(writer, carstate->field_48);
	gamestate_write_s16(writer, carstate->car_trackdata3_index);
	gamestate_write_s16_array(writer, carstate->car_rc1, 4U);
	gamestate_write_s16_array(writer, carstate->car_rc2, 4U);
	gamestate_write_s16_array(writer, carstate->car_rc3, 4U);
	gamestate_write_s16_array(writer, carstate->car_rc4, 4U);
	gamestate_write_s16_array(writer, carstate->car_rc5, 4U);
	gamestate_write_vector_array(writer, carstate->car_whlWorldCrds1, 4U);
	gamestate_write_vector_array(writer, carstate->car_whlWorldCrds2, 4U);
	gamestate_write_vector(writer, &carstate->car_vec_unk3);
	gamestate_write_vector(writer, &carstate->car_vec_unk4);
	gamestate_write_vector(writer, &carstate->car_vec_unk5);
	gamestate_write_s16(writer, carstate->field_B6);
	gamestate_write_s16(writer, carstate->field_B8);
	gamestate_write_s16(writer, carstate->field_BA);
	gamestate_write_s8(writer, carstate->car_is_braking);
	gamestate_write_s8(writer, carstate->car_is_accelerating);
	gamestate_write_s8(writer, carstate->car_current_gear);
	gamestate_write_s8(writer, carstate->car_sumSurfFrontWheels);
	gamestate_write_s8(writer, carstate->car_sumSurfRearWheels);
	gamestate_write_s8(writer, carstate->car_sumSurfAllWheels);
	gamestate_write_s8_array(writer, carstate->car_surfaceWhl, 4U);
	gamestate_write_s8(writer, carstate->car_engineLimiterTimer);
	gamestate_write_s8(writer, carstate->car_slidingFlag);
	gamestate_write_s8(writer, carstate->field_C8);
	gamestate_write_s8(writer, carstate->car_crashBmpFlag);
	gamestate_write_s8(writer, carstate->car_changing_gear);
	gamestate_write_s8(writer, carstate->car_fpsmul2);
	gamestate_write_s8(writer, carstate->car_transmission);
	gamestate_write_s8(writer, carstate->field_CD);
	gamestate_write_s8(writer, carstate->field_CE);
	gamestate_write_s8(writer, carstate->field_CF);
}

legacy_u16 gamestate_serialize(legacy_u8 far* destination,
	const struct GAMESTATE* source)
{
	struct GAMESTATE_WRITER writer;

	writer.destination = destination;
	writer.offset = 0U;

	gamestate_write_s32_array(&writer, source->game_longs1, 24U);
	gamestate_write_s32_array(&writer, source->game_longs2, 24U);
	gamestate_write_s32_array(&writer, source->game_longs3, 24U);
	gamestate_write_vector_array(&writer, source->game_vec1, 2U);
	gamestate_write_vector(&writer, &source->game_vec3);
	gamestate_write_vector(&writer, &source->game_vec4);
	gamestate_write_s16(&writer, source->game_frame_in_sec);
	gamestate_write_s16(&writer, source->game_frames_per_sec);
	gamestate_write_s32(&writer, source->game_travDist);
	gamestate_write_s16(&writer, source->game_frame);
	gamestate_write_s16(&writer, source->game_total_finish);
	gamestate_write_s16(&writer, source->field_144);
	gamestate_write_s16(&writer, source->game_pEndFrame);
	gamestate_write_s16(&writer, source->game_oEndFrame);
	gamestate_write_s16(&writer, source->game_penalty);
	gamestate_write_u16(&writer, source->game_impactSpeed);
	gamestate_write_u16(&writer, source->game_topSpeed);
	gamestate_write_s16(&writer, source->game_jumpCount);
	gamestate_write_carstate(&writer, &source->playerstate);
	gamestate_write_carstate(&writer, &source->opponentstate);
	gamestate_write_s16(&writer, source->field_2F2);
	gamestate_write_s16(&writer, source->field_2F4);
	gamestate_write_s16(&writer, source->game_startcol);
	gamestate_write_s16(&writer, source->game_startcol2);
	gamestate_write_s16(&writer, source->game_startrow);
	gamestate_write_s16(&writer, source->game_startrow2);
	gamestate_write_s16_array(&writer, source->field_2FE, 24U);
	gamestate_write_s16_array(&writer, source->field_32E, 24U);
	gamestate_write_s16_array(&writer, source->field_35E, 24U);
	gamestate_write_s16_array(&writer, source->field_38E, 24U);
	gamestate_write_s8_array(&writer, source->field_3BE, 48U);
	gamestate_write_s8_array(&writer, source->kevinseed, 6U);
	gamestate_write_s8(&writer, source->field_3F4);
	gamestate_write_s8(&writer, source->game_inputmode);
	gamestate_write_s8(&writer, source->game_3F6autoLoadEvalFlag);
	gamestate_write_s8_array(&writer, source->field_3F7, 2U);
	gamestate_write_s8(&writer, source->field_3F9);
	gamestate_write_s8_array(&writer, source->field_3FA, 48U);
	gamestate_write_s8(&writer, source->field_42A);
	gamestate_write_s8_array(&writer, source->field_42B, 24U);
	gamestate_write_s8_array(&writer, source->field_443, 24U);
	gamestate_write_s8(&writer, source->field_45B);
	gamestate_write_s8(&writer, source->field_45C);
	gamestate_write_s8(&writer, source->field_45D);
	gamestate_write_s8(&writer, source->field_45E);
	gamestate_write_s8(&writer, source->field_45F);

	return writer.offset;
}
