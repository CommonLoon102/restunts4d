#include <assert.h>
#include <string.h>

#include "../c/replay.h"

static void fill_gameinfo(struct GAMEINFO* gameinfo)
{
	static const legacy_u8 player_car[4] = { 'A', 'B', 'C', 'D' };
	static const legacy_u8 opponent_car[4] = { 0xFFU, 0U, 0x80U, 0x55U };
	static const legacy_u8 track_name[9] = {
		'T', 'R', 'A', 'C', 'K', 0U, 0x7FU, 0x80U, 0xFFU
	};
	legacy_u16 index;

	for (index = 0U; index < 4U; index++) {
		gameinfo->game_playercarid[index] =
			LEGACY_S8_FROM_BITS(player_car[index]);
		gameinfo->game_opponentcarid[index] =
			LEGACY_S8_FROM_BITS(opponent_car[index]);
	}
	gameinfo->game_playermaterial = LEGACY_S8_FROM_BITS(0x81U);
	gameinfo->game_playertransmission = 0x7F;
	gameinfo->game_opponenttype = 6;
	gameinfo->game_opponentmaterial = 3;
	gameinfo->game_opponenttransmission = 1;
	for (index = 0U; index < 9U; index++) {
		gameinfo->game_trackname[index] =
			LEGACY_S8_FROM_BITS(track_name[index]);
	}
	gameinfo->game_framespersec = 0x1234U;
	gameinfo->game_recordedframes = 0xABCDU;
}

static void assert_gameinfo_equal(const struct GAMEINFO* left,
	const struct GAMEINFO* right)
{
	legacy_u16 index;

	for (index = 0U; index < 4U; index++) {
		assert(left->game_playercarid[index] ==
			right->game_playercarid[index]);
		assert(left->game_opponentcarid[index] ==
			right->game_opponentcarid[index]);
	}
	assert(left->game_playermaterial == right->game_playermaterial);
	assert(left->game_playertransmission == right->game_playertransmission);
	assert(left->game_opponenttype == right->game_opponenttype);
	assert(left->game_opponentmaterial == right->game_opponentmaterial);
	assert(left->game_opponenttransmission == right->game_opponenttransmission);
	for (index = 0U; index < 9U; index++) {
		assert(left->game_trackname[index] ==
			right->game_trackname[index]);
	}
	assert(left->game_framespersec == right->game_framespersec);
	assert(left->game_recordedframes == right->game_recordedframes);
}

static void test_replay_ui_math(void)
{
	legacy_u16 amount;
	legacy_u16 distance;
	legacy_u16 remaining;

	assert(replay_timeline_position(0U, 0U, 110U) == 0U);
	assert(replay_timeline_position(50U, 100U, 110U) == 55U);
	assert(replay_timeline_position(100U, 100U, 110U) == 110U);
	assert(replay_timeline_position(101U, 100U, 110U) == 110U);

	assert(replay_rewind_interpolate(1U, 99U, 100U) == 0U);
	assert(replay_rewind_interpolate(100U, 50U, 100U) == 50U);
	assert(replay_rewind_interpolate(12000U, 50U, 100U) == 6000U);
	assert(replay_rewind_interpolate(5U, 1U, 0U) == 0U);

	for (amount = 1U; amount <= 100U; amount++) {
		for (distance = 1U; distance <= 100U; distance++) {
			for (remaining = 0U; remaining <= distance; remaining++) {
				assert(replay_rewind_interpolate(amount, remaining,
					distance) <= amount);
			}
		}
	}
}

int main(void)
{
	static const legacy_u8 expected[REPLAY_GAMEINFO_SIZE] = {
		'A', 'B', 'C', 'D', 0x81U, 0x7FU, 6U,
		0xFFU, 0U, 0x80U, 0x55U, 3U, 1U,
		'T', 'R', 'A', 'C', 'K', 0U, 0x7FU, 0x80U, 0xFFU,
		0x34U, 0x12U, 0xCDU, 0xABU
	};
	struct GAMEINFO encoded;
	struct GAMEINFO decoded;
	legacy_u8 storage[REPLAY_GAMEINFO_SIZE + 1U];
	legacy_u8* bytes;

	bytes = storage + 1;
	fill_gameinfo(&encoded);
	replay_gameinfo_encode(bytes, &encoded);
	assert(memcmp(bytes, expected, REPLAY_GAMEINFO_SIZE) == 0);

	memset(&decoded, 0, sizeof(decoded));
	replay_gameinfo_decode(&decoded, bytes);
	assert_gameinfo_equal(&encoded, &decoded);
	test_replay_ui_math();
	return 0;
}
