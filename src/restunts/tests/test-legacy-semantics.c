#include <assert.h>
#include <stddef.h>

#include "../c/legacy.h"

#define TEST_SHIFT_COUNT_LIMIT 40U
#define TEST_DWORD_PATTERN 2309737967UL
#define TEST_WORD_PATTERN 43981U
#define TEST_WORD_PATTERN_LOW_BYTE 205U
#define TEST_REPLACEMENT_BYTE 18U
#define TEST_REPLACEMENT_WORD 4660U
#define TEST_WORD_WITH_REPLACEMENT_BYTE 43794U
#define TEST_WORD_WITH_REPLACEMENT_WORD_LOW_BYTE 43828U
#define TEST_U8_DOUBLE_MAX_RESULT 254U
#define TEST_QUARTER_DWORD_RANGE 1073741824UL
#define TEST_LITTLE_ENDIAN_WORD 65244U
#define TEST_LITTLE_ENDIAN_WORD_LOW_BYTE 220U
#define TEST_LITTLE_ENDIAN_WORD_HIGH_BYTE 254U
#define TEST_DWORD_BYTE_ZERO 239U
#define TEST_DWORD_BYTE_ONE 205U
#define TEST_DWORD_BYTE_TWO 171U
#define TEST_DWORD_BYTE_THREE 137U
#define TEST_PATTERN_LOW_WORD 52719U
#define TEST_PATTERN_HIGH_WORD 35243U
#define TEST_SIGN_EXTENDED_LOW_WORD 2147549183UL

static legacy_u16 reference_sar16(legacy_u16 bits, legacy_u16 count)
{
	count &= LEGACY_X86_SHIFT_COUNT_MASK;
	while (count-- != 0U)
		bits = (legacy_u16)((bits >> 1) |
			(bits & LEGACY_U16_SIGN_BIT));
	return bits;
}

static legacy_u32 reference_sar32(legacy_u32 bits, legacy_u16 count)
{
	count &= LEGACY_X86_SHIFT_COUNT_MASK;
	while (count-- != 0U)
		bits = (bits >> 1) | (bits & (legacy_u32)LEGACY_U32_SIGN_BIT);
	return bits;
}

static legacy_u16 reference_shl16(legacy_u16 bits, legacy_u16 count)
{
	count &= LEGACY_X86_SHIFT_COUNT_MASK;
	while (count-- != 0U)
		bits = (legacy_u16)(bits << 1);
	return bits;
}

static legacy_s16 reference_smul_high(legacy_s16 left, legacy_s16 right)
{
	legacy_s32 product;

	product = (legacy_s32)left * (legacy_s32)right;
	return LEGACY_S16_FROM_BITS((legacy_u16)reference_sar32(
		(legacy_u32)product, LEGACY_WORD_BITS));
}

static void test_word_shifts_and_rotates(void)
{
	legacy_u32 value;
	legacy_u16 count;
	legacy_u16 bits;

	for (value = 0UL; value <= LEGACY_U16_MAX; value++) {
		bits = (legacy_u16)value;
		for (count = 0U; count < TEST_SHIFT_COUNT_LIMIT; count++) {
			assert(LEGACY_U16_SAR(bits, count) ==
				reference_sar16(bits, count));
			assert(LEGACY_U16_SHL(bits, count) ==
				reference_shl16(bits, count));
			assert(LEGACY_U16_ROL(
				LEGACY_U16_ROR(bits, count), count) == bits);
		}
	}
}

static void test_dword_shifts_and_rotates(void)
{
	static const legacy_u32 values[] = {
		0UL, 1UL, LEGACY_S32_MAX, LEGACY_U32_SIGN_BIT,
		TEST_DWORD_PATTERN, LEGACY_U32_MAX
	};
	size_t index;
	legacy_u16 count;

	for (index = 0U; index < sizeof(values) / sizeof(values[0]); index++) {
		for (count = 0U; count < TEST_SHIFT_COUNT_LIMIT; count++) {
			assert(LEGACY_U32_SAR(values[index], count) ==
				reference_sar32(values[index], count));
			assert(LEGACY_U32_SHL(values[index], count) ==
				(values[index] <<
					(count & LEGACY_X86_SHIFT_COUNT_MASK)));
			assert(LEGACY_U32_ROL(
				LEGACY_U32_ROR(values[index], count), count) ==
				values[index]);
		}
	}
}

static void test_multiply_and_divide(void)
{
	static const legacy_s16 multiply_values[] = {
		-32768, -32767, -1, 0, 1, 32766, 32767
	};
	size_t left_index;
	size_t right_index;

	assert(LEGACY_U16_LOW_BYTE(TEST_WORD_PATTERN) ==
		TEST_WORD_PATTERN_LOW_BYTE);
	assert(LEGACY_U16_REPLACE_LOW_BYTE(TEST_WORD_PATTERN,
		TEST_REPLACEMENT_BYTE) == TEST_WORD_WITH_REPLACEMENT_BYTE);
	assert(LEGACY_U16_REPLACE_LOW_BYTE(TEST_WORD_PATTERN,
		TEST_REPLACEMENT_WORD) == TEST_WORD_WITH_REPLACEMENT_WORD_LOW_BYTE);
	assert(LEGACY_S8_WRAP_ADD(127, 1) == -128);
	assert(LEGACY_S8_WRAP_SUB(-128, 1) == 127);
	assert(LEGACY_S8_WRAP_NEGATE(-128) == -128);
	assert(LEGACY_S8_WRAP_MUL(64, 4) == 0);
	assert(LEGACY_U8_WRAP_ADD(LEGACY_U8_MAX, 1U) == 0U);
	assert(LEGACY_U8_WRAP_SUB(0U, 1U) == LEGACY_U8_MAX);
	assert(LEGACY_U8_WRAP_MUL(LEGACY_U8_MAX, 2U) ==
		TEST_U8_DOUBLE_MAX_RESULT);
	assert(LEGACY_S16_WRAP_ADD(32767, 1) == -32768);
	assert(LEGACY_S16_WRAP_SUB(-32768, 1) == 32767);
	assert(LEGACY_S32_WRAP_ADD(
		LEGACY_S32_FROM_BITS(LEGACY_S32_MAX), 1L) ==
		LEGACY_S32_FROM_BITS(LEGACY_U32_SIGN_BIT));
	assert(LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_FROM_BITS(LEGACY_S32_MAX), 1) ==
		LEGACY_S32_FROM_BITS(LEGACY_U32_SIGN_BIT));
	assert(LEGACY_S32_WRAP_SUB_S16(
		LEGACY_S32_FROM_BITS(LEGACY_U32_SIGN_BIT), 1) ==
		LEGACY_S32_FROM_BITS(LEGACY_S32_MAX));
	assert(LEGACY_S32_WRAP_SUB_S16(0L, -32768) == 32768L);
	assert(LEGACY_S32_WRAP_MUL(
		LEGACY_S32_FROM_BITS(TEST_QUARTER_DWORD_RANGE), 4L) == 0L);
	assert(LEGACY_U16_MUL_HIGH(LEGACY_U16_MAX, LEGACY_U16_MAX) ==
		LEGACY_U16_MAX - 1U);
	for (left_index = 0U; left_index < sizeof(multiply_values) /
		sizeof(multiply_values[0]); left_index++) {
		for (right_index = 0U; right_index < sizeof(multiply_values) /
			sizeof(multiply_values[0]); right_index++) {
			assert(LEGACY_S16_MUL_HIGH(
				multiply_values[left_index],
				multiply_values[right_index]) == reference_smul_high(
				multiply_values[left_index],
				multiply_values[right_index]));
		}
	}
	assert(LEGACY_U16_DIV_OR_ZERO(LEGACY_U16_MAX, 2U) ==
		LEGACY_S16_MAX);
	assert(LEGACY_U16_DIV_OR_ZERO(LEGACY_U16_MAX, 0U) == 0U);
	assert(LEGACY_S16_DIV_OR_ZERO(7, 3) == 2);
	assert(LEGACY_S16_DIV_OR_ZERO(-7, 3) == -2);
	assert(LEGACY_S16_DIV_OR_ZERO(7, 0) == 0);
	assert(LEGACY_S16_DIV_OR_ZERO(-32768, -1) == 0);
	assert(LEGACY_U32_DIV_OR_ZERO(LEGACY_U32_MAX, 2UL) ==
		LEGACY_S32_MAX);
	assert(LEGACY_U32_DIV_OR_ZERO(LEGACY_U32_MAX, 0UL) == 0UL);
	assert(LEGACY_S32_DIV_OR_ZERO(7L, -3L) == -2L);
	assert(LEGACY_S32_DIV_OR_ZERO(7L, 0L) == 0L);
	assert(LEGACY_S32_DIV_OR_ZERO(
		LEGACY_S32_FROM_BITS(LEGACY_U32_SIGN_BIT), -1L) == 0L);
}

static void test_little_endian_access(void)
{
	legacy_u8 storage[6];
	legacy_u8* bytes;

	bytes = storage + 1;

	LEGACY_WRITE_U16_LE(bytes, TEST_LITTLE_ENDIAN_WORD);
	assert(bytes[0] == TEST_LITTLE_ENDIAN_WORD_LOW_BYTE &&
		bytes[1] == TEST_LITTLE_ENDIAN_WORD_HIGH_BYTE);
	assert(LEGACY_READ_U16_LE(bytes) == TEST_LITTLE_ENDIAN_WORD);
	assert(LEGACY_READ_S16_LE(bytes) == -292);

	LEGACY_WRITE_U32_LE(bytes, TEST_DWORD_PATTERN);
	assert(bytes[0] == TEST_DWORD_BYTE_ZERO &&
		bytes[1] == TEST_DWORD_BYTE_ONE);
	assert(bytes[2] == TEST_DWORD_BYTE_TWO &&
		bytes[3] == TEST_DWORD_BYTE_THREE);
	assert(LEGACY_READ_U32_LE(bytes) == TEST_DWORD_PATTERN);
	assert((legacy_u32)LEGACY_READ_S32_LE(bytes) == TEST_DWORD_PATTERN);
	assert(LEGACY_U32_FROM_WORDS(TEST_PATTERN_LOW_WORD,
		TEST_PATTERN_HIGH_WORD) == TEST_DWORD_PATTERN);
	assert(LEGACY_U32_FROM_WORDS(-1,
		LEGACY_S16_FROM_BITS(LEGACY_U16_SIGN_BIT)) ==
		TEST_SIGN_EXTENDED_LOW_WORD);
}

int main(void)
{
	test_word_shifts_and_rotates();
	test_dword_shifts_and_rotates();
	test_multiply_and_divide();
	test_little_endian_access();
	return 0;
}
