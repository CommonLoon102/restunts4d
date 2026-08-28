#include <assert.h>
#include <string.h>

#include "../../src/restunts/c/gamestate.h"

static void test_object_representation(void)
{
	struct GAMESTATE state;
	legacy_u8 output[GAMESTATE_SERIALIZED_SIZE + 2U];
	legacy_u8* representation;
	legacy_u16 index;
	legacy_u16 length;

	representation = (legacy_u8*)&state;
	for (index = 0U; index < GAMESTATE_SERIALIZED_SIZE; index++) {
		representation[index] = (legacy_u8)(index * 37U + 11U);
	}
	memset(output, 0x5A, sizeof(output));
	length = gamestate_serialize(output + 1, &state);
	assert(length == GAMESTATE_SERIALIZED_SIZE);
	assert(output[0] == 0x5AU);
	assert(output[GAMESTATE_SERIALIZED_SIZE + 1U] == 0x5AU);
	assert(memcmp(output + 1, representation,
		GAMESTATE_SERIALIZED_SIZE) == 0);
}

static void test_little_endian_fields(void)
{
	struct GAMESTATE state;
	legacy_u8 output[GAMESTATE_SERIALIZED_SIZE];

	memset(&state, 0, sizeof(state));
	state.game_travDist = -3L;
	state.game_frame = -2;
	state.game_impactSpeed = 0xABCDU;
	state.playerstate.car_posWorld1.lx = -4L;
	gamestate_serialize(output, &state);

	assert(output[0x13CU] == 0xFDU);
	assert(output[0x13DU] == 0xFFU);
	assert(output[0x13EU] == 0xFFU);
	assert(output[0x13FU] == 0xFFU);
	assert(output[0x140U] == 0xFEU);
	assert(output[0x141U] == 0xFFU);
	assert(output[0x14CU] == 0xCDU);
	assert(output[0x14DU] == 0xABU);
	assert(output[0x152U] == 0xFCU);
	assert(output[0x153U] == 0xFFU);
	assert(output[0x154U] == 0xFFU);
	assert(output[0x155U] == 0xFFU);
}

int main(void)
{
	test_object_representation();
	test_little_endian_fields();
	return 0;
}
