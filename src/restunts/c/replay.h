#ifndef RESTUNTS_REPLAY_H
#define RESTUNTS_REPLAY_H

#include "legacy.h"

#ifdef RESTUNTS_SDL
#ifndef far
#define far
#endif
#endif

#define REPLAY_GAMEINFO_SIZE 26U

#pragma pack (push, 1)

struct GAMEINFO {
	legacy_s8 game_playercarid[4];
	legacy_s8 game_playermaterial;
	legacy_s8 game_playertransmission;
	legacy_s8 game_opponenttype;
	legacy_s8 game_opponentcarid[4];
	legacy_s8 game_opponentmaterial;
	legacy_s8 game_opponenttransmission;
	legacy_s8 game_trackname[9];
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

#endif
