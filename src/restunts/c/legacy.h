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

/*
 * Borland's cast is the original machine-level bit reinterpretation.  The
 * hosted implementation avoids C's implementation-defined out-of-range
 * unsigned-to-signed conversion while producing the same two's-complement
 * value.
 */
#if defined(__BORLANDC__)
#define LEGACY_S16_FROM_BITS(value) ((legacy_s16)(legacy_u16)(value))
#else
static legacy_s16 legacy_s16_from_bits(legacy_u16 value)
{
	if (value <= 0x7FFFU)
		return (legacy_s16)value;
	return (legacy_s16)(-1 - (legacy_s16)(0xFFFFU - value));
}
#define LEGACY_S16_FROM_BITS(value) \
	legacy_s16_from_bits((legacy_u16)(value))
#endif

#define LEGACY_U16_WRAP_ADD(left, right) \
	((legacy_u16)((legacy_u16)(left) + (legacy_u16)(right)))
#define LEGACY_U16_WRAP_SUB(left, right) \
	((legacy_u16)((legacy_u16)(left) - (legacy_u16)(right)))
#define LEGACY_U16_WRAP_NEGATE(value) \
	((legacy_u16)(0U - (legacy_u16)(value)))

#define LEGACY_U16_SAR1(value) \
	((legacy_u16)(((legacy_u16)(value) >> 1) | \
	((legacy_u16)(value) & 0x8000U)))

#define LEGACY_U32_FROM_WORDS(high_word, low_word) \
	(((legacy_u32)(legacy_u16)(high_word) << 16) | \
	(legacy_u16)(low_word))
#define LEGACY_U32_LOW_WORD(value) ((legacy_u16)(legacy_u32)(value))
#define LEGACY_U32_HIGH_WORD(value) \
	((legacy_u16)((legacy_u32)(value) >> 16))
#define LEGACY_U32_IS_NEGATIVE(value) \
	((((legacy_u32)(value)) & (legacy_u32)0x80000000UL) != 0)

#endif
