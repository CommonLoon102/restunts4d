#include "legacy.h"

#define LEGACY_U16_DIVIDE_ZERO_VALUE 0U
#define LEGACY_S16_DIVIDE_ZERO_VALUE 0
#define LEGACY_S16_DIVIDE_OVERFLOW_DENOMINATOR (-1)
#define LEGACY_U32_DIVIDE_ZERO_VALUE ((legacy_u32)0)
#define LEGACY_S32_DIVIDE_ZERO_VALUE ((legacy_s32)0)
#define LEGACY_S32_DIVIDE_OVERFLOW_DENOMINATOR ((legacy_s32)-1)

legacy_u16 legacy_u16_div_or_zero(
	legacy_u16 numerator, legacy_u16 denominator)
{
	if (denominator == LEGACY_U16_DIVIDE_ZERO_VALUE)
		return LEGACY_U16_DIVIDE_ZERO_VALUE;
	return (legacy_u16)(numerator / denominator);
}

legacy_s16 legacy_s16_div_or_zero(
	legacy_s16 numerator, legacy_s16 denominator)
{
	if (denominator == LEGACY_S16_DIVIDE_ZERO_VALUE ||
		((legacy_u16)numerator == LEGACY_U16_SIGN_BIT &&
		denominator == LEGACY_S16_DIVIDE_OVERFLOW_DENOMINATOR))
		return LEGACY_S16_DIVIDE_ZERO_VALUE;
	return (legacy_s16)(numerator / denominator);
}

legacy_u32 legacy_u32_div_or_zero(
	legacy_u32 numerator, legacy_u32 denominator)
{
	if (denominator == LEGACY_U32_DIVIDE_ZERO_VALUE)
		return LEGACY_U32_DIVIDE_ZERO_VALUE;
	return (legacy_u32)(numerator / denominator);
}

legacy_s32 legacy_s32_div_or_zero(
	legacy_s32 numerator, legacy_s32 denominator)
{
	if (denominator == LEGACY_S32_DIVIDE_ZERO_VALUE ||
		((legacy_u32)numerator == (legacy_u32)LEGACY_U32_SIGN_BIT &&
		denominator == LEGACY_S32_DIVIDE_OVERFLOW_DENOMINATOR))
		return LEGACY_S32_DIVIDE_ZERO_VALUE;
	return (legacy_s32)(numerator / denominator);
}
