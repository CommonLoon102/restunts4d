#ifndef RESTUNTS_LEGACY_H
#define RESTUNTS_LEGACY_H

#include <limits.h>

/*
 * Integer types used by code whose behavior is defined by the original
 * 16-bit DOS executable.  Do not use plain int or long for serialized data,
 * word wrapping, or double-word intermediates: their widths change between
 * the Borland DOS and modern hosted data models.
 */
typedef signed char legacy_s8;
typedef unsigned char legacy_u8;
typedef signed short legacy_s16;
typedef unsigned short legacy_u16;

#if UINT_MAX == 0xFFFFFFFFUL
typedef signed int legacy_s32;
typedef unsigned int legacy_u32;
#elif ULONG_MAX == 0xFFFFFFFFUL
typedef signed long legacy_s32;
typedef unsigned long legacy_u32;
#else
#error Restunts requires an exact 32-bit integer type
#endif

/* C89-compatible compile-time assertions, including for Borland C++. */
typedef char legacy_byte_must_be_8_bits[(CHAR_BIT == 8) ? 1 : -1];
typedef char legacy_s8_must_be_1_byte[(sizeof(legacy_s8) == 1) ? 1 : -1];
typedef char legacy_u8_must_be_1_byte[(sizeof(legacy_u8) == 1) ? 1 : -1];
typedef char legacy_s16_must_be_2_bytes[(sizeof(legacy_s16) == 2) ? 1 : -1];
typedef char legacy_u16_must_be_2_bytes[(sizeof(legacy_u16) == 2) ? 1 : -1];
typedef char legacy_s32_must_be_4_bytes[(sizeof(legacy_s32) == 4) ? 1 : -1];
typedef char legacy_u32_must_be_4_bytes[(sizeof(legacy_u32) == 4) ? 1 : -1];

/* Convert byte bit patterns without plain-char signedness; pass a pure value. */
#if defined(__BORLANDC__)
#define LEGACY_S8_FROM_BITS(value) ((legacy_s8)(legacy_u8)(value))
#else
#define LEGACY_S8_FROM_BITS(value) \
	((legacy_u8)(value) <= 0x7FU ? \
	(legacy_s8)(legacy_u8)(value) : \
	(legacy_s8)(-1 - (legacy_s8)(0xFFU - (legacy_u8)(value))))
#endif

/* Pass a side-effect-free byte pointer to these little-endian accessors. */
#define LEGACY_READ_U16_LE(bytes) \
	((legacy_u16)((legacy_u16)(legacy_u8)((bytes)[0]) | \
	((legacy_u16)(legacy_u8)((bytes)[1]) << 8)))

#define LEGACY_WRITE_U16_LE(bytes, value) \
	do { \
		legacy_u16 legacy_write_u16_value_ = (legacy_u16)(value); \
		(bytes)[0] = (legacy_u8)legacy_write_u16_value_; \
		(bytes)[1] = (legacy_u8)(legacy_write_u16_value_ >> 8); \
	} while (0)

#define LEGACY_READ_U32_LE(bytes) \
	((legacy_u32)(legacy_u8)((bytes)[0]) | \
	((legacy_u32)(legacy_u8)((bytes)[1]) << 8) | \
	((legacy_u32)(legacy_u8)((bytes)[2]) << 16) | \
	((legacy_u32)(legacy_u8)((bytes)[3]) << 24))

#define LEGACY_WRITE_U32_LE(bytes, value) \
	do { \
		legacy_u32 legacy_write_u32_value_ = (legacy_u32)(value); \
		(bytes)[0] = (legacy_u8)legacy_write_u32_value_; \
		(bytes)[1] = (legacy_u8)(legacy_write_u32_value_ >> 8); \
		(bytes)[2] = (legacy_u8)(legacy_write_u32_value_ >> 16); \
		(bytes)[3] = (legacy_u8)(legacy_write_u32_value_ >> 24); \
	} while (0)

/*
 * Borland's cast is the original machine-level bit reinterpretation.  The
 * hosted implementation avoids C's implementation-defined out-of-range
 * unsigned-to-signed conversion while producing the same two's-complement
 * value.  As with the shift macro below, pass only side-effect-free values.
 */
#if defined(__BORLANDC__)
#define LEGACY_S16_FROM_BITS(value) ((legacy_s16)(legacy_u16)(value))
#else
#define LEGACY_S16_FROM_BITS(value) \
	((legacy_u16)(value) <= 0x7FFFU ? \
	(legacy_s16)(legacy_u16)(value) : \
	(legacy_s16)(-1 - (legacy_s16)(0xFFFFU - (legacy_u16)(value))))
#endif

#define LEGACY_U16_WRAP_ADD(left, right) \
	((legacy_u16)((legacy_u16)(left) + (legacy_u16)(right)))
#define LEGACY_U16_WRAP_SUB(left, right) \
	((legacy_u16)((legacy_u16)(left) - (legacy_u16)(right)))
#define LEGACY_U16_WRAP_MUL(left, right) \
	((legacy_u16)((legacy_u32)(legacy_u16)(left) * \
	(legacy_u32)(legacy_u16)(right)))
#define LEGACY_U16_WRAP_NEGATE(value) \
	((legacy_u16)(0U - (legacy_u16)(value)))
#define LEGACY_S16_WRAP_ADD(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(left, right))
#define LEGACY_S16_WRAP_SUB(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(left, right))
#define LEGACY_S16_WRAP_NEGATE(value) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_NEGATE(value))
#define LEGACY_S16_ABS_WORD(value) \
	((legacy_s16)(value) < 0 ? \
	LEGACY_S16_WRAP_NEGATE(value) : (legacy_s16)(value))
#if defined(__BORLANDC__)
#define LEGACY_S16_WRAP_NEGATE_ADD(value, addend) \
	((legacy_s16)(-(legacy_s16)(value) + (legacy_s16)(addend)))
#define LEGACY_S16_WRAP_NEGATE_SUB(value, subtrahend) \
	((legacy_s16)(-(legacy_s16)(value) - (legacy_s16)(subtrahend)))
#define LEGACY_S16_WRAP_NEGATE_SUM_ADD(value, sum_addend, final_addend) \
	((legacy_s16)(-((legacy_s16)(value) + (legacy_s16)(sum_addend)) + \
	(legacy_s16)(final_addend)))
#define LEGACY_S16_WRAP_ADD_SUB_SUB(first, second, third, fourth) \
	((first) + (second) - (third) - (fourth))
#define LEGACY_S16_WRAP_SHL1(value) ((value) << 1)
#define LEGACY_S16_WRAP_NEGATE_SHL1(value) \
	((legacy_s16)(-(legacy_s16)(value) << 1))
#else
#define LEGACY_S16_WRAP_NEGATE_ADD(value, addend) \
	LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_NEGATE(value), addend)
#define LEGACY_S16_WRAP_NEGATE_SUB(value, subtrahend) \
	LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_NEGATE(value), subtrahend)
#define LEGACY_S16_WRAP_NEGATE_SUM_ADD(value, sum_addend, final_addend) \
	LEGACY_S16_WRAP_ADD(LEGACY_S16_WRAP_NEGATE( \
		LEGACY_S16_WRAP_ADD(value, sum_addend)), final_addend)
#define LEGACY_S16_WRAP_ADD_SUB_SUB(first, second, third, fourth) \
	LEGACY_S16_WRAP_SUB(LEGACY_S16_WRAP_SUB( \
		LEGACY_S16_WRAP_ADD(first, second), third), fourth)
#define LEGACY_S16_WRAP_SHL1(value) \
	LEGACY_S16_FROM_BITS((legacy_u16)((legacy_u16)(value) << 1))
#define LEGACY_S16_WRAP_NEGATE_SHL1(value) \
	LEGACY_S16_FROM_BITS((legacy_u16)( \
		(legacy_u16)LEGACY_S16_WRAP_NEGATE(value) << 1))
#endif
#if defined(__BORLANDC__)
#define LEGACY_S16_WRAP_SHL6(value) \
	((legacy_s16)((legacy_s16)(value) << 6))
#define LEGACY_S16_WRAP_SUB_SHL6(left, right) \
	((legacy_s16)(((legacy_s16)(left) - (legacy_s16)(right)) << 6))
#define LEGACY_S16_WRAP_NEGATE_SHL6(value) \
	((legacy_s16)(-(legacy_s16)(value) << 6))
#else
#define LEGACY_S16_WRAP_SHL6(value) \
	LEGACY_S16_FROM_BITS((legacy_u16)((legacy_u16)(value) << 6))
#define LEGACY_S16_WRAP_SUB_SHL6(left, right) \
	LEGACY_S16_WRAP_SHL6(LEGACY_S16_WRAP_SUB(left, right))
#define LEGACY_S16_WRAP_NEGATE_SHL6(value) \
	LEGACY_S16_WRAP_SHL6(LEGACY_S16_WRAP_NEGATE(value))
#endif

#define LEGACY_U16_SAR1(value) \
	((legacy_u16)(((legacy_u16)(value) >> 1) | \
	((legacy_u16)(value) & 0x8000U)))
#define LEGACY_U16_SAR8(value) \
	((legacy_u16)(((legacy_u16)(value) >> 8) | \
	((((legacy_u16)(value) & 0x8000U) != 0) ? 0xFF00U : 0U)))
#define LEGACY_U16_SAR10(value) \
	((legacy_u16)(((legacy_u16)(value) >> 10) | \
	((((legacy_u16)(value) & 0x8000U) != 0) ? 0xFFC0U : 0U)))

#if defined(__BORLANDC__)
#define LEGACY_S16_SAR10(value) ((value) >> 10)
#define LEGACY_S16_WRAP_SAR10_SUB_NEGATE(value, subtrahend) \
	(-(((value) >> 10) - (subtrahend)))
#else
#define LEGACY_S16_SAR10(value) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_SAR10(value))
#define LEGACY_S16_WRAP_SAR10_SUB_NEGATE(value, subtrahend) \
	LEGACY_S16_WRAP_NEGATE(LEGACY_S16_WRAP_SUB( \
		LEGACY_S16_FROM_BITS(LEGACY_U16_SAR10(value)), subtrahend))
#endif

#define LEGACY_U32_SAR1(value) \
	((legacy_u32)(((legacy_u32)(value) >> 1) | \
	((legacy_u32)(value) & (legacy_u32)0x80000000UL)))
#define LEGACY_U32_SAR2(value) \
	LEGACY_U32_SAR1(LEGACY_U32_SAR1(value))
#define LEGACY_U32_SAR6(value) \
	((legacy_u32)(((legacy_u32)(value) >> 6) | \
	((((legacy_u32)(value) & (legacy_u32)0x80000000UL) != 0) ? \
	(legacy_u32)0xFC000000UL : (legacy_u32)0)))

#define LEGACY_U32_FROM_WORDS(high_word, low_word) \
	(((legacy_u32)(legacy_u16)(high_word) << 16) | \
	(legacy_u16)(low_word))
#define LEGACY_U32_WRAP_ADD(left, right) \
	((legacy_u32)((legacy_u32)(left) + (legacy_u32)(right)))
#define LEGACY_U32_WRAP_SUB(left, right) \
	((legacy_u32)((legacy_u32)(left) - (legacy_u32)(right)))
#define LEGACY_U32_WRAP_MUL(left, right) \
	((legacy_u32)((legacy_u32)(left) * (legacy_u32)(right)))
#define LEGACY_U32_LOW_WORD(value) ((legacy_u16)(legacy_u32)(value))
#define LEGACY_U32_HIGH_WORD(value) \
	((legacy_u16)((legacy_u32)(value) >> 16))
#define LEGACY_U32_IS_NEGATIVE(value) \
	((((legacy_u32)(value)) & (legacy_u32)0x80000000UL) != 0)

#define LEGACY_U32_SIGN_EXTEND_S16(value) \
	LEGACY_U32_FROM_WORDS( \
		(((legacy_u16)(value) & 0x8000U) != 0) ? 0xFFFFU : 0U, \
		(legacy_u16)(value))

#if defined(__BORLANDC__)
#define LEGACY_S32_FROM_BITS(value) ((legacy_s32)(legacy_u32)(value))
#else
#define LEGACY_S32_FROM_BITS(value) \
	((legacy_u32)(value) <= (legacy_u32)0x7FFFFFFFUL ? \
	(legacy_s32)(legacy_u32)(value) : \
	(legacy_s32)(-1 - (legacy_s32)( \
		(legacy_u32)0xFFFFFFFFUL - (legacy_u32)(value))))
#endif

/* Pass side-effect-free values to these signed-word/double-word operations. */
#if defined(__BORLANDC__)
#define LEGACY_S32_WRAP_ADD_S16(left, right) \
	((legacy_s32)(left) + (legacy_s16)(right))
#define LEGACY_S32_WRAP_SUB_S16(left, right) \
	((legacy_s32)(left) - (legacy_s16)(right))
#define LEGACY_S32_WRAP_SUM4_SAR2(first, second, third, fourth) \
	(((legacy_s32)(first) + (legacy_s32)(second) + \
	(legacy_s32)(third) + (legacy_s32)(fourth)) >> 2)
#define LEGACY_S16_FROM_S32_SAR6(value) \
	((legacy_s16)((legacy_s32)(value) >> 6))
#define LEGACY_S16_LOW_S32_SUB(left, right) \
	((legacy_s16)((legacy_s32)(left) - (legacy_s32)(right)))
#else
#define LEGACY_S32_WRAP_ADD_S16(left, right) \
	LEGACY_S32_FROM_BITS(LEGACY_U32_WRAP_ADD( \
		(legacy_u32)(left), LEGACY_U32_SIGN_EXTEND_S16(right)))
#define LEGACY_S32_WRAP_SUB_S16(left, right) \
	LEGACY_S32_FROM_BITS(LEGACY_U32_WRAP_SUB( \
		(legacy_u32)(left), LEGACY_U32_SIGN_EXTEND_S16(right)))
#define LEGACY_S32_WRAP_SUM4_SAR2(first, second, third, fourth) \
	LEGACY_S32_FROM_BITS(LEGACY_U32_SAR2(LEGACY_U32_WRAP_ADD( \
		LEGACY_U32_WRAP_ADD((legacy_u32)(first), (legacy_u32)(second)), \
		LEGACY_U32_WRAP_ADD((legacy_u32)(third), (legacy_u32)(fourth)))))
#define LEGACY_S16_FROM_S32_SAR6(value) \
	LEGACY_S16_FROM_BITS(LEGACY_U32_LOW_WORD( \
		LEGACY_U32_SAR6((legacy_u32)(value))))
#define LEGACY_S16_LOW_S32_SUB(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB( \
		LEGACY_U32_LOW_WORD(left), LEGACY_U32_LOW_WORD(right)))
#endif

#define LEGACY_S16_LOW_S32_ADD_S16(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U32_LOW_WORD( \
		(legacy_u32)LEGACY_S32_WRAP_ADD_S16(left, right)))
#define LEGACY_S16_HIGH_S32_ADD_S16(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U32_HIGH_WORD( \
		(legacy_u32)LEGACY_S32_WRAP_ADD_S16(left, right)))

#endif
