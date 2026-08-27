#ifndef RESTUNTS_LEGACY_H
#define RESTUNTS_LEGACY_H

#include <limits.h>

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

/* Pass side-effect-free values to these legacy word operations. */
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
#define LEGACY_S16_WRAP_ADD(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_ADD(left, right))
#define LEGACY_S16_WRAP_SUB(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(left, right))
#define LEGACY_S16_WRAP_NEGATE(value) \
	LEGACY_S16_FROM_BITS((legacy_u16)(0U - (legacy_u16)(value)))
#define LEGACY_S16_WRAP_MUL(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_MUL(left, right))

#define LEGACY_U16_SAR2(value) \
	((legacy_u16)(((legacy_u16)(value) >> 2) | \
	((((legacy_u16)(value) & 0x8000U) != 0) ? 0xC000U : 0U)))
#define LEGACY_S16_SAR2(value) \
	LEGACY_S16_FROM_BITS(LEGACY_U16_SAR2(value))

#define LEGACY_READ_U16_LE(bytes) \
	((legacy_u16)((legacy_u16)((const legacy_u8*)(bytes))[0] | \
	((legacy_u16)((const legacy_u8*)(bytes))[1] << 8)))

#define LEGACY_WRITE_U16_LE(bytes, value) \
	do { \
		legacy_u16 legacy_write_u16_value_ = (legacy_u16)(value); \
		((legacy_u8*)(bytes))[0] = (legacy_u8)legacy_write_u16_value_; \
		((legacy_u8*)(bytes))[1] = \
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
#define LEGACY_S32_WRAP_ADD(left, right) \
	LEGACY_S32_FROM_BITS( \
		(legacy_u32)(left) + (legacy_u32)(right))
#define LEGACY_S32_WRAP_ADD_S16(left, right) \
	LEGACY_S32_FROM_BITS( \
		(legacy_u32)(left) + LEGACY_U32_SIGN_EXTEND_S16(right))

#endif
