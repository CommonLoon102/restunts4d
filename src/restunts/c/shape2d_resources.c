#include <stddef.h>
#include "externs.h"
#include "fileio.h"
#include "legacy.h"
#include "memmgr.h"
#include "platform.h"
#include "resource.h"
#include "shape2d.h"
#include "shape2d_internal.h"

legacy_u32 parse_shape2d_helper(void far* data)
{
	return ((legacy_u32)dos_memory_pointer_segment(data) << 4) + dos_memory_pointer_offset(data);
}

void far* parse_shape2d_helper2(legacy_u32 linear_address)
{
	return dos_memory_make_pointer((legacy_u16)(linear_address >> 4),
		(legacy_u16)linear_address & 0x0FU);
}

legacy_s16 parse_shape2d_helper3(void far* data)
{
	legacy_u8 far* source_ptr;
	legacy_u16 source_segment;
	legacy_u16 source;
	legacy_u16 count;
	legacy_u8 value;

	source_segment = dos_memory_pointer_segment(data);
	source = dos_memory_pointer_offset(data);
	source_ptr = (legacy_u8 far*)dos_memory_make_pointer(source_segment, source);
	value = *source_ptr;
	count = 0;
	for (;;) {
		source_ptr = (legacy_u8 far*)dos_memory_make_pointer(source_segment, source);
		source++;
		if (*source_ptr != value)
			return count;
		count++;
	}
}

static legacy_u8 shape2d_far_read_byte(legacy_u16 segment,
	legacy_u16 offset)
{
	return *(legacy_u8 far*)dos_memory_make_pointer(segment, offset);
}

static void shape2d_far_write_byte(legacy_u16 segment,
	legacy_u16 offset, legacy_u8 value)
{
	*(legacy_u8 far*)dos_memory_make_pointer(segment, offset) = value;
}

static void shape2d_far_write_dword(legacy_u16 segment,
	legacy_u16 offset, legacy_u32 value)
{
	shape2d_far_write_byte(segment, offset, (legacy_u8)value);
	offset++;
	shape2d_far_write_byte(segment, offset, (legacy_u8)(value >> 8));
	offset++;
	shape2d_far_write_byte(segment, offset, (legacy_u8)(value >> 16));
	offset++;
	shape2d_far_write_byte(segment, offset, (legacy_u8)(value >> 24));
}

static void shape2d_copy_wrapped(legacy_u16 source_segment,
	legacy_u16* source, legacy_u16 destination_segment,
	legacy_u16* destination, legacy_u16 count)
{
	legacy_u16 copied;

	copied = 0;
	while (LEGACY_S16_FROM_BITS(copied) <
		LEGACY_S16_FROM_BITS(count)) {
		shape2d_far_write_byte(destination_segment, *destination,
			shape2d_far_read_byte(source_segment, *source));
		(*source)++;
		(*destination)++;
		copied++;
	}
}

static void shape2d_write_run(legacy_u16 source_segment,
	legacy_u16* source, legacy_u16 destination_segment,
	legacy_u16* destination, legacy_u16 count)
{
	shape2d_far_write_byte(destination_segment, *destination,
		(legacy_u8)count);
	(*destination)++;
	shape2d_far_write_byte(destination_segment, *destination,
		shape2d_far_read_byte(source_segment, *source));
	(*destination)++;
	*source = LEGACY_U16_WRAP_ADD(*source, count);
}

void parse_shape2d(void far* memchunk, void far* mempages)
{
	struct SHAPE2D far* shape;
	void far* output_pointer;
	legacy_u32 initial_output_linear;
	legacy_u32 output_linear;
	legacy_u32 output_size;
	legacy_u16 chunk_segment;
	legacy_u16 chunk_offset;
	legacy_u16 pages_segment;
	legacy_u16 pages_offset;
	legacy_u16 offsets_offset;
	legacy_u16 output_segment;
	legacy_u16 output_offset;
	legacy_u16 source_segment;
	legacy_u16 source_offset;
	legacy_u16 scan_offset;
	legacy_u16 literal_offset;
	legacy_u16 shape_count;
	legacy_u16 shape_index;
	legacy_u16 header_size;
	legacy_u16 copied;
	legacy_u16 remaining;
	legacy_u16 literal_count;
	legacy_u16 run_count;

	chunk_segment = dos_memory_pointer_segment(memchunk);
	chunk_offset = dos_memory_pointer_offset(memchunk);
	pages_segment = dos_memory_pointer_segment(mempages);
	pages_offset = dos_memory_pointer_offset(mempages);
	shape_count = file_get_res_shape_count(memchunk);
	offsets_offset = LEGACY_U16_WRAP_ADD(pages_offset,
		LEGACY_U16_WRAP_ADD(LEGACY_U16_WRAP_MUL(shape_count, 4U), 6U));
	header_size = LEGACY_U16_WRAP_ADD(
		LEGACY_U16_WRAP_MUL(shape_count, 4U), 6U);
	copied = 0;
	while (LEGACY_S16_FROM_BITS(header_size) >
		LEGACY_S16_FROM_BITS(copied)) {
		shape2d_far_write_byte(pages_segment, pages_offset,
			shape2d_far_read_byte(chunk_segment, chunk_offset));
		chunk_offset++;
		pages_offset++;
		copied++;
	}
	output_segment = dos_memory_pointer_segment(mempages);
	output_offset = LEGACY_U16_WRAP_ADD(dos_memory_pointer_offset(mempages),
		LEGACY_U16_WRAP_ADD(LEGACY_U16_WRAP_MUL(shape_count, 8U), 6U));
	initial_output_linear = parse_shape2d_helper(
		dos_memory_make_pointer(output_segment, output_offset));

	shape_index = 0;
	while (LEGACY_S16_FROM_BITS(shape_index) <
		LEGACY_S16_FROM_BITS(shape_count)) {
		shape = file_get_shape2d((legacy_u8 far*)memchunk, shape_index);
		output_linear = parse_shape2d_helper(
			dos_memory_make_pointer(output_segment, output_offset));
		output_pointer = parse_shape2d_helper2(output_linear);
		output_segment = dos_memory_pointer_segment(output_pointer);
		output_offset = dos_memory_pointer_offset(output_pointer);
		shape2d_far_write_dword(pages_segment, offsets_offset,
			output_linear - initial_output_linear);
		offsets_offset = LEGACY_U16_WRAP_ADD(offsets_offset, 4U);

		source_segment = dos_memory_pointer_segment(shape);
		source_offset = dos_memory_pointer_offset(shape);
		shape2d_copy_wrapped(source_segment, &source_offset,
			output_segment, &output_offset,
			SHAPE2D_HEADER_SIZE);
		scan_offset = source_offset;
		literal_offset = scan_offset;
		literal_count = 0;
		remaining = LEGACY_U16_WRAP_MUL(
			shape2d_get_word((legacy_u8 far*)shape),
			shape2d_get_word((legacy_u8 far*)shape + 2U));
		scan_offset++;
		literal_count++;

		if (remaining != 0) {
			for (;;) {
				run_count = (legacy_u16)parse_shape2d_helper3(
					dos_memory_make_pointer(source_segment, scan_offset));
				if (LEGACY_S16_FROM_BITS(run_count) <= 3 &&
					literal_count < remaining) {
					scan_offset++;
					literal_count++;
					continue;
				}

				while (LEGACY_S16_FROM_BITS(literal_count) > 0x7F) {
					literal_count = LEGACY_U16_WRAP_SUB(
						literal_count, 0x7FU);
					remaining = LEGACY_U16_WRAP_SUB(
						remaining, 0x7FU);
					shape2d_far_write_byte(output_segment,
						output_offset, 0x81U);
					output_offset++;
					shape2d_copy_wrapped(source_segment,
						&literal_offset, output_segment,
						&output_offset, 0x7FU);
				}
				if (literal_count != 0) {
					shape2d_far_write_byte(output_segment,
						output_offset,
						(legacy_u8)(0U - literal_count));
					output_offset++;
					remaining = LEGACY_U16_WRAP_SUB(
						remaining, literal_count);
					shape2d_copy_wrapped(source_segment,
						&literal_offset, output_segment,
						&output_offset, literal_count);
				}

				if (run_count > remaining)
					run_count = remaining;
				while (LEGACY_S16_FROM_BITS(run_count) > 0x7F) {
					run_count = LEGACY_U16_WRAP_SUB(
						run_count, 0x7FU);
					remaining = LEGACY_U16_WRAP_SUB(
						remaining, 0x7FU);
					shape2d_write_run(source_segment, &scan_offset,
						output_segment, &output_offset, 0x7FU);
				}
				if (LEGACY_S16_FROM_BITS(run_count) > 3) {
					remaining = LEGACY_U16_WRAP_SUB(
						remaining, run_count);
					shape2d_write_run(source_segment, &scan_offset,
						output_segment, &output_offset, run_count);
				}

				literal_offset = scan_offset;
				literal_count = 0;
				scan_offset++;
				literal_count++;
				if (remaining == 0)
					break;
			}
		}
		shape2d_far_write_byte(output_segment, output_offset, 0);
		output_offset++;
		shape_index++;
	}

	output_size = parse_shape2d_helper(
		dos_memory_make_pointer(output_segment, output_offset)) -
		parse_shape2d_helper(mempages);
	if ((legacy_u8)output_size & 0x0FU)
		output_size = (output_size >> 4) + 1UL;
	else
		output_size >>= 4;
	mmgr_resize_memory(dos_memory_pointer_offset(mempages), dos_memory_pointer_segment(mempages),
		(legacy_u16)output_size);
}

static legacy_u8 far* file_get_shape2d_bytes(legacy_u8 far* memchunk,
	legacy_s16 index)
{
	return resource_file_data(memchunk, (legacy_u16)index);
}

struct SHAPE2D far* file_get_shape2d(legacy_u8 far* memchunk,
	legacy_s16 index)
{
	return (struct SHAPE2D far*)file_get_shape2d_bytes(memchunk, index);
}

void nopsub_326BA(legacy_u8 far* memchunk, legacy_u16 index, legacy_u32* result) {
	*result = LEGACY_READ_U32_LE(resource_file_identifier(memchunk, index));
}

legacy_u16 file_get_res_shape_count(void far* memchunk) {
	return resource_file_count((const legacy_u8 far*)memchunk);
}

void file_unflip_shape2d(legacy_u8 far* memchunk, legacy_s8 far* mempages) {

	legacy_s16 shapecount, counter, width, height;
	legacy_s16 evenrows, oddrows;
	legacy_u8 far* memshape;
	struct SHAPE2D far* shape_header;
	legacy_s8 far* membitmapptr;
	legacy_u8 flag;
	legacy_s16 i, j;

	shapecount = resource_file_count(memchunk);
	counter = 0;
	do {
		memshape = file_get_shape2d_bytes(memchunk, counter);
		shape_header = (struct SHAPE2D far*)memshape;
		membitmapptr = (legacy_s8 far*)memshape + SHAPE2D_HEADER_SIZE;
		flag = shape_header->unknown[3];
		if ((flag & 0xF0) == 0) {
			flag = shape_header->unknown[2] >> 4;
			if (flag != 0) {
				// The original does not merely skip an unknown flip type, it
				// gives up on the whole resource, so the shapes after this
				// one are left flipped and the
				// caller is told. This port skips the shape and carries on,
				// and is declared void. Harmless: the only call site
				// (asmorig/seg034.asm:237) does `add sp, 8` and goes straight
				// on to mmgr_release without ever reading ax, and the three
				// arms below cover every flip type the resources use.
				if (flag < 4) {
					width = shape_header->width;
					height = shape_header->height;
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
									mempages[i + j * width] = membitmapptr[
										LEGACY_S16_DIV_OR_ZERO(j, 2) + i * height];
								}
							}
							for (j = 1; j < height; j += 2) { // odd rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[
										LEGACY_S16_DIV_OR_ZERO(
											LEGACY_S16_WRAP_ADD(height, j), 2) +
										i * height];
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
							evenrows = LEGACY_S16_DIV_OR_ZERO(
								LEGACY_S16_WRAP_ADD(height, 1), 2);
							oddrows = LEGACY_S16_DIV_OR_ZERO(height, 2);
							for (j = 0; j < height; j += 2) { // even rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[
										LEGACY_S16_DIV_OR_ZERO(j, 2) + i * evenrows];
								}
							}
							for (j = 1; j < height; j += 2) { // odd rows
								for (i = 0; i < width; i++) { // width
									mempages[i + j * width] = membitmapptr[
										width * evenrows +
										LEGACY_S16_DIV_OR_ZERO(j, 2) + i * oddrows];
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
}

void file_unflip_shape2d_pes(legacy_u8 far* memchunk, legacy_s8 far* mempages) {
	legacy_s16 shapecount, width, height, i, j, x, y;
	legacy_u8 val;
	legacy_u8 far* membitmapptr;
	legacy_u8 far* memshape;
	struct SHAPE2D far* shape_header;

	shapecount = file_get_res_shape_count(memchunk);

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d_bytes(memchunk, i);
		shape_header = (struct SHAPE2D far*)memshape;

		if (!(shape_header->unknown[3] & 0xF0)) {
			val = (shape_header->unknown[2] >> 4) & 0x0F;

			if (val) {
				width = shape_header->width;
				height = shape_header->height;
				membitmapptr = memshape + SHAPE2D_HEADER_SIZE;

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

void file_load_shape2d_expand(legacy_u8 far* memchunk, legacy_s8 far* mempages) {
	legacy_s16 shapecount, length, i, j, k, l;
	legacy_u8 far* memchunkptr, far* mempagesptr, px, pat;
	legacy_u32 val;
	legacy_u32 product;
	legacy_u16 lowterm;
	legacy_u16 directory_prefix_size;
	legacy_u32 nextoffset;
	legacy_u8 far* srcshape;
	struct SHAPE2D far* source_header;
	legacy_u8 far* dstshape;
	struct SHAPE2D far* destination_header;

	shapecount = file_get_res_shape_count(memchunk);

	// Skip size.
	memchunkptr = memchunk + RESOURCE_FILE_COUNT_OFFSET;
	mempagesptr = mempages + RESOURCE_FILE_COUNT_OFFSET;

	// Copy count and ids.
	directory_prefix_size = LEGACY_U16_WRAP_ADD(RESOURCE_FILE_COUNT_SIZE,
		LEGACY_U16_WRAP_MUL(shapecount,
			RESOURCE_FILE_IDENTIFIER_SIZE));
	fmemcpy(mempagesptr, memchunkptr, directory_prefix_size);
	nextoffset = 0;

	for (i = 0; i < shapecount; ++i) {
		srcshape = file_get_shape2d_bytes(memchunk, i);
		source_header = (struct SHAPE2D far*)srcshape;
		product = (legacy_u32)source_header->width *
			source_header->height;
		length = (legacy_s16)(legacy_u16)product;

		// dx:ax at this point is HIWORD(w*h) : (LOWORD(w*h)*8 + 16), each
		// half 16 bits wide and wrapping on its own - the three shl's and
		// the `add ax, size SHAPE2D` never carry into dx. Only the
		// `add ax, bx / adc dx, cx` that folds in the running offset does.
		lowterm = (legacy_u16)length * 8 + SHAPE2D_HEADER_SIZE;

		resource_file_set_offset((legacy_u8 far*)mempages,
			(legacy_u16)shapecount, (legacy_u16)i, nextoffset);
		nextoffset += (legacy_u32)lowterm
					+ ((legacy_u32)(legacy_u16)(product >> 16) << 16);

		dstshape = file_get_shape2d_bytes((legacy_u8 far*)mempages, i);
		destination_header = (struct SHAPE2D far*)dstshape;
		// `mov cx, 6 / rep movsw` - the first six words only, up to and
		// including s2d_pos_y. s2d_unk3..s2d_unk6 hold the pattern and flip
		// nibbles and are deliberately left alone in the destination.
		fmemcpy(dstshape, srcshape, 6 * sizeof(legacy_u16));

		destination_header->width = LEGACY_U16_WRAP_MUL(
			destination_header->width, 8U);

		if (length && length <= 8000) {
			mempagesptr = dstshape + SHAPE2D_HEADER_SIZE;

			val = source_header->unknown[1] >> 4;
			val |= val << 8;

			for (j = 0; j < length * 4; ++j) {
				shape2d_put_word(mempagesptr, val);
				mempagesptr += 2U;
			}
			memchunkptr = srcshape + SHAPE2D_HEADER_SIZE;

			for (j = 0; j < 4; ++j) {
				pat = source_header->unknown[j] & 0x0F;

				if (pat) {
					mempagesptr = dstshape + SHAPE2D_HEADER_SIZE;
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
	resource_file_set_size((legacy_u8 far*)mempages,
		(legacy_u32)resource_file_data_start(
			(legacy_u16)shapecount) + nextoffset);
}

legacy_u16 file_get_unflip_size(legacy_s8 far* memchunk) {
	legacy_u16 i, shapecount, size, maxsize;
	legacy_u8 far* memshape;
	struct SHAPE2D far* shape_header;

	shapecount = file_get_res_shape_count(memchunk);
	maxsize = 0;

	for (i = 0; i < shapecount; i++) {
		memshape = file_get_shape2d_bytes((legacy_u8 far*)memchunk, i);
		shape_header = (struct SHAPE2D far*)memshape;
		size = (shape_header->width *
			shape_header->height +
			0x20) >> 4;
		if (size > maxsize)
			maxsize = size;
	}
	return maxsize;
}

legacy_u16 file_load_shape2d_expandedsize(void far* memchunk) {
	legacy_u16 shapecount, i;
	legacy_s32 size;
	legacy_u8 far* memshape;
	struct SHAPE2D far* shape_header;

	shapecount = file_get_res_shape_count(memchunk);

	// The original forms this seed in AX, then uses CWD: both the shift and
	// header addition wrap to 16 bits before the result is sign-extended.
	size = (legacy_s16)(legacy_u16)((shapecount * 8) +
		SHAPE2D_HEADER_SIZE);

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d_bytes((legacy_u8 far*)memchunk, i);
		shape_header = (struct SHAPE2D far*)memshape;
		// `shl ax, 3` then `sub dx, dx / adc`: the per-shape term is a
		// 16-bit value ZERO-extended into the accumulator, and the header
		// size is folded in afterwards with its own carry.
		size += (legacy_u32)(legacy_u16)(shape_header->width * shape_header->height * 8)
			  + SHAPE2D_HEADER_SIZE;
	}

	return (size + SHAPE2D_HEADER_SIZE) >> 4;
}

void file_load_shape2d_palmap_init(legacy_u8 far* pal) {
	legacy_s16 i;

	for (i = 0; i < 0x10; ++i) {
		palmap[i] = pal[i];
	}
}

void file_load_shape2d_palmap_apply(legacy_u8 far* memchunk, legacy_u8 palmap[]) {
	legacy_u16 shapecount, length, i, j;
	legacy_u8 far* memchunkptr;
	legacy_u8 far* memshape;
	struct SHAPE2D far* shape_header;

	shapecount = file_get_res_shape_count(memchunk);

	for (i = 0; i < shapecount; ++i) {
		memshape = file_get_shape2d_bytes(memchunk, i);
		shape_header = (struct SHAPE2D far*)memshape;
		length = shape_header->width *
			shape_header->height;

		memchunkptr = memshape + SHAPE2D_HEADER_SIZE;

		for (j = 0; j < length; ++j) {
			// `mov bl, es:[di] / mov al, [bx+si] / stosb` - the lookup reads
			// the byte di is on, and only stosb advances di afterwards.
			*memchunkptr = palmap[*memchunkptr];
			memchunkptr++;
		}
	}
}

void far* file_load_shape2d_esh(void far* memchunk, const legacy_s8* str) {
	legacy_u16 expandedsize;
	void far* mempages;
	void far* palmapres;

	expandedsize = file_load_shape2d_expandedsize(memchunk);

	palmapres = locate_shape_nofatal(memchunk, "!MGA");

	if (palmapres) {
		file_load_shape2d_palmap_init((legacy_u8 far*)palmapres +
			SHAPE2D_HEADER_SIZE);
	}

	mempages = mmgr_alloc_pages(str, expandedsize);

	resource_file_set_size((legacy_u8 far*)mempages,
		(legacy_u32)expandedsize * 16U);

	file_load_shape2d_expand(memchunk, mempages);
	mmgr_release(memchunk);
	memchunk = mmgr_op_unk(mempages);
	file_load_shape2d_palmap_apply(memchunk, palmap);

	return memchunk;
}

void far* file_load_shape2d(const legacy_s8* shapename, legacy_s16 fatal) {
	legacy_s8 str[100];
	legacy_s8* strptr;
	legacy_s16 counter;
	void far* memchunk;
	void far* mempages;
	legacy_s16 unflipsize;

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
		if (!memchunk) return dos_memory_make_pointer(0, 0);

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
		if (!memchunk) return dos_memory_make_pointer(0, 0);

		mempages = mmgr_alloc_pages("UNFLIP", 1000);
		file_unflip_shape2d_pes(memchunk, mempages);
		mmgr_release(mempages);

		return file_load_shape2d_esh(memchunk, str);
	}
	else if (stricmp(strptr, ".ESH") == 0) {
		memchunk = file_load_binary(str, fatal);
		if (!memchunk) return dos_memory_make_pointer(0, 0);

		return file_load_shape2d_esh(memchunk, str);
	}
	else { // .VSH or an explicit unknown extension
		return file_load_binary(str, fatal);
	}
}

void far* file_load_shape2d_fatal(const legacy_s8* shapename) {
	return file_load_shape2d(shapename, 1);
}

void far* file_load_shape2d_nofatal(const legacy_s8* shapename) {
	return file_load_shape2d(shapename, 0);
}

void far* file_load_shape2d_nofatal2(const legacy_s8* shapename) {
	return file_load_shape2d_nofatal(shapename);
}

void far* file_load_shape2d_res(const legacy_s8* resname, legacy_s16 fatal) {
	legacy_s16 chunksize;
	const legacy_s8* shapename = mmgr_path_to_name(resname);
	void far* mempages;
	void far* memchunk = mmgr_get_chunk_by_name(shapename);
	legacy_u16 freeparas, margin, rawseg;

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
	if (freeparas < (legacy_u16)chunksize + 2 &&
		!(highpool_route(resname, (legacy_u16)chunksize) &&
		  highpool_can_fit((legacy_u16)chunksize))) {
		margin = ((legacy_u16)chunksize >> 1) + ((legacy_u16)chunksize >> 2);
		if (margin > freeparas - (freeparas >> 3))
			margin = freeparas - (freeparas >> 3);

		if (margin >= ((legacy_u16)chunksize >> 1)) {
			rawseg = dos_memory_pointer_segment(memchunk);
			mmgr_resize_memory(0, rawseg, chunksize + margin);
			copy_paras_reverse(rawseg, rawseg + margin, chunksize);
			parse_shape2d(dos_memory_make_pointer(rawseg + margin, 0), dos_memory_make_pointer(rawseg, 0));
			mmgr_resize_memory(0, rawseg, chunksize);
			mmgr_rename_chunk(dos_memory_make_pointer(rawseg, 0), resname);
			return dos_memory_make_pointer(rawseg, 0);
		}
	}

	mempages = mmgr_alloc_pages(resname, chunksize);

	parse_shape2d(memchunk, mempages);

	mmgr_release(memchunk);
	return mmgr_op_unk(mempages);
}

void far* file_load_shape2d_res_fatal(const legacy_s8* resname) {
	return file_load_shape2d_res(resname, 1);
}

void far* file_load_shape2d_res_nofatal(const legacy_s8* resname) {
	return file_load_shape2d_res(resname, 0);
}
