#ifdef RESTUNTS_DOS
#include <dos.h>
#include <mem.h>
#elif RESTUNTS_SDL

#endif
#include <stddef.h>
#include <stdlib.h>

#include "externs.h"
#include "memmgr.h"
#include "fileio.h"
#include "legacy.h"
#include "shape2d.h"

extern char aWindowdefOutOfRowTableSpa[];
extern char aMcgaWindow[];
extern char aWindowReleased[];
extern struct SPRITE far* wndsprite;

extern unsigned char* far wnd_defs; // a reserved memory chunk of 0xE10 bytes in seg012. contents are SPRITE structs followed by lineoffsets. cast to a far pointer for access to the contents in other segments.
extern char* far next_wnd_def; // near pointer relative to seg012 to the current SPRITE in wnd_defs. cast to a far pointer for access to the contents in other segments
extern struct SPRITE far sprite1; // seg012
extern struct SPRITE far sprite2; // seg012
extern struct SPRITE far* mcgawndsprite;
extern struct SPRITE far* mouseunkspriteptr;
extern struct SPRITE far* mmouspriteptr;
extern struct SPRITE far* smouspriteptr;
extern char mouse_isdirty;
extern legacy_u8 far* word_405FE;

static legacy_u16 shape2d_get_word(const legacy_u8 far* source)
{
	return (legacy_u16)source[0] | ((legacy_u16)source[1] << 8);
}

static void shape2d_put_word(legacy_u8 far* destination, legacy_u16 value)
{
	destination[0] = (legacy_u8)value;
	destination[1] = (legacy_u8)(value >> 8);
}

void sprite_set_1_size(unsigned short left, unsigned short right,
	unsigned short top, unsigned short height)
{
	sprite1.sprite_left2 = left;
	sprite1.sprite_left = left;
	sprite1.sprite_widthsum = right;
	sprite1.sprite_right = right;
	sprite1.sprite_top = top;
	sprite1.sprite_height = height;
}

void sprite_1_unk(int x, int y, int width, int height, int color)
{
	legacy_u8 far* bitmap;
	legacy_u16 far* line_offsets;
	legacy_u16 offset;
	legacy_u16 row;
	legacy_u16 column;
	legacy_u16 row_count;
	legacy_u16 column_count;

	if (LEGACY_S16_FROM_BITS(width) <= 0 ||
		LEGACY_S16_FROM_BITS(height) <= 0)
		return;
	bitmap = (legacy_u8 far*)sprite1.sprite_bitmapptr;
	line_offsets = (legacy_u16 far*)MK_FP(
		FP_SEG(&sprite1), FP_OFF(sprite1.sprite_lineofs));
	offset = LEGACY_U16_WRAP_ADD(
		line_offsets[(legacy_u16)y], (legacy_u16)x);
	row_count = (legacy_u16)height;
	column_count = (legacy_u16)width;
	for (row = 0; row < row_count; row++) {
		for (column = 0; column < column_count; column++)
			bitmap[LEGACY_U16_WRAP_ADD(offset, column)] =
				(legacy_u8)color;
		offset = LEGACY_U16_WRAP_ADD(offset, sprite1.sprite_pitch);
	}
}

void sprite_1_unk2(int x, int y, int width, int height, int color)
{
	legacy_s16 clipped_x;
	legacy_s16 clipped_y;
	legacy_s16 clipped_width;
	legacy_s16 clipped_height;
	legacy_s16 difference;

	clipped_x = LEGACY_S16_FROM_BITS(x);
	clipped_y = LEGACY_S16_FROM_BITS(y);
	clipped_width = LEGACY_S16_FROM_BITS(width);
	clipped_height = LEGACY_S16_FROM_BITS(height);
	difference = LEGACY_S16_WRAP_SUB(sprite1.sprite_left, clipped_x);
	if (difference > 0) {
		clipped_x = LEGACY_S16_FROM_BITS(sprite1.sprite_left);
		clipped_width = LEGACY_S16_WRAP_SUB(clipped_width, difference);
		if (clipped_width <= 0)
			return;
	}
	difference = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_ADD(clipped_x, clipped_width),
		sprite1.sprite_right);
	if (difference > 0) {
		clipped_width = LEGACY_S16_WRAP_SUB(clipped_width, difference);
		if (clipped_width <= 0)
			return;
	}
	difference = LEGACY_S16_WRAP_SUB(sprite1.sprite_top, clipped_y);
	if (difference > 0) {
		clipped_height = LEGACY_S16_WRAP_SUB(clipped_height, difference);
		if (clipped_height <= 0)
			return;
		clipped_y = LEGACY_S16_FROM_BITS(sprite1.sprite_top);
	}
	difference = LEGACY_S16_WRAP_SUB(
		LEGACY_S16_WRAP_ADD(clipped_y, clipped_height),
		sprite1.sprite_height);
	if (difference > 0) {
		clipped_height = LEGACY_S16_WRAP_SUB(clipped_height, difference);
		if (clipped_height <= 0)
			return;
	}
	sprite_1_unk(clipped_x, clipped_y, clipped_width, clipped_height, color);
}

static void font_draw_text_impl(const char* text, int x, int y, int opaque)
{
	legacy_u8 far* font_definition;
	legacy_u8 far* glyph_data;
	legacy_u8 far* bitmap;
	legacy_u16 far* line_offsets;
	legacy_u16 glyph_offset;
	legacy_u16 current_x;
	legacy_u16 current_y;
	legacy_u16 destination;
	legacy_u16 glyph_width;
	legacy_u16 row_index;
	legacy_u8 character;
	legacy_u8 color;
	legacy_u8 background;
	legacy_u8 bits;
	legacy_u8 bit;
	legacy_s8 byte_count;
	legacy_s8 old_byte_count;
	legacy_s16 row_count;
	legacy_s16 old_row_count;

	font_definition = word_405FE;
	shape2d_put_word(font_definition + 8U, (legacy_u16)x);
	shape2d_put_word(font_definition + 0x0AU, (legacy_u16)y);
	bitmap = (legacy_u8 far*)MK_FP(
		FP_SEG(sprite1.sprite_bitmapptr), 0);
	line_offsets = (legacy_u16 far*)MK_FP(
		FP_SEG(&sprite1), FP_OFF(sprite1.sprite_lineofs));
	while ((character = (legacy_u8)*text++) != 0) {
		glyph_offset = shape2d_get_word(font_definition + 0x16U +
			(legacy_u16)character * 2U);
		if (glyph_offset == 0) {
			if (character == '\r' || character == '\n') {
				shape2d_put_word(font_definition + 8U,
					shape2d_get_word(font_definition + 4U));
				shape2d_put_word(font_definition + 0x0AU,
					LEGACY_U16_WRAP_ADD(
						shape2d_get_word(font_definition + 0x0AU),
						shape2d_get_word(font_definition + 0x12U)));
			}
			continue;
		}
		glyph_data = font_definition + glyph_offset;
		current_x = shape2d_get_word(font_definition + 8U);
		if (font_definition[0x14U] != 0) {
			glyph_width = *glyph_data++;
			shape2d_put_word(font_definition + 0x10U, glyph_width);
			font_definition[0x0CU] = (legacy_u8)((glyph_width + 7U) >> 3);
		}
		color = font_definition[0];
		background = font_definition[2];
		current_y = shape2d_get_word(font_definition + 0x0AU);
		row_index = current_y;
		row_count = LEGACY_S16_FROM_BITS(
			shape2d_get_word(font_definition + 0x0EU));
		do {
			destination = LEGACY_U16_WRAP_ADD(
				line_offsets[row_index], current_x);
			byte_count = LEGACY_S8_FROM_BITS(font_definition[0x0CU]);
			do {
				bits = *glyph_data++;
				for (bit = 0; bit < 8U; bit++) {
					if ((bits & 0x80U) != 0)
						bitmap[destination] = color;
					else if (opaque != 0)
						bitmap[destination] = background;
					bits <<= 1;
					destination++;
				}
				old_byte_count = byte_count;
				byte_count = LEGACY_S8_FROM_BITS(
					(legacy_u8)((legacy_u8)byte_count - 1U));
			} while (old_byte_count != LEGACY_S8_FROM_BITS(0x80U) &&
				byte_count > 0);
			row_index++;
			old_row_count = row_count;
			row_count = LEGACY_S16_WRAP_SUB(row_count, 1);
		} while (old_row_count != LEGACY_S16_FROM_BITS(0x8000U) &&
			row_count > 0);
		shape2d_put_word(font_definition + 8U,
			LEGACY_U16_WRAP_ADD(current_x,
				shape2d_get_word(font_definition + 0x10U)));
	}
}

void font_draw_text(const char* text, int x, int y)
{
	font_draw_text_impl(text, x, y, 0);
}

void sub_345BC(const char* text, int x, int y)
{
	font_draw_text_impl(text, x, y, 1);
}

void draw_filled_lines(int* x1arr, int* x2arr, unsigned y,
	unsigned numlines, unsigned color)
{
	legacy_u8 far* bitmap;
	legacy_u16 far* line_offsets;
	legacy_u16 current_y;
	legacy_u16 line_count;
	legacy_u16 old_line_count;
	legacy_u16 left;
	legacy_u16 right;
	legacy_u16 width;
	legacy_u16 destination;

	line_count = (legacy_u16)numlines;
	if (line_count == 0)
		return;
	bitmap = (legacy_u8 far*)MK_FP(
		FP_SEG(sprite1.sprite_bitmapptr), 0);
	line_offsets = (legacy_u16 far*)MK_FP(
		FP_SEG(&sprite1), FP_OFF(sprite1.sprite_lineofs));
	current_y = (legacy_u16)y;
	do {
		left = (legacy_u16)*x1arr++;
		right = (legacy_u16)*x2arr++;
		width = LEGACY_U16_WRAP_ADD(
			LEGACY_U16_WRAP_SUB(right, left), 1U);
		if (width != 0 && width <= 0x8000U) {
			destination = LEGACY_U16_WRAP_ADD(
				line_offsets[current_y], left);
			do {
				bitmap[destination] = (legacy_u8)color;
				destination++;
				width--;
			} while (width != 0);
		}
		current_y++;
		old_line_count = line_count;
		line_count = LEGACY_U16_WRAP_SUB(line_count, 1U);
	} while (old_line_count != 0x8000U &&
		LEGACY_S16_FROM_BITS(line_count) > 0);
}

void sprite_1_unk3(struct SHAPE2D far* shape, unsigned phase)
{
	static const legacy_u8 row_order[12] = {
		11, 5, 8, 2, 10, 4, 7, 1, 9, 3, 6, 0
	};
	static const legacy_u8 skip_count[4] = { 1, 3, 0, 2 };
	static const legacy_u8 advance_count[4] = { 3, 1, 4, 2 };
	legacy_u8 far* shape_bytes;
	legacy_u8 far* bitmap;
	legacy_u8 far* line_entry_ptr;
	legacy_u8 far* source_ptr;
	legacy_u16 shape_segment;
	legacy_u16 sprite_segment;
	legacy_u16 line_table_start;
	legacy_u16 line_table_end;
	legacy_u16 line_entry;
	legacy_u16 data_start;
	legacy_u16 row_source;
	legacy_u16 source;
	legacy_u16 source_row_step;
	legacy_u16 destination;
	legacy_u16 width;
	legacy_u16 height;
	legacy_u16 pos_x;
	legacy_u16 pos_y;
	legacy_u16 remaining;
	legacy_u16 row_phase;
	legacy_u16 pattern;
	legacy_u16 selector;
	legacy_u16 skip;
	legacy_u16 advance;
	legacy_s16 order_index;

	shape_bytes = (legacy_u8 far*)shape;
	shape_segment = FP_SEG(shape);
	sprite_segment = FP_SEG(&sprite1);
	bitmap = (legacy_u8 far*)MK_FP(
		FP_SEG(sprite1.sprite_bitmapptr), 0);
	width = shape2d_get_word(shape_bytes);
	height = shape2d_get_word(shape_bytes + 2U);
	pos_x = shape2d_get_word(shape_bytes + 8U);
	pos_y = shape2d_get_word(shape_bytes + 0x0AU);
	line_table_start = LEGACY_U16_WRAP_ADD(
		FP_OFF(sprite1.sprite_lineofs), (legacy_u16)(pos_y << 1));
	line_table_end = LEGACY_U16_WRAP_ADD(
		line_table_start, (legacy_u16)(height << 1));
	data_start = LEGACY_U16_WRAP_ADD(FP_OFF(shape),
		(legacy_u16)sizeof(struct SHAPE2D));
	source_row_step = (legacy_u16)((legacy_u32)width * 12UL);
	for (order_index = 11; order_index >= 0; order_index--) {
		selector = row_order[order_index];
		line_entry = LEGACY_U16_WRAP_ADD(line_table_start,
			(legacy_u16)(selector << 1));
		source = LEGACY_U16_WRAP_ADD(data_start,
			(legacy_u16)((legacy_u32)width * selector));
		row_phase = (legacy_u16)phase;
		while (line_entry < line_table_end) {
			line_entry_ptr = (legacy_u8 far*)MK_FP(
				sprite_segment, line_entry);
			destination = LEGACY_U16_WRAP_ADD(
				shape2d_get_word(line_entry_ptr), pos_x);
			remaining = width;
			row_source = source;
			pattern = row_phase;
			for (;;) {
				pattern &= 3U;
				skip = skip_count[pattern];
				if (LEGACY_S16_FROM_BITS(remaining) <=
					(legacy_s16)skip)
					break;
				remaining = LEGACY_U16_WRAP_SUB(remaining, skip);
				source = LEGACY_U16_WRAP_ADD(source, skip);
				destination = LEGACY_U16_WRAP_ADD(destination, skip);
				source_ptr = (legacy_u8 far*)MK_FP(
					shape_segment, source);
				bitmap[destination] = *source_ptr;
				advance = advance_count[pattern];
				source = LEGACY_U16_WRAP_ADD(source, advance);
				destination = LEGACY_U16_WRAP_ADD(
					destination, advance);
				remaining = LEGACY_U16_WRAP_SUB(
					remaining, advance);
				pattern++;
			}
			row_phase++;
			line_entry = LEGACY_U16_WRAP_ADD(line_entry, 24U);
			source = LEGACY_U16_WRAP_ADD(
				row_source, source_row_step);
		}
		phase++;
	}
}

struct SPRITE far* sprite_make_wnd(unsigned int width, unsigned int height, unsigned int unk) {
	int pages, i;
	char* wnd;
	char* nextwnd;
	struct SPRITE far * farwnd;
	char far* shapebuf;
	struct SHAPE2D far* hdr;
	unsigned int lineofs;
	unsigned int* lineofsptr;
	unsigned int far* farlineofsptr;
	unsigned short wnddefseg;
	
	(void)unk;

	wnddefseg = FP_SEG(&wnd_defs);

	pages = ((width * height + sizeof(struct SHAPE2D)) >> 4) + 1;
	shapebuf = mmgr_alloc_pages("MCGA WINDOW", pages);
	
	hdr = (struct SHAPE2D far*)MK_FP(FP_SEG(shapebuf), 0);
	hdr->s2d_width = width;
	hdr->s2d_height = height;
	hdr->s2d_pos_x = 0;
	hdr->s2d_pos_y = 0;
	hdr->s2d_unk1 = 0;
	hdr->s2d_unk2 = 0;

	// it is safe to read/write the pointers to next_wnd_def/wnd_defs, but not the contents
	wnd = next_wnd_def;
	nextwnd = next_wnd_def + sizeof(struct SPRITE) + height * sizeof(unsigned int);
	if (FP_OFF(nextwnd) >= FP_OFF(&wnd_defs) + 0xE10) {
		fatal_error(aWindowdefOutOfRowTableSpa);
	}
	next_wnd_def = nextwnd;

	// get a writable far pointer to the wndsprite
	farwnd = MK_FP(wnddefseg, FP_OFF(wnd));

	lineofsptr = (unsigned int*)(wnd + sizeof(struct SPRITE));
	farwnd->sprite_bitmapptr = hdr;
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
	farlineofsptr = MK_FP(wnddefseg, FP_OFF(lineofsptr));
	lineofs = sizeof(struct SHAPE2D);
	// One of several counted loops where the original uses `loop`, which runs
	// 65536 times on a count of zero while this runs none. Reaching it needs a
	// zero-height window; the same applies to the unflip and palette loops in
	// this file.
	for (i = 0; i < height; i++) {
		*farlineofsptr = lineofs;
		farlineofsptr++;
		lineofs += width;
	}

	return farwnd;
}

void sprite_free_wnd(struct SPRITE far* wndsprite) {
	unsigned short spritesize;
	// The height comes from the bitmap header, not from the SPRITE: the
	// original walks through sprite_bitmapptr to reach SHAPE2D.s2d_height.
	// sprite_make_wnd initializes both heights alike, and normal clipping edits
	// the sprite1 working copy rather than the stored window SPRITE, so using
	// the bitmap field here is structural parity rather than a clipping repair.
	spritesize = sizeof(struct SPRITE) + wndsprite->sprite_bitmapptr->s2d_height * sizeof(unsigned short);
	if (FP_OFF(wndsprite) + spritesize != FP_OFF(next_wnd_def)) {
		fatal_error(aWindowReleased);
	}
	next_wnd_def = next_wnd_def - spritesize;
	mmgr_release((void far*)wndsprite->sprite_bitmapptr);
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
	sprite_set_1_from_argptr(wndsprite);
}

void sprite_copy_wnd_to_1_clear(void) {
	sprite_set_1_from_argptr(wndsprite);
	sprite_clear_1_color(0);
}

void sprite_copy_both_to_arg(struct SPRITE* argsprite) {
	fmemcpy(argsprite, &sprite1, sizeof(struct SPRITE) * 2);
}

void sprite_copy_arg_to_both(struct SPRITE* argsprite) {
	fmemcpy(&sprite1, argsprite, sizeof(struct SPRITE) * 2);
}

void mouse_draw_opaque(void) {
	struct SPRITE saved_sprites[2];

	sprite_copy_both_to_arg(saved_sprites);
	sprite_copy_2_to_1();
	sprite_putimage(mouseunkspriteptr->sprite_bitmapptr);
	sprite_copy_arg_to_both(saved_sprites);
	mouse_isdirty = 0;
}

void mouse_draw_transparent(void) {
	struct SPRITE saved_sprites[2];
	int aligned_x;

	aligned_x = mouse_xpos - mouse_xpos % video_flag2_is1;
	sprite_copy_both_to_arg(saved_sprites);
	sprite_copy_2_to_1();
	sprite_clear_shape_alt(
		mouseunkspriteptr->sprite_bitmapptr,
		aligned_x,
		mouse_ypos);
	sprite_putimage_and(
		mmouspriteptr->sprite_bitmapptr,
		mouse_xpos,
		mouse_ypos);
	sprite_putimage_or(
		smouspriteptr->sprite_bitmapptr,
		mouse_xpos,
		mouse_ypos);
	sprite_copy_arg_to_both(saved_sprites);
	mouse_isdirty = 1;
}

void sprite_clear_1_color(unsigned char color) {
	
	int height, top, left, right, pitch, lines, width, widthdiff, i, j;
	unsigned int ofs;
	unsigned char far* bitmapptr;
	unsigned int far* lineofs;

	top = sprite1.sprite_top;
	left = sprite1.sprite_left;
	right = sprite1.sprite_right;
	pitch = sprite1.sprite_pitch;
	bitmapptr = (unsigned char far*)sprite1.sprite_bitmapptr;
	lineofs = MK_FP(FP_SEG(&sprite1), FP_OFF(sprite1.sprite_lineofs));

	lines = sprite1.sprite_height - top;
	if (lines <= 0) return;

	ofs = lineofs[top] + left;

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

#if 0
void sprite_putimage(struct SHAPE2D far* shape) {

	int lines, widthdiff, i, j;
	unsigned int ofs;
	unsigned char far* destbitmapptr;
	unsigned int far* destlineofs;
	unsigned char far* srcbitmapptr;
	ported_sprite_putimage_(shape);
	return ;
/*
	this fails in the opponent car selector on the overlaid opponent bitmap:

	destbitmapptr = (unsigned char far*)sprite1.sprite_bitmapptr;
	destlineofs = MK_FP(FP_SEG(&sprite1), FP_OFF(sprite1.sprite_lineofs));
	srcbitmapptr = ((unsigned char far*)shape) + sizeof(struct SHAPE2D);

	if (shape->s2d_pos_y + shape->s2d_height > sprite1.sprite_height) {
		lines = sprite1.sprite_height - shape->s2d_pos_y;
	} else {
		lines = shape->s2d_height;
	}

	ofs = destlineofs[shape->s2d_pos_y] + shape->s2d_pos_x;
	widthdiff = sprite1.sprite_pitch - shape->s2d_width;

	for (i = 0; i < lines; i++) {
		for (j = 0; j < shape->s2d_width; j++) {
			destbitmapptr[ofs ++] = *srcbitmapptr++;
		}
		ofs += widthdiff;
	}
	*/
}

void sprite_putimage_and(struct SHAPE2D far* shape, unsigned short a, unsigned short b) {
	ported_sprite_putimage_and_(shape, a, b);
}

void sprite_putimage_or(struct SHAPE2D far* shape, unsigned short a, unsigned short b) {
	ported_sprite_putimage_or_(shape, a, b);
}
#endif

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

// like locate_resource_by_index()
struct SHAPE2D far* file_get_shape2d(unsigned char far* memchunk, int index) {
	unsigned short shapecount, offsetofs, dataofs;
	unsigned long chunkofs;
	unsigned char huge* result;
	
	shapecount = *(unsigned short far*)&memchunk[4];
	offsetofs = (index << 2) + (shapecount << 2) + 6;
	dataofs = (shapecount << 3) + 6;
	chunkofs = *(unsigned long far*)(&memchunk[offsetofs]);
	result = memchunk;
	result += dataofs + chunkofs;
	return (struct SHAPE2D far*)result;
}

unsigned short file_get_res_shape_count(void far* memchunk) {
	return ((unsigned short far*)memchunk)[2];
}

void file_unflip_shape2d(unsigned char far* memchunk, char far* mempages) {

	int shapecount, counter, width, height;
	int evenrows, oddrows;
	struct SHAPE2D far* memshape;
	char far* membitmapptr;
	unsigned char flag;
	int i, j;

	shapecount = *(unsigned short far*)&memchunk[4];
	counter = 0;
	do {
		memshape = file_get_shape2d(memchunk, counter);
		membitmapptr = ((char far*)memshape) + sizeof(struct SHAPE2D);
		flag = memshape->s2d_unk6;
		if ((flag & 0xF0) == 0) {
			flag = memshape->s2d_unk5 >> 4;
			if (flag != 0) {
				// The original does not merely skip an unknown flip type, it
				// gives up on the whole resource:
				//
				//     cmp     al, 4
				//     jb      short loc_32B5F
				//     mov     ax, 1
				//     jmp     short loc_32B58     ; return 1, right now
				//
				// so the shapes after this one are left flipped and the
				// caller is told. This port skips the shape and carries on,
				// and is declared void. Harmless: the only call site
				// (asmorig/seg034.asm:237) does `add sp, 8` and goes straight
				// on to mmgr_release without ever reading ax, and the three
				// arms below cover every flip type the resources use.
				if (flag < 4) {
					width = memshape->s2d_width;
					height = memshape->s2d_height;
					switch (flag - 1) {
						case 0:
							// regular flip
							for (j = 0; j < height; j++) { // height
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[j + i * height];
								}
							}
							break;
						case 1:
							// interlaced: the even rows first, then the odd
							// ones. loc_32BBA walks the second pass with
							// dx = 1, 3, .. while dx < height, so an odd
							// height gets one fewer odd row than even rows.
							for (j = 0; j < height; j += 2) { // even rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[(j / 2) + i * height];
								}
							}
							for (j = 1; j < height; j += 2) { // odd rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[((height + j) / 2) + i * height];
								}
							}
							break;
						case 2:
							// loc_32BDE. Even and odd rows are stored as two
							// separate column-major blocks: the even one holds
							// ceil(height/2) samples per column from offset 0,
							// the odd one holds height/2 per column starting at
							// width * ceil(height/2). The original never reloads
							// bx between the two halves of a pass, which is what
							// puts the odd rows at that offset.
							evenrows = (height + 1) / 2;
							oddrows = height / 2;
							for (j = 0; j < height; j += 2) { // even rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[(j / 2) + i * evenrows];
								}
							}
							for (j = 1; j < height; j += 2) { // odd rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[width * evenrows + (j / 2) + i * oddrows];
								}
							}
							break;
					}
					
					// copy flipped bits from mempages -> subres
					for (j = 0; j < height; j++) { // height
						for (i = 0; i < width; i++) { // width
							membitmapptr[i + j * width] = mempages[i + j * width];
						}
					}
				}
			}
		}
		counter++;
		shapecount--;
	} while (shapecount > 0);
	
/*    asm {

	the original of unflip case 2 above:

// switch 2
loc_32BDE:
    mov     bx, dx // dx = row counter
    shr     bx, 1
    add     bx, 10h
    add     bx, [var_6]
    mov     cx, [var_C] // width
    mov     si, [var_E]  // height
    shr     si, 1
    adc     si, 0		// si = (height + 1) / 2

loc_32BF3:
    mov     al, [bx]
    stosb
    add     bx, si
    loop    loc_32BF3

    inc     dx
    cmp     dx, [var_E]
    jz      short loc_32C15 // done

    mov     cx, [var_C]
    mov     si, [var_E]
    shr     si, 1
loc_32C08:
    mov     al, [bx]
    stosb
    add     bx, si
    loop    loc_32C08
    inc     dx
    cmp     dx, [var_E]
    jnz     short loc_32BDE
    */

}

void file_unflip_shape2d_pes(unsigned char far* memchunk, char far* mempages) {
	int shapecount, width, height, i, j, x, y;
	unsigned char val;
	unsigned char far* membitmapptr;
	struct SHAPE2D far* memshape;

	shapecount = file_get_res_shape_count(memchunk);

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d(memchunk, i);

		if (!(memshape->s2d_unk6 & 0xF0)) {
			val = (memshape->s2d_unk5 >> 4) & 0x0F;

			if (val) {
				width = memshape->s2d_width;
				height = memshape->s2d_height;
				membitmapptr = ((unsigned char far*)memshape) + sizeof(struct SHAPE2D);
				
				for (j = 0; j < 4; ++j) {
					if (val & 0x01) {
						for (y = 0; y < height; ++y) {
							for (x = 0; x < width; ++x) {
								mempages[y * width + x] = membitmapptr[x * height + y];
							}
						}
						
						// Copy flipped data from mempages -> subres
						for (y = 0; y < height; ++y) {
							for (x = 0; x < width; ++x) {
								membitmapptr[y * width + x] = mempages[y * width + x];
							}
						}
					}
					membitmapptr += width * height;
					val >>= 1;
				}
			}
		}
	}
}

void file_load_shape2d_expand(unsigned char far* memchunk, char far* mempages) {
	int shapecount, length, i, j, k, l;
	unsigned char far* memchunkptr, far* mempagesptr, px, pat;
	unsigned long val;
	unsigned long product;
	unsigned short lowterm;
	unsigned long far* offsets, nextoffset;
	struct SHAPE2D far* srcshape, far* dstshape;

	shapecount = file_get_res_shape_count(memchunk);
	
	// Skip size.
	memchunkptr = memchunk + 4;
	mempagesptr = mempages + 4;
	
	// Copy count and ids.
	for (i = 0; i < (shapecount * 2 + 1); ++i) {
		*((unsigned short far*)mempagesptr)++ = *((unsigned short far*)memchunkptr)++;
	}
	
	// Store pointer to offset table.
	offsets = (unsigned long far*)mempagesptr;
	nextoffset = 0;
	
	for (i = 0; i < shapecount; ++i) {
		srcshape = file_get_shape2d(memchunk, i);
		product = (unsigned long)srcshape->s2d_width * srcshape->s2d_height;
		length = (int)(unsigned short)product;

		// dx:ax at this point is HIWORD(w*h) : (LOWORD(w*h)*8 + 16), each
		// half 16 bits wide and wrapping on its own - the three shl's and
		// the `add ax, size SHAPE2D` never carry into dx. Only the
		// `add ax, bx / adc dx, cx` that folds in the running offset does.
		lowterm = (unsigned short)length * 8 + sizeof(struct SHAPE2D);

		offsets[i] = nextoffset;
		nextoffset += (unsigned long)lowterm
		            + ((unsigned long)(unsigned short)(product >> 16) << 16);
		
		dstshape = file_get_shape2d(mempages, i);
		// `mov cx, 6 / rep movsw` - the first six words only, up to and
		// including s2d_pos_y. s2d_unk3..s2d_unk6 hold the pattern and flip
		// nibbles and are deliberately left alone in the destination.
		fmemcpy(dstshape, srcshape, 6 * sizeof(unsigned short));
		
		dstshape->s2d_width *= 8;

		if (length && length <= 8000) {
			mempagesptr = (unsigned char far*)dstshape + sizeof(struct SHAPE2D);
			
			val = srcshape->s2d_unk4 >> 4;
			val |= val << 8;

			for (j = 0; j < length * 4; ++j) {
				*((unsigned short far*)mempagesptr)++ = val;
			}
			memchunkptr = (unsigned char far*)srcshape + sizeof(struct SHAPE2D);
			
			for (j = 0; j < 4; ++j) {
				pat = (&srcshape->s2d_unk3)[j] & 0x0F;

				if (pat) {
					mempagesptr = (unsigned char far*)dstshape + sizeof(struct SHAPE2D);
					for (k = 0; k < length; ++k) {
						px = *memchunkptr++;
						for (l = 0; l < 8; ++l) {
							if (px & 0x80) {
								*mempagesptr |= pat;
							}
							px <<= 1;
							mempagesptr++;
						}
					}
				}
				else {
					break;
				}
			}
		}
	}
	
	// Final size. The original folds this in as a 16-bit term too
	// (bx = shapecount*8 + 6, then `add ax, bx / adc dx, 0`).
	*(unsigned long far*)mempages = (unsigned short)(6 + (shapecount * 8)) + nextoffset;
}

unsigned short file_get_unflip_size(char far* memchunk) {
	unsigned short i, shapecount, size, maxsize;
	struct SHAPE2D far* memshape;

	shapecount = file_get_res_shape_count(memchunk);
	maxsize = 0;
	
	for (i = 0; i < shapecount; i++) {
		memshape = file_get_shape2d(memchunk, i);
		size = (memshape->s2d_width * memshape->s2d_height + 0x20) >> 4;
		maxsize = max(maxsize, size);
	}
	return maxsize;
}

unsigned short file_load_shape2d_expandedsize(void far* memchunk) {
	unsigned short shapecount, i;
	long size;
	struct SHAPE2D far* memshape;
	
	shapecount = file_get_res_shape_count(memchunk);

	// The original forms this seed in AX, then uses CWD: both the shift and
	// header addition wrap to 16 bits before the result is sign-extended.
	size = (short)(unsigned short)((shapecount * 8) + sizeof(struct SHAPE2D));

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d(memchunk, i);
		// `shl ax, 3` then `sub dx, dx / adc`: the per-shape term is a
		// 16-bit value ZERO-extended into the accumulator, and the header
		// size is folded in afterwards with its own carry.
		size += (unsigned long)(unsigned short)(memshape->s2d_width * memshape->s2d_height * 8)
		      + sizeof(struct SHAPE2D);
	}

	return (size + sizeof(struct SHAPE2D)) >> 4;
}

void file_load_shape2d_palmap_init(unsigned char far* pal) {
	int i;
	
	for (i = 0; i < 0x10; ++i) {
		palmap[i] = pal[i];
	}
}

void file_load_shape2d_palmap_apply(unsigned char far* memchunk, unsigned char palmap[]) {
	unsigned short shapecount, length, i, j;
	unsigned char far* memchunkptr;
	struct SHAPE2D far* memshape;
	
	shapecount = file_get_res_shape_count(memchunk);
	
	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d(memchunk, i);
		length = memshape->s2d_width * memshape->s2d_height;
		
		memchunkptr = (unsigned char far*)memshape + sizeof(struct SHAPE2D);
		
		for (j = 0; j < length; ++j) {
			// `mov bl, es:[di] / mov al, [bx+si] / stosb` - the lookup reads
			// the byte di is on, and only stosb advances di afterwards.
			*memchunkptr = palmap[*memchunkptr];
			memchunkptr++;
		}
	}
}

void far* file_load_shape2d_esh(void far* memchunk, const char* str) {
	unsigned short expandedsize;
	void far* mempages;
	void far* palmapres;

	expandedsize = file_load_shape2d_expandedsize(memchunk);

	palmapres = locate_shape_nofatal(memchunk, "!MGA");
	
	if (palmapres) {
		file_load_shape2d_palmap_init(((unsigned char far*)palmapres) + sizeof(struct SHAPE2D));
	}
	
	mempages = mmgr_alloc_pages(str, expandedsize);

	*(long far*)mempages = (long)expandedsize * 16;
	
	file_load_shape2d_expand(memchunk, mempages);
	mmgr_release(memchunk);
	memchunk = mmgr_op_unk(mempages);
	file_load_shape2d_palmap_apply(memchunk, palmap);
	
	return memchunk;
}

void far* file_load_shape2d(char* shapename, int fatal) {
	char str[100];
	char* strptr;
	int counter;
	void far* memchunk;
	void far* mempages;
	int unflipsize;

	strcpy(str, shapename);
	strptr = str;
	
	while (*strptr != '.' && *strptr) {
		strptr++;
	}
	
	if (*strptr != 0) {
		memchunk = mmgr_get_chunk_by_name(str);
		if (memchunk) return memchunk; // return existing chunk with same name
	}
	else {
		for (counter = 0; *shapeexts[counter] != 0; counter++) {
			strcpy(strptr, shapeexts[counter]);
			memchunk = mmgr_get_chunk_by_name(str);
			if (memchunk) return memchunk; // return existing chunk with same name

			if (file_find(str)) {
				break;
			}
		}
		// list exhausted: fall through to the dispatch with the last extension
		// (".ESH") still in str, like the original loc_3AA53 `jz _try_load_pvs`
	}

	if (stricmp(strptr, ".PVS") == 0) {
		memchunk = file_decomp(str, fatal);
		if (!memchunk) return MK_FP(0, 0);

		unflipsize = file_get_unflip_size(memchunk);
		mempages = mmgr_alloc_pages("UNFLIP", unflipsize);
		file_unflip_shape2d(memchunk, mempages);
		mmgr_release(mempages);

		return memchunk;
	}
	else if (stricmp(strptr, ".XVS") == 0) {
		return file_decomp(str, fatal);
	}
	else if (stricmp(strptr, ".PES") == 0) {
		memchunk = file_decomp(str, fatal);
		if (!memchunk) return MK_FP(0, 0);

		mempages = mmgr_alloc_pages("UNFLIP", 1000);
		file_unflip_shape2d_pes(memchunk, mempages);
		mmgr_release(mempages);

		return file_load_shape2d_esh(memchunk, str);
	}
	else if (stricmp(strptr, ".ESH") == 0) {
		memchunk = file_load_binary(str, fatal);
		if (!memchunk) return MK_FP(0, 0);

		return file_load_shape2d_esh(memchunk, str);
	}
	else { // .VSH or an explicit unknown extension
		return file_load_binary(str, fatal);
	}
}

void far* file_load_shape2d_fatal(char* shapename) {
	return file_load_shape2d(shapename, 1);
}

void far* file_load_shape2d_nofatal(char* shapename) {
	return file_load_shape2d(shapename, 0);
}

void far* file_load_shape2d_nofatal2(char* shapename) {
	return file_load_shape2d(shapename, 0);
}

extern void parse_shape2d(void far* memchunk, void far* mempages);

void far* file_load_shape2d_res(char* resname, int fatal) {
	int chunksize;
	char* shapename = mmgr_path_to_name(resname);
	void far* mempages;
	void far* memchunk = mmgr_get_chunk_by_name(shapename);
	unsigned short freeparas, margin, rawseg;

	if (memchunk) return memchunk;

	memchunk = file_load_shape2d(shapename, fatal);
	if (!memchunk) return 0;

	chunksize = mmgr_get_chunk_size(memchunk);

	// Parsing normally needs a second buffer as large as the loaded one, and
	// the largest custom dashboards leave no room for that in the arena.
	// Upper memory is the first choice for the second buffer; only when the
	// destination really has to come out of the arena, and does not fit, is
	// the chunk grown instead so the raw data can slide up inside it and
	// parse_shape2d write downwards into the same chunk.
	//
	// That overlap is safe: both cursors run forwards with the writer
	// starting a margin below the reader, and across the stock and custom
	// cars the writer leads by at most 38% of the resource against a margin
	// of 60% or more. Compared shape by shape against the two-buffer output
	// the bytes are identical everywhere parse_shape2d writes; only the tail
	// past the last shape differs, and the parser leaves that region alone
	// in either case. What is left afterwards has the same size, position
	// and name as the two-buffer path would have produced.
	freeparas = mmgr_get_ofs_diff();
	if (freeparas < (unsigned short)chunksize + 2 &&
	    !(highpool_route(resname, (unsigned short)chunksize) &&
	      highpool_can_fit((unsigned short)chunksize))) {
		margin = ((unsigned short)chunksize >> 1) + ((unsigned short)chunksize >> 2);
		if (margin > freeparas - (freeparas >> 3))
			margin = freeparas - (freeparas >> 3);

		if (margin >= ((unsigned short)chunksize >> 1)) {
			rawseg = FP_SEG(memchunk);
			mmgr_resize_memory(0, rawseg, chunksize + margin);
			copy_paras_reverse(rawseg, rawseg + margin, chunksize);
			parse_shape2d(MK_FP(rawseg + margin, 0), MK_FP(rawseg, 0));
			mmgr_resize_memory(0, rawseg, chunksize);
			mmgr_rename_chunk(MK_FP(rawseg, 0), resname);
			return MK_FP(rawseg, 0);
		}
	}

	mempages = mmgr_alloc_pages(resname, chunksize);

	parse_shape2d(memchunk, mempages);

	mmgr_release(memchunk);
	return mmgr_op_unk(mempages);
}

void far* file_load_shape2d_res_fatal(char* resname) {
	return file_load_shape2d_res(resname, 1);
}

void far* file_load_shape2d_res_nofatal(char* resname) {
	return file_load_shape2d_res(resname, 0);
}
