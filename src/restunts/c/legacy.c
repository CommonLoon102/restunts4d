#include "legacy.h"

legacy_u16 legacy_u16_div_or_zero(
	legacy_u16 numerator, legacy_u16 denominator)
{
	if (denominator == 0U)
		return 0U;
	return (legacy_u16)(numerator / denominator);
}

legacy_s16 legacy_s16_div_or_zero(
	legacy_s16 numerator, legacy_s16 denominator)
{
	if (denominator == 0 ||
		((legacy_u16)numerator == 0x8000U && denominator == -1))
		return 0;
	return (legacy_s16)(numerator / denominator);
}

legacy_u32 legacy_u32_div_or_zero(
	legacy_u32 numerator, legacy_u32 denominator)
{
	if (denominator == (legacy_u32)0)
		return (legacy_u32)0;
	return (legacy_u32)(numerator / denominator);
}

legacy_s32 legacy_s32_div_or_zero(
	legacy_s32 numerator, legacy_s32 denominator)
{
	if (denominator == (legacy_s32)0 ||
		((legacy_u32)numerator == (legacy_u32)0x80000000UL &&
		denominator == (legacy_s32)-1))
		return (legacy_s32)0;
	return (legacy_s32)(numerator / denominator);
}
