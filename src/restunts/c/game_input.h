#ifndef RESTUNTS_GAME_INPUT_H
#define RESTUNTS_GAME_INPUT_H

#include "legacy.h"

typedef legacy_s16 (far* readchar_callback_type)(void);

void kb_reg_callback(legacy_s16 code, void (far* callback)(void));
legacy_s16 kb_parse_key(legacy_s16 code);
void nopsub_304AF(legacy_s16 code);
void nopsub_kb_set_readchar_callback(readchar_callback_type callback);
readchar_callback_type nopsub_kb_get_readchar_callback(void);
void sub_307B4(void);
legacy_s16 sub_307D2(legacy_s16 index);
legacy_s16 sub_307E3(void);
legacy_s16 nopsub_307FA(void);
void load_palandcursor(void);
legacy_s16 handle_ingame_kb_shortcuts(legacy_s16 key);
void mouse_draw_transparent_check(void);
void mouse_draw_opaque_check(void);
/* A clickable rectangle. Menus keep one array of these per screen. */
struct BUTTON_AREA {
	legacy_s16 x1;
	legacy_s16 x2;
	legacy_s16 y1;
	legacy_s16 y2;
};

legacy_s16 mouse_multi_hittest(legacy_s16 count,
	const struct BUTTON_AREA* buttons);
legacy_s16 get_kb_or_joy_flags(void);
legacy_s16 input_checking(legacy_s16 frame_delta);
legacy_s16 mouse_track_op(legacy_s16 operation, legacy_s16 x,
	legacy_s16 width, legacy_s16 y, legacy_s16 height,
	legacy_s16 selected, legacy_s16 selection_width,
	legacy_s16 item_count);
legacy_s16 input_do_checking(legacy_s16 frame_delta);
void check_input(void);
void nopsub_28F26(void);
void input_push_status(void);
void input_pop_status(void);
legacy_s16 input_repeat_check(legacy_s16 duration);
void mouse_minmax_position(legacy_s16 inset);

extern legacy_s16 input_combined_flags;

#endif
