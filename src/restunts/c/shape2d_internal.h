#ifndef RESTUNTS_SHAPE2D_INTERNAL_H
#define RESTUNTS_SHAPE2D_INTERNAL_H

#include "legacy.h"

legacy_u16 shape2d_get_word(const legacy_u8 far* source);
void shape2d_put_word(legacy_u8 far* destination, legacy_u16 value);
legacy_u16 shape2d_get_line_offset(legacy_u16 sprite_segment,
	legacy_u16 y);

#endif
