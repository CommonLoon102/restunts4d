#ifndef RESTUNTS_LEGACY_H
#define RESTUNTS_LEGACY_H

#include <limits.h>

/* Borland's segmented-memory qualifier has no meaning on flat-memory hosts. */
#if !defined(__BORLANDC__) && !defined(far)
#define far
#endif
#if !defined(__BORLANDC__) && !defined(huge)
#define huge
#endif

/* Exact-width integers for behavior inherited from the 16-bit executable. */
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

typedef char legacy_byte_must_be_8_bits[(CHAR_BIT == 8) ? 1 : -1];
typedef char legacy_s16_must_be_2_bytes[(sizeof(legacy_s16) == 2) ? 1 : -1];
typedef char legacy_u16_must_be_2_bytes[(sizeof(legacy_u16) == 2) ? 1 : -1];
typedef char legacy_s32_must_be_4_bytes[(sizeof(legacy_s32) == 4) ? 1 : -1];
typedef char legacy_u32_must_be_4_bytes[(sizeof(legacy_u32) == 4) ? 1 : -1];

legacy_u16 legacy_u16_div_or_zero(
	legacy_u16 numerator, legacy_u16 denominator);
legacy_s16 legacy_s16_div_or_zero(
	legacy_s16 numerator, legacy_s16 denominator);
legacy_u32 legacy_u32_div_or_zero(
	legacy_u32 numerator, legacy_u32 denominator);
legacy_s32 legacy_s32_div_or_zero(
	legacy_s32 numerator, legacy_s32 denominator);

/* Pass side-effect-free values to these legacy word operations. */
#if defined(__BORLANDC__)
#define LEGACY_S8_FROM_BITS(value) ((legacy_s8)(legacy_u8)(value))
#define LEGACY_S16_FROM_BITS(value) ((legacy_s16)(legacy_u16)(value))
#else
#define LEGACY_S8_FROM_BITS(value) \
	((legacy_u8)(value) <= 0x7FU ? \
	(legacy_s8)(legacy_u8)(value) : \
	(legacy_s8)(-1 - (legacy_s8)(0xFFU - (legacy_u8)(value))))
#define LEGACY_S16_FROM_BITS(value) \
	((legacy_u16)(value) <= 0x7FFFU ? \
	(legacy_s16)(legacy_u16)(value) : \
	(legacy_s16)(-1 - (legacy_s16)(0xFFFFU - (legacy_u16)(value))))
#endif

#define LEGACY_U8_WRAP_ADD(left, right) \
	((legacy_u8)((legacy_u8)(left) + (legacy_u8)(right)))
#define LEGACY_U8_WRAP_SUB(left, right) \
	((legacy_u8)((legacy_u8)(left) - (legacy_u8)(right)))
#define LEGACY_U8_WRAP_MUL(left, right) \
	((legacy_u8)((legacy_u16)(legacy_u8)(left) * \
	(legacy_u16)(legacy_u8)(right)))
#define LEGACY_S8_WRAP_ADD(left, right) \
	LEGACY_S8_FROM_BITS(LEGACY_U8_WRAP_ADD(left, right))
#define LEGACY_S8_WRAP_SUB(left, right) \
	LEGACY_S8_FROM_BITS(LEGACY_U8_WRAP_SUB(left, right))
#define LEGACY_S8_WRAP_NEGATE(value) \
	LEGACY_S8_FROM_BITS((legacy_u8)(0U - (legacy_u8)(value)))
#define LEGACY_S8_WRAP_MUL(left, right) \
	LEGACY_S8_FROM_BITS(LEGACY_U8_WRAP_MUL(left, right))

#define LEGACY_U16_WRAP_ADD(left, right) \
	((legacy_u16)((legacy_u16)(left) + (legacy_u16)(right)))
#define LEGACY_U16_WRAP_SUB(left, right) \
	((legacy_u16)((legacy_u16)(left) - (legacy_u16)(right)))
#define LEGACY_U16_WRAP_MUL(left, right) \
	((legacy_u16)((legacy_u32)(legacy_u16)(left) * \
	(legacy_u32)(legacy_u16)(right)))
#define LEGACY_S16_WRAP_ADD(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(left, right))
#define LEGACY_S16_WRAP_SUB(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(left, right))
#define LEGACY_S16_WRAP_NEGATE(value) \
	LEGACY_S16_FROM_BITS((legacy_u16)(0U - (legacy_u16)(value)))
#define LEGACY_S16_WRAP_MUL(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_MUL(left, right))

#define LEGACY_U16_LOW_BYTE(value) \
	((legacy_u8)(legacy_u16)(value))
#define LEGACY_U16_REPLACE_LOW_BYTE(word, value) \
	((legacy_u16)(((legacy_u16)(word) & 0xFF00U) | \
	(legacy_u16)(legacy_u8)(value)))

/* x86 masks variable shift counts to five bits. */
#define LEGACY_SHIFT_COUNT(value) ((legacy_u16)(value) & 0x1FU)
#define LEGACY_ROTATE16_COUNT(value) ((legacy_u16)(value) & 0x0FU)

#define LEGACY_U16_SAR(value, count) \
	((legacy_u16)(LEGACY_SHIFT_COUNT(count) == 0U ? \
		(legacy_u16)(value) : \
		(((legacy_u16)(value) & 0x8000U) != 0U ? \
			(LEGACY_SHIFT_COUNT(count) >= 16U ? \
			(legacy_u32)0xFFFFUL : \
			(legacy_u32)((legacy_u16)(value) >> \
				LEGACY_SHIFT_COUNT(count)) | \
			(legacy_u16)((legacy_u32)0xFFFFUL << \
				(16U - LEGACY_SHIFT_COUNT(count)))) : \
			(LEGACY_SHIFT_COUNT(count) >= 16U ? 0UL : \
			(legacy_u32)((legacy_u16)(value) >> \
				LEGACY_SHIFT_COUNT(count))))))
#define LEGACY_S16_SAR(value, count) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_SAR(value, count))
#define LEGACY_U16_SAR2(value) LEGACY_U16_SAR(value, 2U)
#define LEGACY_S16_SAR2(value) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_SAR2(value))

#define LEGACY_U16_SHL(value, count) \
	((legacy_u16)(LEGACY_SHIFT_COUNT(count) >= 16U ? 0UL : \
	(legacy_u32)(legacy_u16)(value) << LEGACY_SHIFT_COUNT(count)))
#define LEGACY_S16_SHL(value, count) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_SHL(value, count))

#define LEGACY_U16_ROL(value, count) \
	((legacy_u16)(LEGACY_ROTATE16_COUNT(count) == 0U ? \
		(legacy_u16)(value) : \
		((legacy_u32)(legacy_u16)(value) << \
			LEGACY_ROTATE16_COUNT(count)) | \
		((legacy_u16)(value) >> \
			(16U - LEGACY_ROTATE16_COUNT(count)))))
#define LEGACY_U16_ROR(value, count) \
	((legacy_u16)(LEGACY_ROTATE16_COUNT(count) == 0U ? \
		(legacy_u16)(value) : \
		((legacy_u16)(value) >> LEGACY_ROTATE16_COUNT(count)) | \
		((legacy_u32)(legacy_u16)(value) << \
			(16U - LEGACY_ROTATE16_COUNT(count)))))

#define LEGACY_U16_MUL_HIGH(left, right) \
	((legacy_u16)(((legacy_u32)(legacy_u16)(left) * \
	(legacy_u32)(legacy_u16)(right)) >> 16))
#define LEGACY_S16_MUL_HIGH(left, right) \
	LEGACY_S16_FROM_BITS((legacy_u16)LEGACY_U32_SAR((legacy_u32)( \
		(legacy_s32)LEGACY_S16_FROM_BITS(left) * \
		(legacy_s32)LEGACY_S16_FROM_BITS(right)), 16U))

/* The original #DE recovery skips a faulting division with a zero result. */
#define LEGACY_U16_DIV_OR_ZERO(numerator, denominator) \
	legacy_u16_div_or_zero((legacy_u16)(numerator), \
		(legacy_u16)(denominator))
#define LEGACY_S16_DIV_OR_ZERO(numerator, denominator) \
	legacy_s16_div_or_zero(LEGACY_S16_FROM_BITS(numerator), \
		LEGACY_S16_FROM_BITS(denominator))

#define LEGACY_READ_U16_LE(bytes) \
	((legacy_u16)((legacy_u16)(legacy_u8)(bytes)[0] | \
	((legacy_u16)(legacy_u8)(bytes)[1] << 8)))

#define LEGACY_WRITE_U16_LE(bytes, value) \
	do { \
		legacy_u16 legacy_write_u16_value_ = (legacy_u16)(value); \
		(bytes)[0] = (legacy_u8)legacy_write_u16_value_; \
		(bytes)[1] = \
			(legacy_u8)(legacy_write_u16_value_ >> 8); \
	} while (0)

#if defined(__BORLANDC__)
#define LEGACY_S32_FROM_BITS(value) ((legacy_s32)(legacy_u32)(value))
#else
#define LEGACY_S32_FROM_BITS(value) \
	((legacy_u32)(value) <= (legacy_u32)0x7FFFFFFFUL ? \
	(legacy_s32)(legacy_u32)(value) : \
	(legacy_s32)(-1 - (legacy_s32)( \
		(legacy_u32)0xFFFFFFFFUL - (legacy_u32)(value))))
#endif

#define LEGACY_U32_SIGN_EXTEND_S16(value) \
	((legacy_u32)( \
		(((legacy_u16)(value) & 0x8000U) != 0 ? \
		(legacy_u32)0xFFFF0000UL : (legacy_u32)0) | \
		(legacy_u16)(value)))
#define LEGACY_U32_FROM_WORDS(low_word, high_word) \
	((legacy_u32)((legacy_u16)(low_word) | \
		((legacy_u32)(legacy_u16)(high_word) << 16)))
#define LEGACY_S32_WRAP_ADD(left, right) \
	LEGACY_S32_FROM_BITS( \
		(legacy_u32)(left) + (legacy_u32)(right))
#define LEGACY_S32_WRAP_ADD_S16(left, right) \
	LEGACY_S32_FROM_BITS( \
		(legacy_u32)(left) + LEGACY_U32_SIGN_EXTEND_S16(right))
#define LEGACY_S32_WRAP_SUB_S16(left, right) \
	LEGACY_S32_FROM_BITS( \
		(legacy_u32)(left) - LEGACY_U32_SIGN_EXTEND_S16(right))

#define LEGACY_U32_WRAP_ADD(left, right) \
	((legacy_u32)((legacy_u32)(left) + (legacy_u32)(right)))
#define LEGACY_U32_WRAP_SUB(left, right) \
	((legacy_u32)((legacy_u32)(left) - (legacy_u32)(right)))
#define LEGACY_U32_WRAP_MUL(left, right) \
	((legacy_u32)((legacy_u32)(left) * (legacy_u32)(right)))
#define LEGACY_S32_WRAP_SUB(left, right) \
	LEGACY_S32_FROM_BITS(LEGACY_U32_WRAP_SUB(left, right))
#define LEGACY_S32_WRAP_NEGATE(value) \
	LEGACY_S32_FROM_BITS((legacy_u32)((legacy_u32)0 - \
		(legacy_u32)(value)))
#define LEGACY_S32_WRAP_MUL(left, right) \
	LEGACY_S32_FROM_BITS(LEGACY_U32_WRAP_MUL(left, right))

#define LEGACY_U32_SAR(value, count) \
	((legacy_u32)(LEGACY_SHIFT_COUNT(count) == 0U ? \
		(legacy_u32)(value) : \
		(((legacy_u32)(value) & (legacy_u32)0x80000000UL) != 0UL ? \
			((legacy_u32)(value) >> LEGACY_SHIFT_COUNT(count)) | \
			((legacy_u32)0xFFFFFFFFUL << \
				(32U - LEGACY_SHIFT_COUNT(count))) : \
			(legacy_u32)(value) >> LEGACY_SHIFT_COUNT(count))))
#define LEGACY_S32_SAR(value, count) \
	LEGACY_S32_FROM_BITS(LEGACY_U32_SAR(value, count))

#define LEGACY_U32_SHL(value, count) \
	((legacy_u32)(value) << LEGACY_SHIFT_COUNT(count))
#define LEGACY_S32_SHL(value, count) \
	LEGACY_S32_FROM_BITS(LEGACY_U32_SHL(value, count))

#define LEGACY_U32_ROL(value, count) \
	((legacy_u32)(LEGACY_SHIFT_COUNT(count) == 0U ? \
		(legacy_u32)(value) : \
		((legacy_u32)(value) << LEGACY_SHIFT_COUNT(count)) | \
		((legacy_u32)(value) >> \
			(32U - LEGACY_SHIFT_COUNT(count)))))
#define LEGACY_U32_ROR(value, count) \
	((legacy_u32)(LEGACY_SHIFT_COUNT(count) == 0U ? \
		(legacy_u32)(value) : \
		((legacy_u32)(value) >> LEGACY_SHIFT_COUNT(count)) | \
		((legacy_u32)(value) << \
			(32U - LEGACY_SHIFT_COUNT(count)))))

#define LEGACY_U32_DIV_OR_ZERO(numerator, denominator) \
	legacy_u32_div_or_zero((legacy_u32)(numerator), \
		(legacy_u32)(denominator))
#define LEGACY_S32_DIV_OR_ZERO(numerator, denominator) \
	legacy_s32_div_or_zero(LEGACY_S32_FROM_BITS(numerator), \
		LEGACY_S32_FROM_BITS(denominator))

#define LEGACY_READ_U32_LE(bytes) \
	((legacy_u32)((legacy_u32)(legacy_u8)(bytes)[0] | \
	((legacy_u32)(legacy_u8)(bytes)[1] << 8) | \
	((legacy_u32)(legacy_u8)(bytes)[2] << 16) | \
	((legacy_u32)(legacy_u8)(bytes)[3] << 24)))
#define LEGACY_READ_S16_LE(bytes) \
	LEGACY_S16_FROM_BITS(LEGACY_READ_U16_LE(bytes))
#define LEGACY_READ_S32_LE(bytes) \
	LEGACY_S32_FROM_BITS(LEGACY_READ_U32_LE(bytes))

#define LEGACY_WRITE_U32_LE(bytes, value) \
	do { \
		legacy_u32 legacy_write_u32_value_ = (legacy_u32)(value); \
		(bytes)[0] = (legacy_u8)legacy_write_u32_value_; \
		(bytes)[1] = \
			(legacy_u8)(legacy_write_u32_value_ >> 8); \
		(bytes)[2] = \
			(legacy_u8)(legacy_write_u32_value_ >> 16); \
		(bytes)[3] = \
			(legacy_u8)(legacy_write_u32_value_ >> 24); \
	} while (0)

#endif
