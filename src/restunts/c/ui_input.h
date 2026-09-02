#ifndef RESTUNTS_UI_INPUT_H
#define RESTUNTS_UI_INPUT_H

#include "legacy.h"
#include "shape2d.h"

legacy_s16 call_read_line(legacy_s8* text, legacy_s16 max_characters,
	legacy_s16 x, legacy_s16 y, legacy_u32 timeout);
legacy_s16 sprite_blit_to_video(struct SPRITE far* sprite,
	legacy_s16 mode);

#endif
