#ifndef RESTUNTS_SHAPE2D_INTERNAL_H
#define RESTUNTS_SHAPE2D_INTERNAL_H

#include "legacy.h"

struct SPRITE;

extern legacy_s8 aWindowdefOutOfRowTableSpa[];
extern legacy_s8 aMcgaWindow[];
extern legacy_s8 aWindowReleased[];
extern struct SPRITE far* render_window_sprite;
/* Reserved seg012 storage: SPRITE structs followed by line offsets. */
extern legacy_u8* far wnd_defs;
/* Near pointer relative to seg012 for the current SPRITE in wnd_defs. */
extern legacy_s8* far next_wnd_def;
extern struct SPRITE far sprite1;
extern struct SPRITE far sprite2;
extern struct SPRITE far* mcgawndsprite;
extern struct SPRITE far* mouse_background_sprite;
extern struct SPRITE far* mouse_medium_sprite;
extern struct SPRITE far* mouse_small_sprite;
extern legacy_s8 mouse_background_dirty;
extern legacy_u8 byte_3B8FC;
extern struct SPRITE far* sprite_ptrs[4];
extern legacy_s16 word_4646A[4];
extern legacy_s16 word_46486[4];
extern legacy_u8 far incnums[];

legacy_u16 shape2d_get_word(const legacy_u8 far* source);
void shape2d_put_word(legacy_u8 far* destination, legacy_u16 value);
legacy_u16 shape2d_get_line_offset(legacy_u16 sprite_segment,
	legacy_u16 y);

#endif
