#include "replay.h"

#define REPLAY_PLAYER_CAR_OFFSET 0U
#define REPLAY_PLAYER_MATERIAL_OFFSET 4U
#define REPLAY_PLAYER_TRANSMISSION_OFFSET 5U
#define REPLAY_OPPONENT_TYPE_OFFSET 6U
#define REPLAY_OPPONENT_CAR_OFFSET 7U
#define REPLAY_OPPONENT_MATERIAL_OFFSET 11U
#define REPLAY_OPPONENT_TRANSMISSION_OFFSET 12U
#define REPLAY_TRACK_NAME_OFFSET 13U
#define REPLAY_FRAMES_PER_SECOND_OFFSET 22U
#define REPLAY_RECORDED_FRAMES_OFFSET 24U

void replay_gameinfo_decode(struct GAMEINFO* destination,
	const legacy_u8 far* source)
{
	legacy_u16 index;

	for (index = 0U; index < 4U; index++) {
		destination->game_playercarid[index] = LEGACY_S8_FROM_BITS(
			source[REPLAY_PLAYER_CAR_OFFSET + index]);
	}
	destination->game_playermaterial = LEGACY_S8_FROM_BITS(
		source[REPLAY_PLAYER_MATERIAL_OFFSET]);
	destination->game_playertransmission = LEGACY_S8_FROM_BITS(
		source[REPLAY_PLAYER_TRANSMISSION_OFFSET]);
	destination->game_opponenttype = LEGACY_S8_FROM_BITS(
		source[REPLAY_OPPONENT_TYPE_OFFSET]);
	for (index = 0U; index < 4U; index++) {
		destination->game_opponentcarid[index] = LEGACY_S8_FROM_BITS(
			source[REPLAY_OPPONENT_CAR_OFFSET + index]);
	}
	destination->game_opponentmaterial = LEGACY_S8_FROM_BITS(
		source[REPLAY_OPPONENT_MATERIAL_OFFSET]);
	destination->game_opponenttransmission = LEGACY_S8_FROM_BITS(
		source[REPLAY_OPPONENT_TRANSMISSION_OFFSET]);
	for (index = 0U; index < 9U; index++) {
		destination->game_trackname[index] = LEGACY_S8_FROM_BITS(
			source[REPLAY_TRACK_NAME_OFFSET + index]);
	}
	destination->game_framespersec = LEGACY_READ_U16_LE(
		source + REPLAY_FRAMES_PER_SECOND_OFFSET);
	destination->game_recordedframes = LEGACY_READ_U16_LE(
		source + REPLAY_RECORDED_FRAMES_OFFSET);
}

void replay_gameinfo_encode(legacy_u8 far* destination,
	const struct GAMEINFO* source)
{
	legacy_u16 index;

	for (index = 0U; index < 4U; index++) {
		destination[REPLAY_PLAYER_CAR_OFFSET + index] =
			(legacy_u8)source->game_playercarid[index];
	}
	destination[REPLAY_PLAYER_MATERIAL_OFFSET] =
		(legacy_u8)source->game_playermaterial;
	destination[REPLAY_PLAYER_TRANSMISSION_OFFSET] =
		(legacy_u8)source->game_playertransmission;
	destination[REPLAY_OPPONENT_TYPE_OFFSET] =
		(legacy_u8)source->game_opponenttype;
	for (index = 0U; index < 4U; index++) {
		destination[REPLAY_OPPONENT_CAR_OFFSET + index] =
			(legacy_u8)source->game_opponentcarid[index];
	}
	destination[REPLAY_OPPONENT_MATERIAL_OFFSET] =
		(legacy_u8)source->game_opponentmaterial;
	destination[REPLAY_OPPONENT_TRANSMISSION_OFFSET] =
		(legacy_u8)source->game_opponenttransmission;
	for (index = 0U; index < 9U; index++) {
		destination[REPLAY_TRACK_NAME_OFFSET + index] =
			(legacy_u8)source->game_trackname[index];
	}
	LEGACY_WRITE_U16_LE(destination + REPLAY_FRAMES_PER_SECOND_OFFSET,
		source->game_framespersec);
	LEGACY_WRITE_U16_LE(destination + REPLAY_RECORDED_FRAMES_OFFSET,
		source->game_recordedframes);
}
