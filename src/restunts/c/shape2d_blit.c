#include <stddef.h>
#include "externs.h"
#include "fatal.h"
#include "legacy.h"
#include "memmgr.h"
#include "platform.h"
#include "shape2d.h"
#include "shape2d_internal.h"

struct SHAPE2D_CLIP {
	legacy_u16 source;
	legacy_u16 source_advance;
	legacy_u16 destination;
	legacy_u16 destination_advance;
	legacy_u16 width;
	legacy_u16 rows;
};

struct SHAPE2D_RLE_CURSOR {
	legacy_u16 shape_segment;
	legacy_u16 source;
	legacy_u16 remaining;
	legacy_u8 value;
	legacy_s16 literal;
};

static void shape2d_render_rle(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y, legacy_s16 operation)
{
	legacy_u8 far* shape_bytes;
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u16 shape_segment;
	legacy_u16 source;
	legacy_u16 line_entry;
	legacy_u16 destination;
	legacy_u16 width;
	legacy_u16 remaining;
	legacy_u16 old_remaining;
	legacy_u16 count;
	legacy_u8 control_bits;
	legacy_s8 control;
	legacy_u8 value;
	legacy_s16 literal;

	shape_bytes = (legacy_u8 far*)shape;
	shape_segment = dos_memory_pointer_segment(shape);
	source = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	width = shape2d_get_word(shape_bytes);
	line_entry = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(sprite1.sprite_lineofs),
		(legacy_u16)(y << 1));
	destination = LEGACY_U16_WRAP_ADD(shape2d_get_word(
		(legacy_u8 far*)dos_memory_make_pointer(dos_memory_pointer_segment(&sprite1), line_entry)), x);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	remaining = width;
	for (;;) {
		source_ptr = (legacy_u8 far*)dos_memory_make_pointer(shape_segment, source);
		control_bits = *source_ptr;
		source++;
		control = LEGACY_S8_FROM_BITS(control_bits);
		if (control == 0)
			return;
		literal = control < 0;
		if (literal != 0) {
			count = (legacy_u8)(0U - control_bits);
		} else {
			count = control_bits;
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(shape_segment, source);
			value = *source_ptr;
			source++;
		}
		do {
			if (literal != 0) {
				source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
					shape_segment, source);
				value = *source_ptr;
				source++;
			}
			if (operation == SHAPE2D_RASTER_OR)
				bitmap[destination] |= value;
			else if (operation == SHAPE2D_RASTER_COPY)
				bitmap[destination] = value;
			else
				bitmap[destination] &= value;
			destination++;
			old_remaining = remaining;
			remaining = LEGACY_U16_WRAP_SUB(remaining, 1U);
			if (old_remaining == 0x8000U ||
				LEGACY_S16_FROM_BITS(remaining) <= 0) {
				line_entry = LEGACY_U16_WRAP_ADD(line_entry, 2U);
				destination = LEGACY_U16_WRAP_ADD(shape2d_get_word(
					(legacy_u8 far*)dos_memory_make_pointer(
						dos_memory_pointer_segment(&sprite1), line_entry)), x);
				remaining = width;
			}
			count--;
		} while (count != 0);
	}
}

static void shape2d_render_rle_at_anchor(struct SHAPE2D far* shape,
	legacy_s16 x, legacy_s16 y, legacy_s16 operation)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)),
		operation);
}

void shape2d_render_bmp_as_mask(struct SHAPE2D far* shape)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU), SHAPE2D_RASTER_AND);
}

void nopsub_33AC0(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_render_rle_at_anchor(shape, x, y, SHAPE2D_RASTER_AND);
}

void nopsub_33AE4(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_render_rle(shape, (legacy_u16)x, (legacy_u16)y,
		SHAPE2D_RASTER_AND);
}

void shape2d_op_unk4(legacy_u16 offset, legacy_u16 segment)
{
	struct SHAPE2D far* shape;
	legacy_u8 far* shape_bytes;

	shape = (struct SHAPE2D far*)dos_memory_make_pointer(segment, offset);
	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU), SHAPE2D_RASTER_OR);
}

void shape2d_op_unk5(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_render_rle(shape, (legacy_u16)x, (legacy_u16)y,
		SHAPE2D_RASTER_COPY);
}

void shape2d_op_unk(struct SHAPE2D far* shape)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU), SHAPE2D_RASTER_COPY);
}

void nopsub_33DBE(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_render_rle_at_anchor(shape, x, y, SHAPE2D_RASTER_COPY);
}

struct SPRITE far* sprite_make_wnd(legacy_u16 width, legacy_u16 height, legacy_u16 unk) {
	legacy_s16 pages, i;
	legacy_s8* wnd;
	legacy_s8* nextwnd;
	struct SPRITE far * farwnd;
	legacy_s8 far* shapebuf;
	legacy_u8 far* header;
	legacy_u16 lineofs;
	legacy_u8* lineofsptr;
	legacy_u8 far* farlineofsptr;
	legacy_u16 wnddefseg;

	(void)unk;

	wnddefseg = dos_memory_pointer_segment(&wnd_defs);

	pages = ((width * height + SHAPE2D_HEADER_SIZE) >> 4) + 1;
	shapebuf = mmgr_alloc_pages("MCGA WINDOW", pages);

	header = (legacy_u8 far*)dos_memory_make_pointer(dos_memory_pointer_segment(shapebuf), 0);
	shape2d_put_word(header + SHAPE2D_WIDTH_OFFSET, width);
	shape2d_put_word(header + SHAPE2D_HEIGHT_OFFSET, height);
	shape2d_put_word(header + SHAPE2D_POS_X_OFFSET, 0U);
	shape2d_put_word(header + SHAPE2D_POS_Y_OFFSET, 0U);
	shape2d_put_word(header + SHAPE2D_UNK1_OFFSET, 0U);
	shape2d_put_word(header + SHAPE2D_UNK2_OFFSET, 0U);

	// it is safe to read/write the pointers to next_wnd_def/wnd_defs, but not the contents
	wnd = next_wnd_def;
	nextwnd = next_wnd_def + sizeof(struct SPRITE) + height * sizeof(legacy_u16);
	if (dos_memory_pointer_offset(nextwnd) >= dos_memory_pointer_offset(&wnd_defs) + 0xE10) {
		fatal_error(aWindowdefOutOfRowTableSpa);
	}
	next_wnd_def = nextwnd;

	// get a writable far pointer to the render_window_sprite
	farwnd = dos_memory_make_pointer(wnddefseg, dos_memory_pointer_offset(wnd));

	lineofsptr = (legacy_u8*)(wnd + sizeof(struct SPRITE));
	farwnd->sprite_bitmapptr = (struct SHAPE2D far*)header;
	farwnd->sprite_lineofs = lineofsptr;
	farwnd->sprite_left = 0;
	farwnd->sprite_left2 = 0;
	farwnd->sprite_right = width;
	farwnd->sprite_pitch = width;	// ??
	farwnd->sprite_top = 0;
	farwnd->sprite_height = height;
	farwnd->sprite_width2 = width;
	farwnd->sprite_widthsum = width;

	// create a writable far pointer to the line offsets
	farlineofsptr = (legacy_u8 far*)dos_memory_make_pointer(
		wnddefseg, dos_memory_pointer_offset(lineofsptr));
	lineofs = SHAPE2D_HEADER_SIZE;
	// One of several counted loops where the original uses `loop`, which runs
	// 65536 times on a count of zero while this runs none. Reaching it needs a
	// zero-height window; the same applies to the unflip and palette loops in
	// this file.
	for (i = 0; i < height; i++) {
		shape2d_put_word(farlineofsptr, lineofs);
		farlineofsptr += 2U;
		lineofs += width;
	}

	return farwnd;
}

void sprite_free_wnd(struct SPRITE far* render_window_sprite) {
	legacy_u16 spritesize;
	// The height comes from the bitmap header, not from the SPRITE: the
	// original walks through sprite_bitmapptr to reach SHAPE2D.s2d_height.
	// sprite_make_wnd initializes both heights alike, and normal clipping edits
	// the sprite1 working copy rather than the stored window SPRITE, so using
	// the bitmap field here is structural parity rather than a clipping repair.
	spritesize = sizeof(struct SPRITE) + shape2d_get_height(
		render_window_sprite->sprite_bitmapptr) * sizeof(legacy_u16);
	if (dos_memory_pointer_offset(render_window_sprite) + spritesize != dos_memory_pointer_offset(next_wnd_def)) {
		fatal_error(aWindowReleased);
	}
	next_wnd_def = next_wnd_def - spritesize;
	mmgr_release((void far*)render_window_sprite->sprite_bitmapptr);
}

void sprite_set_1_from_argptr(struct SPRITE far* argsprite) {
	fmemcpy(&sprite1, argsprite, sizeof(struct SPRITE));
}

void sprite_copy_2_to_1(void) {
	sprite_set_1_from_argptr(&sprite2);
}

void sprite_copy_2_to_1_2(void) {
	sprite_set_1_from_argptr(&sprite2);
}

void sprite_copy_2_to_1_clear(void) {
	sprite_set_1_from_argptr(&sprite2);
	sprite_clear_1_color(0);
}

void sprite_copy_wnd_to_1(void) {
	sprite_set_1_from_argptr(render_window_sprite);
}

void sprite_copy_wnd_to_1_clear(void) {
	sprite_set_1_from_argptr(render_window_sprite);
	sprite_clear_1_color(0);
}

void sprite_copy_both_to_arg(struct SPRITE* argsprite) {
	fmemcpy(argsprite, &sprite1, sizeof(struct SPRITE) * 2);
}

void sprite_copy_arg_to_both(struct SPRITE* argsprite) {
	fmemcpy(&sprite1, argsprite, sizeof(struct SPRITE) * 2);
}

legacy_s16 sub_274B0(legacy_s16 left, legacy_s16 right, legacy_s16 top, legacy_s16 bottom)
{
	struct SPRITE saved_sprites[2];
	struct SPRITE far* window;
	legacy_u16 index;
	legacy_s16 width;
	legacy_s16 height;
	legacy_s32 required;

	width = LEGACY_S16_WRAP_SUB(right, left);
	height = LEGACY_S16_WRAP_SUB(bottom, top);
	required = ((legacy_s32)width * height) /
		((legacy_s32)video_flag1_is1 * video_flag4_is1) + 0x12L;
	if (mmgr_get_res_ofs_diff_scaled() <= (legacy_u32)required)
		return 0;

	mouse_draw_opaque_check();
	window = sprite_make_wnd((legacy_u16)width, (legacy_u16)height, 0x0FU);
	index = byte_3B8FC;
	sprite_ptrs[index] = window;
	word_4646A[index] = left;
	word_46486[index] = top;
	sprite_copy_both_to_arg(saved_sprites);
	fmemcpy(trackdata12 + index * sizeof(saved_sprites),
		saved_sprites, sizeof(saved_sprites));
	sprite_copy_2_to_1();
	sprite_clear_shape_alt(window->sprite_bitmapptr, left, top);
	byte_3B8FC++;
	return 1;
}

void sub_275C6(void)
{
	struct SPRITE saved_sprites[2];
	legacy_u16 index;

	if (byte_3B8FC == 0)
		return;
	byte_3B8FC--;
	index = byte_3B8FC;
	mouse_draw_opaque_check();
	sprite_shape_to_1(sprite_ptrs[index]->sprite_bitmapptr,
		word_4646A[index], word_46486[index]);
	fmemcpy(saved_sprites,
		trackdata12 + index * sizeof(saved_sprites),
		sizeof(saved_sprites));
	sprite_copy_arg_to_both(saved_sprites);
	sprite_free_wnd(sprite_ptrs[index]);
	mouse_draw_transparent_check();
}

void mouse_draw_opaque(void) {
	struct SPRITE saved_sprites[2];

	sprite_copy_both_to_arg(saved_sprites);
	sprite_copy_2_to_1();
	sprite_putimage(mouse_background_sprite->sprite_bitmapptr);
	sprite_copy_arg_to_both(saved_sprites);
	mouse_background_dirty = 0;
}

void mouse_draw_transparent(void) {
	struct SPRITE saved_sprites[2];
	legacy_s16 aligned_x;

	aligned_x = mouse_xpos - mouse_xpos % video_flag2_is1;
	sprite_copy_both_to_arg(saved_sprites);
	sprite_copy_2_to_1();
	sprite_clear_shape_alt(
		mouse_background_sprite->sprite_bitmapptr,
		aligned_x,
		mouse_ypos);
	sprite_putimage_and(
		mouse_medium_sprite->sprite_bitmapptr,
		mouse_xpos,
		mouse_ypos);
	sprite_putimage_or(
		mouse_small_sprite->sprite_bitmapptr,
		mouse_xpos,
		mouse_ypos);
	sprite_copy_arg_to_both(saved_sprites);
	mouse_background_dirty = 1;
}

void sprite_clear_1_color(legacy_u8 color) {

	legacy_s16 height, top, left, right, pitch, lines, width, widthdiff, i, j;
	legacy_u16 ofs;
	legacy_u8 far* bitmapptr;

	top = sprite1.sprite_top;
	left = sprite1.sprite_left;
	right = sprite1.sprite_right;
	pitch = sprite1.sprite_pitch;
	bitmapptr = (legacy_u8 far*)sprite1.sprite_bitmapptr;

	lines = sprite1.sprite_height - top;
	if (lines <= 0) return;

	ofs = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), (legacy_u16)top),
		(legacy_u16)left);

	width = right - left;
	if (width <= 0) return ;

	widthdiff = pitch - width;

	for (i = 0; i < lines; i++) {
		for (j = 0; j < width; j++) {
			bitmapptr[ofs ++] = color;
		}
		ofs += widthdiff;
	}
}

static legacy_s16 shape2d_clip_blit(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y, struct SHAPE2D_CLIP* clip)
{
	legacy_u8 far* shape_bytes;
	legacy_u16 width;
	legacy_u16 height;
	legacy_u16 source;
	legacy_u16 clipped_rows;
	legacy_u16 visible;
	legacy_u16 overflow;
	legacy_u16 sprite_width;
	legacy_u16 sum;

	shape_bytes = (legacy_u8 far*)shape;
	width = shape2d_get_word(shape_bytes);
	height = shape2d_get_word(shape_bytes + 2U);
	source = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	clipped_rows = height;
	if (LEGACY_S16_FROM_BITS(y) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_top)) {
		sum = LEGACY_U16_WRAP_ADD(y, clipped_rows);
		if (LEGACY_S16_FROM_BITS(sum) <=
			LEGACY_S16_FROM_BITS(sprite1.sprite_top))
			return 0;
		visible = LEGACY_U16_WRAP_SUB(sum, sprite1.sprite_top);
		overflow = LEGACY_U16_WRAP_SUB(clipped_rows, visible);
		source = LEGACY_U16_WRAP_ADD(source,
			(legacy_u16)((legacy_u32)overflow * width));
		clipped_rows = visible;
		y = sprite1.sprite_top;
	}
	sum = LEGACY_U16_WRAP_ADD(y, clipped_rows);
	if (LEGACY_S16_FROM_BITS(sum) >
		LEGACY_S16_FROM_BITS(sprite1.sprite_height)) {
		overflow = LEGACY_U16_WRAP_SUB(sum, sprite1.sprite_height);
		if (LEGACY_S16_FROM_BITS(clipped_rows) <=
			LEGACY_S16_FROM_BITS(overflow))
			return 0;
		clipped_rows = LEGACY_U16_WRAP_SUB(clipped_rows, overflow);
	}

	visible = width;
	clip->source_advance = 0;
	if (LEGACY_S16_FROM_BITS(x) <
		LEGACY_S16_FROM_BITS(sprite1.sprite_left)) {
		sum = LEGACY_U16_WRAP_ADD(x, visible);
		if (LEGACY_S16_FROM_BITS(sum) <=
			LEGACY_S16_FROM_BITS(sprite1.sprite_left))
			return 0;
		visible = LEGACY_U16_WRAP_SUB(sum, sprite1.sprite_left);
		overflow = LEGACY_U16_WRAP_SUB(width, visible);
		source = LEGACY_U16_WRAP_ADD(source, overflow);
		sprite_width = LEGACY_U16_WRAP_SUB(
			sprite1.sprite_right, sprite1.sprite_left);
		if (LEGACY_S16_FROM_BITS(sprite1.sprite_right) <=
			LEGACY_S16_FROM_BITS(sprite1.sprite_left))
			return 0;
		if (LEGACY_S16_FROM_BITS(visible) >=
			LEGACY_S16_FROM_BITS(sprite_width))
			visible = sprite_width;
		clip->source_advance = LEGACY_U16_WRAP_SUB(width, visible);
		x = sprite1.sprite_left;
	} else {
		sum = LEGACY_U16_WRAP_ADD(x, visible);
		if (LEGACY_S16_FROM_BITS(sum) >=
			LEGACY_S16_FROM_BITS(sprite1.sprite_right)) {
			overflow = LEGACY_U16_WRAP_SUB(
				sum, sprite1.sprite_right);
			if (LEGACY_S16_FROM_BITS(visible) <=
				LEGACY_S16_FROM_BITS(overflow))
				return 0;
			visible = LEGACY_U16_WRAP_SUB(visible, overflow);
			clip->source_advance = overflow;
		}
	}
	if (LEGACY_S16_FROM_BITS(visible) <= 0)
		return 0;

	clip->source = source;
	clip->destination = LEGACY_U16_WRAP_ADD(
		shape2d_get_line_offset(dos_memory_pointer_segment(&sprite1), y), x);
	clip->destination_advance = LEGACY_U16_WRAP_SUB(
		sprite1.sprite_pitch, visible);
	clip->width = visible;
	clip->rows = clipped_rows;
	return 1;
}

static legacy_s16 shape2d_rle_next(struct SHAPE2D_RLE_CURSOR* cursor,
	legacy_u8* value)
{
	legacy_u8 far* source_ptr;
	legacy_u8 control_bits;
	legacy_s8 control;

	if (cursor->remaining == 0) {
		source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
			cursor->shape_segment, cursor->source);
		control_bits = *source_ptr;
		cursor->source++;
		control = LEGACY_S8_FROM_BITS(control_bits);
		if (control == 0)
			return 0;
		cursor->literal = control < 0;
		if (cursor->literal != 0) {
			cursor->remaining = (legacy_u8)(0U - control_bits);
		} else {
			cursor->remaining = control_bits;
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				cursor->shape_segment, cursor->source);
			cursor->value = *source_ptr;
			cursor->source++;
		}
	}
	if (cursor->literal != 0) {
		source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
			cursor->shape_segment, cursor->source);
		cursor->value = *source_ptr;
		cursor->source++;
	}
	*value = cursor->value;
	cursor->remaining--;
	return 1;
}

static void shape2d_render_rle_clipped(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y)
{
	struct SHAPE2D_CLIP clip;
	struct SHAPE2D_RLE_CURSOR cursor;
	legacy_u8 far* shape_bytes;
	legacy_u8 far* bitmap;
	legacy_u16 data_start;
	legacy_u16 skip;
	legacy_u16 count;
	legacy_u16 rows;
	legacy_u16 destination;
	legacy_u16 width;
	legacy_u16 height;
	legacy_u8 value;

	if (!shape2d_clip_blit(shape, x, y, &clip))
		return;
	shape_bytes = (legacy_u8 far*)shape;
	width = shape2d_get_word(shape_bytes);
	height = shape2d_get_word(shape_bytes + 2U);
	data_start = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(shape),
		SHAPE2D_HEADER_SIZE);
	if (clip.source == data_start && clip.source_advance == 0 &&
		clip.width == width && clip.rows == height) {
		shape2d_render_rle(shape, x, y, SHAPE2D_RASTER_COPY);
		return;
	}
	cursor.shape_segment = dos_memory_pointer_segment(shape);
	cursor.source = data_start;
	cursor.remaining = 0;
	cursor.value = 0;
	cursor.literal = 0;
	skip = LEGACY_U16_WRAP_SUB(clip.source, data_start);
	while (skip != 0) {
		if (!shape2d_rle_next(&cursor, &value))
			return;
		skip--;
	}
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	destination = clip.destination;
	rows = clip.rows;
	do {
		count = clip.width;
		do {
			if (!shape2d_rle_next(&cursor, &value))
				return;
			bitmap[destination] = value;
			destination++;
			count--;
		} while (count != 0);
		rows--;
		if (rows == 0)
			return;
		skip = clip.source_advance;
		while (skip != 0) {
			if (!shape2d_rle_next(&cursor, &value))
				return;
			skip--;
		}
		destination = LEGACY_U16_WRAP_ADD(
			destination, clip.destination_advance);
	} while (1);
}

void shape2d_op_unk2(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	shape2d_render_rle_clipped(shape, (legacy_u16)x, (legacy_u16)y);
}

void shape2d_op_unk3(struct SHAPE2D far* shape)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle_clipped(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU));
}

void nopsub_33E90(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	shape2d_render_rle_clipped(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)));
}

static void sprite_putimage_at(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y, legacy_s16 operation)
{
	struct SHAPE2D_CLIP clip;
	legacy_u8 far* source_ptr;
	legacy_u8 far* bitmap;
	legacy_u16 shape_segment;
	legacy_u16 column_count;
	legacy_u16 row_count;
	legacy_u16 old_row_count;
	legacy_u8 mapped_color;

	if (!shape2d_clip_blit(shape, x, y, &clip))
		return;
	shape_segment = dos_memory_pointer_segment(shape);
	bitmap = (legacy_u8 far*)dos_memory_make_pointer(
		dos_memory_pointer_segment(sprite1.sprite_bitmapptr), 0);
	row_count = clip.rows;
	do {
		column_count = clip.width;
		do {
			source_ptr = (legacy_u8 far*)dos_memory_make_pointer(
				shape_segment, clip.source);
			if (operation == SHAPE2D_RASTER_MAP) {
				mapped_color = incnums[*source_ptr];
				if (mapped_color != 0xFFU)
					bitmap[clip.destination] = mapped_color;
			} else if (operation == SHAPE2D_RASTER_OR) {
				bitmap[clip.destination] |= *source_ptr;
			} else if (operation == SHAPE2D_RASTER_COPY) {
				bitmap[clip.destination] = *source_ptr;
			} else {
				bitmap[clip.destination] &= *source_ptr;
			}
			clip.source++;
			clip.destination++;
			column_count--;
		} while (column_count != 0);
		clip.source = LEGACY_U16_WRAP_ADD(
			clip.source, clip.source_advance);
		clip.destination = LEGACY_U16_WRAP_ADD(
			clip.destination, clip.destination_advance);
		old_row_count = row_count;
		row_count = LEGACY_U16_WRAP_SUB(row_count, 1U);
	} while (old_row_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(row_count) > 0);
}

static void sprite_putimage_at_anchor(struct SHAPE2D far* shape,
	legacy_s16 x, legacy_s16 y, legacy_s16 operation)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_putimage_at(shape,
		LEGACY_U16_WRAP_SUB(x, shape2d_get_word(shape_bytes + 4U)),
		LEGACY_U16_WRAP_SUB(y, shape2d_get_word(shape_bytes + 6U)),
		operation);
}

void sprite_putimage(struct SHAPE2D far* shape)
{
	legacy_u8 far* shape_bytes;

	shape_bytes = (legacy_u8 far*)shape;
	sprite_putimage_at(shape,
		shape2d_get_word(shape_bytes + 8U),
		shape2d_get_word(shape_bytes + 0x0AU), SHAPE2D_RASTER_COPY);
}

void nopsub_33B98(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_putimage_at_anchor(shape, x, y, SHAPE2D_RASTER_COPY);
}

void sprite_putimage_and(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y)
{
	sprite_putimage_at(shape, x, y, SHAPE2D_RASTER_AND);
}

void sprite_putimage_or(struct SHAPE2D far* shape,
	legacy_u16 x, legacy_u16 y)
{
	sprite_putimage_at(shape, x, y, SHAPE2D_RASTER_OR);
}

void sprite_putimage_and_alt(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_putimage_at(shape, (legacy_u16)x, (legacy_u16)y,
		SHAPE2D_RASTER_COPY);
}

void sprite_putimage_and_alt2(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_putimage_at_anchor(shape, x, y, SHAPE2D_RASTER_AND);
}

void sprite_putimage_or_alt(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_putimage_at_anchor(shape, x, y, SHAPE2D_RASTER_OR);
}

void sprite_putimage_transparent(struct SHAPE2D far* shape, legacy_s16 x, legacy_s16 y)
{
	sprite_putimage_at(shape, (legacy_u16)x, (legacy_u16)y,
		SHAPE2D_RASTER_MAP);
}

void setup_mcgawnd1(void) {
	if (!mcgawndsprite) {
		mcgawndsprite = sprite_make_wnd(320, 200, 0x0F);
	}

	sprite_set_1_from_argptr(&sprite2);
	sprite_putimage(mcgawndsprite->sprite_bitmapptr);
}

void setup_mcgawnd2(void) {
	if (!mcgawndsprite) {
		mcgawndsprite = sprite_make_wnd(320, 200, 0x0F);
	}

	sprite_set_1_from_argptr(mcgawndsprite);
}
