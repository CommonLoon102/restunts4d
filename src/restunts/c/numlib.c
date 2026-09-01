#include "restunts.h"

static legacy_u32 legacy_rand_seed = 1UL;

legacy_u16 _abs(legacy_u16 value)
{
	if ((legacy_s16)value < 0)
		return (legacy_u16)(0U - value);
	return value;
}

void _srand(legacy_u16 seed)
{
	legacy_rand_seed = seed;
}

legacy_s16 _rand(void)
{
	legacy_rand_seed =
		(legacy_u32)(legacy_rand_seed * 214013UL + 2531011UL);
	return (legacy_s16)((legacy_rand_seed >> 16) & 0x7FFFU);
}
