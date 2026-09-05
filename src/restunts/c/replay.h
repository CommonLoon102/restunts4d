#ifndef RESTUNTS_REPLAY_H
#define RESTUNTS_REPLAY_H

#include "legacy.h"

#define REPLAY_GAMEINFO_SIZE 26U
#define REPLAY_TRACK_SIZE 1802U
#define REPLAY_INPUT_OFFSET (REPLAY_GAMEINFO_SIZE + REPLAY_TRACK_SIZE)
#define REPLAY_CAR_ID_SIZE 4U
#define REPLAY_TRACK_NAME_SIZE 9U

#pragma pack (push, 1)

struct GAMEINFO {
	legacy_s8 game_playercarid[REPLAY_CAR_ID_SIZE];
	legacy_s8 game_playermaterial;
	legacy_s8 game_playertransmission;
	legacy_s8 game_opponenttype;
	legacy_s8 game_opponentcarid[REPLAY_CAR_ID_SIZE];
	legacy_s8 game_opponentmaterial;
	legacy_s8 game_opponenttransmission;
	legacy_s8 game_trackname[REPLAY_TRACK_NAME_SIZE];
	legacy_u16 game_framespersec;
	legacy_u16 game_recordedframes;
};

#pragma pack (pop)

typedef char legacy_gameinfo_must_be_26_bytes[
	(sizeof(struct GAMEINFO) == REPLAY_GAMEINFO_SIZE) ? 1 : -1];

void replay_gameinfo_decode(struct GAMEINFO* destination,
	const legacy_u8 far* source);
void replay_gameinfo_encode(legacy_u8 far* destination,
	const struct GAMEINFO* source);
legacy_u32 replay_file_size(legacy_u16 recorded_frames);
legacy_u16 replay_timeline_position(legacy_u16 frame,
	legacy_u16 recorded_frames, legacy_u16 width);
legacy_u16 replay_rewind_interpolate(legacy_u16 rewind_amount,
	legacy_u16 frames_remaining, legacy_u16 checkpoint_distance);

#endif
