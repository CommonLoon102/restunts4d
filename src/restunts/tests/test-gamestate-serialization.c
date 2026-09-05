#include <assert.h>
#include <string.h>

#include "../c/gamestate.h"

#define TEST_OUTPUT_GUARD_SIZE 2U
#define TEST_OUTPUT_PREFIX_SIZE 1U
#define TEST_PATTERN_MULTIPLIER 37U
#define TEST_PATTERN_INCREMENT 11U
#define TEST_OUTPUT_GUARD_BYTE 90
#define TEST_TRAVEL_DISTANCE -3L
#define TEST_FRAME_NUMBER -2
#define TEST_IMPACT_SPEED 43981U
#define TEST_PLAYER_X_POSITION -4L
#define TEST_TRAVEL_DISTANCE_OFFSET 316U
#define TEST_FRAME_NUMBER_OFFSET 320U
#define TEST_IMPACT_SPEED_OFFSET 332U
#define TEST_PLAYER_X_POSITION_OFFSET 338U
#define TEST_NEGATIVE_THREE_LOW_BYTE 253U
#define TEST_NEGATIVE_TWO_LOW_BYTE 254U
#define TEST_NEGATIVE_FOUR_LOW_BYTE 252U
#define TEST_SIGN_EXTENSION_BYTE 255U
#define TEST_IMPACT_SPEED_LOW_BYTE 205U
#define TEST_IMPACT_SPEED_HIGH_BYTE 171U

static void test_object_representation(void)
{
	struct GAMESTATE state;
	legacy_u8 output[GAMESTATE_SERIALIZED_SIZE + TEST_OUTPUT_GUARD_SIZE];
	legacy_u8* representation;
	legacy_u16 index;
	legacy_u16 length;

	representation = (legacy_u8*)&state;
	for (index = 0U; index < GAMESTATE_SERIALIZED_SIZE; index++) {
		representation[index] = (legacy_u8)(index *
			TEST_PATTERN_MULTIPLIER + TEST_PATTERN_INCREMENT);
	}
	memset(output, TEST_OUTPUT_GUARD_BYTE, sizeof(output));
	length = gamestate_serialize(output + TEST_OUTPUT_PREFIX_SIZE, &state);
	assert(length == GAMESTATE_SERIALIZED_SIZE);
	assert(output[0] == TEST_OUTPUT_GUARD_BYTE);
	assert(output[GAMESTATE_SERIALIZED_SIZE + TEST_OUTPUT_PREFIX_SIZE] ==
		TEST_OUTPUT_GUARD_BYTE);
	assert(memcmp(output + TEST_OUTPUT_PREFIX_SIZE, representation,
		GAMESTATE_SERIALIZED_SIZE) == 0);
}

static void test_little_endian_fields(void)
{
	struct GAMESTATE state;
	legacy_u8 output[GAMESTATE_SERIALIZED_SIZE];

	memset(&state, 0, sizeof(state));
	state.game_travDist = TEST_TRAVEL_DISTANCE;
	state.game_frame = TEST_FRAME_NUMBER;
	state.game_impactSpeed = TEST_IMPACT_SPEED;
	state.playerstate.car_posWorld1.lx = TEST_PLAYER_X_POSITION;
	gamestate_serialize(output, &state);

	assert(output[TEST_TRAVEL_DISTANCE_OFFSET] ==
		TEST_NEGATIVE_THREE_LOW_BYTE);
	assert(output[TEST_TRAVEL_DISTANCE_OFFSET + 1U] ==
		TEST_SIGN_EXTENSION_BYTE);
	assert(output[TEST_TRAVEL_DISTANCE_OFFSET + 2U] ==
		TEST_SIGN_EXTENSION_BYTE);
	assert(output[TEST_TRAVEL_DISTANCE_OFFSET + 3U] ==
		TEST_SIGN_EXTENSION_BYTE);
	assert(output[TEST_FRAME_NUMBER_OFFSET] == TEST_NEGATIVE_TWO_LOW_BYTE);
	assert(output[TEST_FRAME_NUMBER_OFFSET + 1U] ==
		TEST_SIGN_EXTENSION_BYTE);
	assert(output[TEST_IMPACT_SPEED_OFFSET] == TEST_IMPACT_SPEED_LOW_BYTE);
	assert(output[TEST_IMPACT_SPEED_OFFSET + 1U] ==
		TEST_IMPACT_SPEED_HIGH_BYTE);
	assert(output[TEST_PLAYER_X_POSITION_OFFSET] ==
		TEST_NEGATIVE_FOUR_LOW_BYTE);
	assert(output[TEST_PLAYER_X_POSITION_OFFSET + 1U] ==
		TEST_SIGN_EXTENSION_BYTE);
	assert(output[TEST_PLAYER_X_POSITION_OFFSET + 2U] ==
		TEST_SIGN_EXTENSION_BYTE);
	assert(output[TEST_PLAYER_X_POSITION_OFFSET + 3U] ==
		TEST_SIGN_EXTENSION_BYTE);
}

int main(void)
{
	test_object_representation();
	test_little_endian_fields();
	return 0;
}
