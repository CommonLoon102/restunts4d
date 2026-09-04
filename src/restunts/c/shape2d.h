#ifndef RESTUNTS_SHAPE2D_H
#define RESTUNTS_SHAPE2D_H

#include <stddef.h>
#include "legacy.h"

#pragma pack (push, 1)

/* Header of a VGA/EGA bitmap resource, followed directly by the pixel data.
   The layout is the on-disk resource format, so it is fixed. centre_x and
   centre_y are the anchor a shape is drawn around; the unknown bytes carry
   the channel mapping and the transposed/interlaced flags. */
struct SHAPE2D {
	legacy_u16 width;
	legacy_u16 height;
	legacy_u16 centre_x;
	legacy_u16 centre_y;
	legacy_u16 position_x;
	legacy_u16 position_y;
	legacy_u8 unknown[4];
};

#pragma pack (pop)

#define SHAPE2D_HEADER_SIZE (sizeof(struct SHAPE2D))

typedef char shape2d_header_must_be_16_bytes[
	(sizeof(struct SHAPE2D) == 16) ? 1 : -1];
typedef char shape2d_centre_x_must_be_at_04[
	(offsetof(struct SHAPE2D, centre_x) == 4) ? 1 : -1];
typedef char shape2d_position_x_must_be_at_08[
	(offsetof(struct SHAPE2D, position_x) == 8) ? 1 : -1];
typedef char shape2d_unknown_must_be_at_0C[
	(offsetof(struct SHAPE2D, unknown) == 12) ? 1 : -1];

#pragma pack (push, 1)
struct SPRITE {
	struct SHAPE2D far* sprite_bitmapptr;
	legacy_u16 sprite_unk1;
	legacy_u16 sprite_unk2;
	legacy_u16 sprite_unk3;
	legacy_u8* sprite_lineofs;
	legacy_u16 sprite_left;
	legacy_u16 sprite_right;
	legacy_u16 sprite_top;
	legacy_u16 sprite_height;
	legacy_u16 sprite_pitch;
	legacy_u16 sprite_unk4;
	legacy_u16 sprite_width2;
	legacy_u16 sprite_left2;
	legacy_u16 sprite_widthsum;
};
#pragma pack (pop)

legacy_u16 shape2d_get_width(const struct SHAPE2D far* shape);
legacy_u16 shape2d_get_height(const struct SHAPE2D far* shape);
legacy_u16 shape2d_get_unk1(const struct SHAPE2D far* shape);
legacy_u16 shape2d_get_unk2(const struct SHAPE2D far* shape);
legacy_u16 shape2d_get_pos_x(const struct SHAPE2D far* shape);
legacy_u16 shape2d_get_pos_y(const struct SHAPE2D far* shape);
legacy_u16 shape2d_anchored_x(const struct SHAPE2D far* shape, legacy_s16 x);
legacy_u16 shape2d_anchored_y(const struct SHAPE2D far* shape, legacy_s16 y);

/* SPRITE contains both 16-bit near and far pointers. */
#if defined(__BORLANDC__)
typedef char legacy_sprite_must_be_30_bytes[
	(sizeof(struct SPRITE) == 30) ? 1 : -1];
#endif

struct SPRITE far* sprite_make_wnd(legacy_u16 width, legacy_u16 height, legacy_u16);
void sprite_free_wnd(struct SPRITE far* wndsprite);

void sprite_set_1_from_argptr(struct SPRITE far* argsprite);

void sprite_copy_2_to_1(void);
void sprite_copy_2_to_1_2(void);
void sprite_copy_2_to_1_clear(void);
void sprite_copy_wnd_to_1(void);
void sprite_copy_wnd_to_1_clear(void);

void sprite_copy_both_to_arg(struct SPRITE* argsprite);
void sprite_copy_arg_to_both(struct SPRITE* argsprite);

legacy_s16 sub_274B0(legacy_s16 left, legacy_s16 right, legacy_s16 top, legacy_s16 bottom);

void sprite_clear_1_color(legacy_u8 color);
void sub_35B76(legacy_s16 x, legacy_s16 y, legacy_s16 width,
	legacy_s16 height, legacy_s16 color);
void sprite_1_unk(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color);
void sprite_1_unk2(legacy_s16 x, legacy_s16 y, legacy_s16 width, legacy_s16 height, legacy_s16 color);
void sprite_1_unk4(legacy_s16 x1, legacy_s16 y1, legacy_s16 x2, legacy_s16 y2, legacy_s16 color);
void sprite_1_unk3(struct SHAPE2D far* shape, legacy_u16 phase);
void sub_34526(struct SHAPE2D far* shape);
void draw_filled_lines(legacy_s16* x1arr, legacy_s16* x2arr, legacy_u16 y,
	legacy_u16 numlines, legacy_u16 color);
void draw_unknown_lines(legacy_s16* x1arr, legacy_s16* x2arr, legacy_u16 y,
	legacy_u16 numlines, legacy_u16 color);
void draw_patterned_lines(legacy_s16* x1arr, legacy_s16* x2arr,
	legacy_u16 y, legacy_u16 numlines, legacy_u16 color);
void putpixel_line1_maybe(const legacy_u16* line);
void putpixel_single_maybe(legacy_s16 x, legacy_s16 y, legacy_s16 color);
void putpixel_iconMask(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void putpixel_iconFillings(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);

void sprite_putimage(struct SHAPE2D far* shape);
void sprite_shape_to_1(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void sprite_shape_to_1_alt(struct SHAPE2D far* shape);
void sub_275C6(void);
void sprite_putimage_and(struct SHAPE2D far* shape, legacy_u16 a, legacy_u16 b);
void sprite_putimage_and_alt(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void sprite_putimage_and_alt2(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void sprite_putimage_or(struct SHAPE2D far* shape, legacy_u16 a, legacy_u16 b);
void sprite_putimage_or_alt(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void sprite_putimage_transparent(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void sprite_clear_shape_alt(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void sprite_clear_shape(struct SHAPE2D far* shape);
void sub_345BC(const legacy_s8* text, legacy_s16 x, legacy_s16 y);
void shape2d_op_unk(struct SHAPE2D far* shape);
void shape2d_op_unk2(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void shape2d_op_unk3(struct SHAPE2D far* shape);
void shape2d_op_unk4(legacy_u16 offset, legacy_u16 segment);
void shape2d_op_unk5(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void shape2d_render_bmp_as_mask(struct SHAPE2D far* shape);

void shape_op_explosion(legacy_s16 scale, struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);
void sub_35E08(legacy_s16 scale, struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y);

void setup_mcgawnd1(void);
void setup_mcgawnd2(void);
void sub_35C4E(legacy_s16 source_x, legacy_s16 source_y,
	legacy_s16 width, legacy_s16 height, legacy_s16 destination_shift);

struct SHAPE2D far* file_get_shape2d(legacy_u8 far* memchunk, legacy_s16 index);

legacy_u16 file_get_res_shape_count(void far* memchunk);

void file_unflip_shape2d(legacy_u8 far* memchunk, legacy_s8 far* mempages);

void file_unflip_shape2d_pes(legacy_u8 far* memchunk, legacy_s8 far* mempages);

void file_load_shape2d_expand(legacy_u8 far* memchunk, legacy_s8 far* mempages);

legacy_u16 file_get_unflip_size(legacy_s8 far* memchunk);

legacy_u16 file_load_shape2d_expandedsize(void far* memchunk);

void file_load_shape2d_palmap_init(legacy_u8 far* pal);
void file_load_shape2d_palmap_apply(legacy_u8 far* memchunk, legacy_u8 palmap[]);

void far* file_load_shape2d_esh(void far* memchunk, const legacy_s8* str);
void parse_shape2d(void far* memchunk, void far* mempages);
void far* file_load_shape2d(const legacy_s8* shapename, legacy_s16 fatal);

void far* file_load_shape2d_fatal(const legacy_s8* shapename);
void far* file_load_shape2d_nofatal(const legacy_s8* shapename);
void far* file_load_shape2d_nofatal2(const legacy_s8* shapename);

void far* file_load_shape2d_res(const legacy_s8* resname, legacy_s16 fatal);
void far* file_load_shape2d_res_fatal(const legacy_s8* resname);
void far* file_load_shape2d_res_nofatal(const legacy_s8* resname);

#endif
