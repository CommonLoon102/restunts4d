#include <assert.h>
#include <stddef.h>

#include "../c/legacy.h"

static legacy_u16 reference_sar16(legacy_u16 bits, legacy_u16 count)
{
	count &= 0x1FU;
	while (count-- != 0U)
		bits = (legacy_u16)((bits >> 1) | (bits & 0x8000U));
	return bits;
}

static legacy_u32 reference_sar32(legacy_u32 bits, legacy_u16 count)
{
	count &= 0x1FU;
	while (count-- != 0U)
		bits = (bits >> 1) | (bits & (legacy_u32)0x80000000UL);
	return bits;
}

static legacy_u16 reference_shl16(legacy_u16 bits, legacy_u16 count)
{
	count &= 0x1FU;
	while (count-- != 0U)
		bits = (legacy_u16)(bits << 1);
	return bits;
}

static legacy_s16 reference_smul_high(legacy_s16 left, legacy_s16 right)
{
	legacy_s32 product;

	product = (legacy_s32)left * (legacy_s32)right;
	return LEGACY_S16_FROM_BITS((legacy_u16)reference_sar32(
		(legacy_u32)product, 16U));
}

static void test_word_shifts_and_rotates(void)
{
	legacy_u32 value;
	legacy_u16 count;
	legacy_u16 bits;

	for (value = 0UL; value <= 0xFFFFUL; value++) {
		bits = (legacy_u16)value;
		for (count = 0U; count < 40U; count++) {
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
		0UL, 1UL, 0x7FFFFFFFUL, 0x80000000UL, 0x89ABCDEFUL,
		0xFFFFFFFFUL
	};
	size_t index;
	legacy_u16 count;

	for (index = 0U; index < sizeof(values) / sizeof(values[0]); index++) {
		for (count = 0U; count < 40U; count++) {
			assert(LEGACY_U32_SAR(values[index], count) ==
				reference_sar32(values[index], count));
			assert(LEGACY_U32_SHL(values[index], count) ==
				(values[index] << (count & 0x1FU)));
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

	assert(LEGACY_U16_LOW_BYTE(0xABCDU) == 0xCDU);
	assert(LEGACY_U16_REPLACE_LOW_BYTE(0xABCDU, 0x12U) == 0xAB12U);
	assert(LEGACY_U16_REPLACE_LOW_BYTE(0xABCDU, 0x1234U) == 0xAB34U);
	assert(LEGACY_S8_WRAP_ADD(127, 1) == -128);
	assert(LEGACY_S8_WRAP_SUB(-128, 1) == 127);
	assert(LEGACY_S8_WRAP_NEGATE(-128) == -128);
	assert(LEGACY_S8_WRAP_MUL(64, 4) == 0);
	assert(LEGACY_U8_WRAP_ADD(0xFFU, 1U) == 0U);
	assert(LEGACY_U8_WRAP_SUB(0U, 1U) == 0xFFU);
	assert(LEGACY_U8_WRAP_MUL(0xFFU, 2U) == 0xFEU);
	assert(LEGACY_S16_WRAP_ADD(32767, 1) == -32768);
	assert(LEGACY_S16_WRAP_SUB(-32768, 1) == 32767);
	assert(LEGACY_S32_WRAP_ADD(
		LEGACY_S32_FROM_BITS(0x7FFFFFFFUL), 1L) ==
		LEGACY_S32_FROM_BITS(0x80000000UL));
	assert(LEGACY_S32_WRAP_ADD_S16(
		LEGACY_S32_FROM_BITS(0x7FFFFFFFUL), 1) ==
		LEGACY_S32_FROM_BITS(0x80000000UL));
	assert(LEGACY_S32_WRAP_SUB_S16(
		LEGACY_S32_FROM_BITS(0x80000000UL), 1) ==
		LEGACY_S32_FROM_BITS(0x7FFFFFFFUL));
	assert(LEGACY_S32_WRAP_SUB_S16(0L, -32768) == 32768L);
	assert(LEGACY_S32_WRAP_MUL(
		LEGACY_S32_FROM_BITS(0x40000000UL), 4L) == 0L);
	assert(LEGACY_U16_MUL_HIGH(0xFFFFU, 0xFFFFU) == 0xFFFEU);
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
	assert(LEGACY_U16_DIV_OR_ZERO(0xFFFFU, 2U) == 0x7FFFU);
	assert(LEGACY_U16_DIV_OR_ZERO(0xFFFFU, 0U) == 0U);
	assert(LEGACY_S16_DIV_OR_ZERO(7, 3) == 2);
	assert(LEGACY_S16_DIV_OR_ZERO(-7, 3) == -2);
	assert(LEGACY_S16_DIV_OR_ZERO(7, 0) == 0);
	assert(LEGACY_S16_DIV_OR_ZERO(-32768, -1) == 0);
	assert(LEGACY_U32_DIV_OR_ZERO(0xFFFFFFFFUL, 2UL) == 0x7FFFFFFFUL);
	assert(LEGACY_U32_DIV_OR_ZERO(0xFFFFFFFFUL, 0UL) == 0UL);
	assert(LEGACY_S32_DIV_OR_ZERO(7L, -3L) == -2L);
	assert(LEGACY_S32_DIV_OR_ZERO(7L, 0L) == 0L);
	assert(LEGACY_S32_DIV_OR_ZERO(
		LEGACY_S32_FROM_BITS(0x80000000UL), -1L) == 0L);
}

static void test_little_endian_access(void)
{
	legacy_u8 storage[6];
	legacy_u8* bytes;

	bytes = storage + 1;

	LEGACY_WRITE_U16_LE(bytes, 0xFEDCU);
	assert(bytes[0] == 0xDCU && bytes[1] == 0xFEU);
	assert(LEGACY_READ_U16_LE(bytes) == 0xFEDCU);
	assert(LEGACY_READ_S16_LE(bytes) == -292);

	LEGACY_WRITE_U32_LE(bytes, 0x89ABCDEFUL);
	assert(bytes[0] == 0xEFU && bytes[1] == 0xCDU);
	assert(bytes[2] == 0xABU && bytes[3] == 0x89U);
	assert(LEGACY_READ_U32_LE(bytes) == 0x89ABCDEFUL);
	assert((legacy_u32)LEGACY_READ_S32_LE(bytes) == 0x89ABCDEFUL);
	assert(LEGACY_U32_FROM_WORDS(0xCDEFU, 0x89ABU) ==
		0x89ABCDEFUL);
	assert(LEGACY_U32_FROM_WORDS(-1, LEGACY_S16_FROM_BITS(0x8000U)) ==
		0x8000FFFFUL);
}

int main(void)
{
	test_word_shifts_and_rotates();
	test_dword_shifts_and_rotates();
	test_multiply_and_divide();
	test_little_endian_access();
	return 0;
}
