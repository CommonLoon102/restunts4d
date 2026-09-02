#ifndef RESTUNTS_UI_TEXT_H
#define RESTUNTS_UI_TEXT_H

#include "legacy.h"
#include "math.h"

extern legacy_u8 far* active_font_definition;

legacy_s16 font_op(const legacy_s8* text, legacy_s16 glyph_count);
legacy_s16 font_op2(const legacy_s8* text);
legacy_s16 font_op2_alt(const legacy_s8* text);
void font_set_unk(legacy_s16 color, legacy_s16 unknown);
struct RECTANGLE* intro_draw_text(legacy_s8* text, legacy_s16 x,
	legacy_s16 y, legacy_s16 color, legacy_s16 shadow_color);

void print_int_as_string_maybe(legacy_s8* destination, legacy_s16 value,
	legacy_s16 zero_pad, legacy_s16 width);
void format_frame_as_string(legacy_s8* destination, legacy_s16 frame_count,
	legacy_s16 include_hundredths);
void parse_filepath_separators(legacy_s8* destination,
	const legacy_s8* path);
legacy_u16 legacy_near_string_length(const legacy_s8* text);

#endif
