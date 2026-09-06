#ifndef RESTUNTS_SHAPE2D_INTERNAL_H
#define RESTUNTS_SHAPE2D_INTERNAL_H

#include "legacy.h"

enum SHAPE2D_RASTER_OPERATION {
	SHAPE2D_RASTER_AND = 0,
	SHAPE2D_RASTER_OR = 1,
	SHAPE2D_RASTER_COPY = 2,
	SHAPE2D_RASTER_MAP = 3
};

struct SPRITE;

extern legacy_s8 window_row_table_overflow_message[];
extern legacy_s8 video_window_resource_name[];
extern legacy_s8 window_release_order_message[];
extern struct SPRITE far* render_window_sprite;
/* Reserved seg012 storage: SPRITE structs followed by line offsets. */
extern legacy_u8* far wnd_defs;
/* Near pointer relative to seg012 for the current SPRITE in wnd_defs. */
extern legacy_s8* far next_wnd_def;
extern struct SPRITE far drawing_sprite;
extern struct SPRITE far screen_sprite;
extern struct SPRITE far* mcga_backbuffer_sprite;
extern struct SPRITE far* mouse_background_sprite;
extern struct SPRITE far* mouse_medium_sprite;
extern struct SPRITE far* mouse_small_sprite;
extern legacy_s8 mouse_background_dirty;
extern legacy_u8 sprite_background_stack_depth;
extern struct SPRITE far* sprite_ptrs[4];
extern legacy_s16 sprite_background_saved_x[4];
extern legacy_s16 sprite_background_saved_y[4];
extern legacy_u8 far sprite_palette_map[];
extern legacy_u16 raster_fill_pattern;
extern legacy_u16 raster_alternate_color;

legacy_u16 shape2d_get_word(const legacy_u8 far* source);
void shape2d_put_word(legacy_u8 far* destination, legacy_u16 value);
legacy_u16 shape2d_get_line_offset(legacy_u16 sprite_segment,
	legacy_u16 y);

#endif
