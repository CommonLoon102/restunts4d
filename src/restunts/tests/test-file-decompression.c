#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../c/fileio.h"
#include "../c/memmgr.h"
#include "../c/platform.h"
#include "../c/fatal.h"

legacy_u32 file_decomp_vle(legacy_u8 huge *src, legacy_u8 huge *dst, legacy_u16 paragraphs);
legacy_u32 file_decomp_rle(legacy_u8 huge *src, legacy_u8 huge *dst, legacy_u16 paragraphs);
legacy_u32 file_decomp_rle_seq(legacy_u8 huge *src, legacy_u8 huge *dst, legacy_u32 length,
							   legacy_u8 escape);
legacy_u32 file_decomp_rle_single(legacy_u8 huge *src, legacy_u8 huge *dst, legacy_u32 length,
								  legacy_u8 *escapes);

static legacy_u8 memory[0x100000];
static legacy_u8 packed[200000];
static legacy_u8 expected[70000];
static legacy_u8 stage[200000];
static legacy_u8 file_bytes[200000];
static unsigned int file_length, file_position;
static unsigned int open_calls, close_calls, read_calls, resize_calls, copy_calls, fatal_calls;
static unsigned int fail_open, fail_read, cached;
static unsigned int allocated_paragraphs, resized_paragraphs;
static uint64_t trace_hash = UINT64_C(1469598103934665603);

static void trace_word(unsigned int value)
{
	trace_hash = (trace_hash ^ (value & 255U)) * UINT64_C(1099511628211);
	trace_hash = (trace_hash ^ ((value >> 8) & 255U)) * UINT64_C(1099511628211);
}
static void trace_bytes(const legacy_u8 *bytes, unsigned int count)
{
	unsigned int i;
	for (i = 0; i < count; i++) {
		trace_hash = (trace_hash ^ bytes[i]) * UINT64_C(1099511628211);
	}
}
static void check_hash(const char *name, uint64_t expected_hash)
{
#ifdef FILE_RECORD_BASELINE
	printf("%s %016llx\n", name, (unsigned long long)trace_hash);
	(void)expected_hash;
#else
	if (trace_hash != expected_hash) {
		fprintf(stderr, "%s: got %016llx expected %016llx\n", name, (unsigned long long)trace_hash,
				(unsigned long long)expected_hash);
		assert(trace_hash == expected_hash);
	}
#endif
	trace_hash = UINT64_C(1469598103934665603);
}

void far *dos_memory_make_pointer(legacy_u16 segment, legacy_u16 offset)
{
	unsigned int address = (unsigned int)segment * 16U + offset;
	assert(address < sizeof(memory));
	return memory + address;
}
legacy_u16 dos_memory_pointer_segment(const void far *pointer)
{
	assert((const legacy_u8 *)pointer >= memory &&
		   (const legacy_u8 *)pointer < memory + sizeof(memory));
	return ((const legacy_u8 *)pointer - memory) >> 4;
}
legacy_u16 dos_memory_pointer_offset(const void far *pointer)
{
	assert((const legacy_u8 *)pointer >= memory &&
		   (const legacy_u8 *)pointer < memory + sizeof(memory));
	return ((const legacy_u8 *)pointer - memory) & 15U;
}
void copy_paras_reverse(legacy_u16 source, legacy_u16 destination, legacy_s16 paragraphs)
{
	trace_word(1);
	trace_word(source);
	trace_word(destination);
	trace_word(paragraphs);
	copy_calls++;
	assert(paragraphs >= 0);
	memmove(memory + (unsigned int)destination * 16U, memory + (unsigned int)source * 16U,
			(unsigned int)paragraphs * 16U);
}
void far *mmgr_get_chunk_by_name(const legacy_s8 *name)
{
	(void)name;
	trace_word(2);
	return cached ? memory + 0x20000 : 0;
}
void far *mmgr_alloc_pages(const legacy_s8 *name, legacy_u16 paragraphs)
{
	(void)name;
	trace_word(3);
	trace_word(paragraphs);
	allocated_paragraphs = paragraphs;
	return memory + 0x20000;
}
legacy_u16 mmgr_resize_memory(legacy_u16 offset, legacy_u16 segment, legacy_u16 paragraphs)
{
	trace_word(4);
	trace_word(offset);
	trace_word(segment);
	trace_word(paragraphs);
	resize_calls++;
	resized_paragraphs = paragraphs;
	return paragraphs;
}
legacy_u16 dos_file_open(const legacy_s8 *name, legacy_s16 create)
{
	(void)name;
	trace_word(5);
	trace_word(create);
	open_calls++;
	file_position = 0;
	return fail_open ? 0 : 1;
}
legacy_s16 dos_file_close(legacy_u16 handle)
{
	trace_word(6);
	trace_word(handle);
	close_calls++;
	return 0;
}
legacy_u16 dos_file_read(legacy_u16 handle, void far *destination, legacy_u16 length)
{
	unsigned int count = length;
	trace_word(7);
	trace_word(handle);
	trace_word(length);
	read_calls++;
	if (count > file_length - file_position) {
		count = file_length - file_position;
	}
	memcpy(destination, file_bytes + file_position, count);
	file_position += count;
	return count;
}
legacy_s16 dos_file_seek(legacy_u16 handle, legacy_s32 offset, legacy_s16 origin)
{
	trace_word(8);
	trace_word(handle);
	trace_word(offset);
	trace_word(origin);
	assert(origin == DOS_FILE_SEEK_END && offset == 0);
	file_position = file_length;
	return 0;
}
legacy_s32 dos_file_tell(legacy_u16 handle)
{
	trace_word(9);
	trace_word(handle);
	return file_position;
}
legacy_s16 dos_file_error(void)
{
	trace_word(10);
	return fail_read;
}
void fatal_error(const legacy_s8 *format, ...)
{
	(void)format;
	trace_word(11);
	fatal_calls++;
}

static void write_size(legacy_u8 *bytes, unsigned int length)
{
	bytes[0] = length;
	bytes[1] = length >> 8;
	bytes[2] = length >> 16;
}

/* Encode canonical prefix codes independently of the production lookup tables. */
static unsigned int make_vle(legacy_u8 *destination, const legacy_u8 *counts, unsigned int depth,
							 unsigned int length, unsigned int additive, unsigned int scenario)
{
	unsigned int codes[256], widths[256], alphabet_length = 0, code = 0;
	unsigned int width, i, j, symbol, bit, bit_count = 0, data_offset;
	legacy_u8 alphabet[256], value = 0;
	destination[0] = 2;
	write_size(destination + 1, length);
	destination[4] = depth | (additive ? 128 : 0);
	for (width = 1; width <= depth; width++) {
		destination[4 + width] = counts[width - 1];
		for (i = 0; i < counts[width - 1]; i++) {
			codes[alphabet_length] = code++;
			widths[alphabet_length] = width;
			alphabet[alphabet_length] = (legacy_u8)(alphabet_length * 19U + scenario * 17U + 123U);
			alphabet_length++;
		}
		code *= 2;
	}
	memcpy(destination + 5 + depth, alphabet, alphabet_length);
	data_offset = 5 + depth + alphabet_length;
	memset(destination + data_offset, 0, (length + 1) * 2 + 4);
	for (i = 0; i <= length; i++) {
		symbol = (i * 47U + scenario) % alphabet_length;
		if (additive) {
			value = (legacy_u8)(value + alphabet[symbol]);
		} else {
			value = alphabet[symbol];
		}
		expected[i] = value;
		for (j = widths[symbol]; j > 0; j--) {
			bit = (codes[symbol] >> (j - 1)) & 1U;
			destination[data_offset + bit_count / 8] |= bit << (7 - bit_count % 8);
			bit_count++;
		}
	}
	return data_offset + (bit_count + 7) / 8 + 4;
}

static void test_vle(void)
{
	static const unsigned int lengths[] = {0, 1, 7, 15, 31, 257, 65537};
	legacy_u8 counts[16];
	unsigned int layout, l, additive, scenario, depth, size, i;
	legacy_u32 result;
	for (layout = 0; layout < 5; layout++) {
		memset(counts, 0, sizeof(counts));
		depth = 1;
		if (layout == 0) {
			counts[0] = 2;
		}
		if (layout == 1) {
			depth = 4;
			counts[0] = 1;
			counts[1] = 1;
			counts[2] = 1;
			counts[3] = 2;
		}
		if (layout == 2) {
			depth = 9;
			for (i = 0; i < 8; i++) {
				counts[i] = 1;
			}
			counts[8] = 2;
		}
		if (layout == 3) {
			depth = 16;
			for (i = 0; i < 16; i++) {
				counts[i] = 1;
			}
		}
		if (layout == 4) {
			depth = 9;
			counts[7] = 254;
			counts[8] = 2;
		}
		for (l = 0; l < sizeof(lengths) / sizeof(lengths[0]); l++) {
			for (additive = 0; additive < 2; additive++) {
				for (scenario = 0; scenario < 3; scenario++) {
					trace_word(layout);
					trace_word(lengths[l]);
					trace_word(lengths[l] >> 16);
					trace_word(additive);
					trace_word(scenario);
					size = make_vle(packed, counts, depth, lengths[l], additive, scenario);
					memset(memory, 0xa5, sizeof(memory));
					memcpy(memory + 0x5fffb, packed, size);
					result = file_decomp_vle(memory + 0x5fffb, memory + 0x1fffd, 0xffff);
					assert(result == lengths[l]);
					assert(memcmp(memory + 0x1fffd, expected, lengths[l] + 1) == 0);
					assert(memory[0x1fffc] == 0xa5 && memory[0x1fffd + lengths[l] + 1] == 0xa5);
					trace_bytes(memory + 0x1fffc, lengths[l] + 3);
				}
			}
		}
	}
	check_hash("VLE", UINT64_C(0xf04d3106ec426067));
}

static unsigned int make_rle_literals(legacy_u8 *destination, const legacy_u8 *source,
									  unsigned int length)
{
	unsigned int i, cursor = 10;
	destination[0] = 1;
	write_size(destination + 1, length);
	destination[7] = 0;
	destination[8] = 129;
	destination[9] = 0xe0;
	for (i = 0; i < length; i++) {
		if (source[i] == 0xe0) {
			destination[cursor++] = 0xe0;
			destination[cursor++] = 1;
		}
		destination[cursor++] = source[i];
	}
	write_size(destination + 4, cursor - 10);
	return cursor;
}

static void reset_file(void)
{
	memset(memory, 0xa5, sizeof(memory));
	open_calls = 0;
	close_calls = 0;
	read_calls = 0;
	resize_calls = 0;
	copy_calls = 0;
	fatal_calls = 0;
	fail_open = 0;
	fail_read = 0;
	cached = 0;
	allocated_paragraphs = 0;
	resized_paragraphs = 0;
}

static void test_file_passes(void)
{
	unsigned int passes, scenario, i, size, result_size;
	void *result;
	for (passes = 1; passes <= 3; passes++) {
		for (scenario = 0; scenario < 8; scenario++) {
			reset_file();
			result_size = scenario % 2 ? 33 : 16;
			for (i = 0; i < result_size; i++) {
				expected[i] = (legacy_u8)(scenario + i * 7U);
			}
			size = make_rle_literals(packed, expected, result_size);
			for (i = 1; i < passes; i++) {
				memcpy(stage, packed, size);
				size = make_rle_literals(packed, stage, size);
			}
			if (passes > 1) {
				file_bytes[0] = 128 | passes;
				write_size(file_bytes + 1, result_size);
				memcpy(file_bytes + 4, packed, size);
				file_length = size + 4;
			} else {
				memcpy(file_bytes, packed, size);
				file_length = size;
			}
			if (scenario == 2) {
				cached = 1;
			}
			if (scenario == 3) {
				fail_open = 1;
			}
			if (scenario == 4) {
				fail_read = 1;
			}
			if (scenario == 5) {
				file_bytes[passes > 1 ? 4 : 0] = 3;
			}
			if (scenario == 6) {
				file_bytes[0] = 128;
			}
			if (scenario == 7) {
				write_size(file_bytes + 1, 0);
			}
			trace_word(passes);
			trace_word(scenario);
			result = file_decomp("TEST.PVS", scenario % 2);
			if (scenario < 2) {
				assert(result == memory + 0x20000);
				assert(memcmp(result, expected, result_size) == 0);
				assert(resize_calls == 1 && resized_paragraphs == (result_size + 15) / 16);
				assert(copy_calls == passes - 1);
			}
			if (cached) {
				assert(open_calls == 0 && result == memory + 0x20000);
			}
			if (scenario == 3 || scenario == 4 || scenario == 5 || scenario == 7) {
				assert(result == 0);
			}
			trace_word(result != 0);
			trace_word(open_calls);
			trace_word(close_calls);
			trace_word(read_calls);
			trace_word(resize_calls);
			trace_word(copy_calls);
			trace_word(fatal_calls);
			trace_word(allocated_paragraphs);
			trace_word(resized_paragraphs);
			trace_bytes(memory + 0x20000, 128);
		}
	}
	check_hash("file passes", UINT64_C(0x30b187c9d0cfa23c));
}

static void test_file_vle(void)
{
	static const unsigned int lengths[] = {1, 15, 257, 65537};
	legacy_u8 counts[1] = {2};
	unsigned int i, additive;
	void *result;
	for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
		for (additive = 0; additive < 2; additive++) {
			reset_file();
			file_length = make_vle(file_bytes, counts, 1, lengths[i], additive, i);
			trace_word(lengths[i]);
			trace_word(lengths[i] >> 16);
			trace_word(additive);
			result = file_decomp("VLE.PVS", 0);
			assert(result == memory + 0x20000);
			assert(memcmp(result, expected, lengths[i] + 1) == 0);
			assert(resize_calls == 1 && resized_paragraphs == (lengths[i] + 15) / 16);
			trace_bytes(result, lengths[i] + 2);
		}
	}
	check_hash("VLE files", UINT64_C(0x91345b9f2b17be5c));
}

static unsigned int make_vle_bytes(legacy_u8 *destination, const legacy_u8 *source,
								   unsigned int length, unsigned int additive)
{
	legacy_u8 alphabet[256], deltas[256], previous = 0, value;
	unsigned int alphabet_length = 0, i, j;
	assert(length <= sizeof(deltas));
	for (i = 0; i < length; i++) {
		value = additive ? (legacy_u8)(source[i] - previous) : source[i];
		previous = source[i];
		for (j = 0; j < alphabet_length && alphabet[j] != value; j++) {
		}
		if (j == alphabet_length) {
			alphabet[alphabet_length++] = value;
		}
		deltas[i] = j;
	}
	assert(alphabet_length < 256);
	destination[0] = 2;
	write_size(destination + 1, length - 1);
	destination[4] = 8 | (additive ? 128 : 0);
	memset(destination + 5, 0, 8);
	destination[12] = alphabet_length;
	memcpy(destination + 13, alphabet, alphabet_length);
	memcpy(destination + 13 + alphabet_length, deltas, length);
	memset(destination + 13 + alphabet_length + length, 0, 4);
	return 17 + alphabet_length + length;
}

static void test_mixed_passes(void)
{
	unsigned int order, additive, i, size;
	void *result;
	for (order = 0; order < 2; order++) {
		for (additive = 0; additive < 2; additive++) {
			reset_file();
			for (i = 0; i < 16; i++) {
				expected[i] = (legacy_u8)(i * 17 + additive * 13);
			}
			if (order == 0) {
				size = make_rle_literals(stage, expected, 16);
				size = make_vle_bytes(packed, stage, size, additive);
			} else {
				size = make_vle_bytes(stage, expected, 16, additive);
				size = make_rle_literals(packed, stage, size);
			}
			file_bytes[0] = 130;
			write_size(file_bytes + 1, 16 - order);
			memcpy(file_bytes + 4, packed, size);
			file_length = size + 4;
			trace_word(order);
			trace_word(additive);
			result = file_decomp("MIXED.PVS", 0);
			assert(result == memory + 0x20000);
			assert(memcmp(result, expected, 16) == 0);
			assert(copy_calls == 1 && resize_calls == 1 && resized_paragraphs == 1);
			trace_bytes(memory + 0x20000, 80);
		}
	}
	check_hash("mixed passes", UINT64_C(0x00951c7e63501a81));
}

static void test_rle_passes(void)
{
	static const legacy_u8 escape_flags[] = {2, 3, 128, 129, 131};
	unsigned int i, scenario, length;
	legacy_u32 result;
	for (i = 0; i < sizeof(escape_flags); i++) {
		for (scenario = 0; scenario < 3; scenario++) {
			reset_file();
			memset(packed, 0, sizeof(packed));
			packed[0] = 1;
			length = 5;
			write_size(packed + 1, length);
			packed[8] = escape_flags[i];
			packed[9] = 0xe0;
			packed[10] = 0xe1;
			packed[11] = 0xe2;
			if (escape_flags[i] == 128) {
				/* Zero declared escapes still enables the sequence pass. */
				packed[9] = 'A';
				packed[10] = 0xe1;
				packed[11] = 'B';
				packed[12] = 0xe1;
				packed[13] = 4;
				write_size(packed + 4, 5);
			} else if (escape_flags[i] <= 3) {
				unsigned int start = 9 + escape_flags[i];
				packed[start] = 0xe1;
				packed[start + 1] = 'A';
				packed[start + 2] = 0xe1;
				packed[start + 3] = 5;
				write_size(packed + 4, 4);
			} else {
				unsigned int start = 9 + (escape_flags[i] & 127);
				packed[start] = 0xe0;
				packed[start + 1] = 5 + scenario;
				packed[start + 2] = 'Z';
				write_size(packed + 4, 3);
			}
			memcpy(memory + 0x5fff9, packed, 32);
			result = file_decomp_rle(memory + 0x5fff9, memory + 0x20000, 16);
			assert(result == length);
			trace_word(escape_flags[i]);
			trace_word(scenario);
			trace_word(result);
			trace_bytes(memory + 0x20000, 32);
		}
	}
	check_hash("RLE passes", UINT64_C(0x861032445c2d38b5));
}

int main(void)
{
	test_vle();
	test_file_passes();
	test_file_vle();
	test_mixed_passes();
	test_rle_passes();
	puts("File decompression regression checks passed.");
	return 0;
}
