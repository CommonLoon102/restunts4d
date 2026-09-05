#include "restunts.h"

#define LEGACY_RAND_DEFAULT_SEED 1UL
#define LEGACY_RAND_MULTIPLIER 214013UL
#define LEGACY_RAND_INCREMENT 2531011UL
#define LEGACY_RAND_OUTPUT_SHIFT 16U
#define LEGACY_RAND_MAX 32767U

static legacy_u32 legacy_rand_seed = LEGACY_RAND_DEFAULT_SEED;

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
		(legacy_u32)(legacy_rand_seed * LEGACY_RAND_MULTIPLIER +
			LEGACY_RAND_INCREMENT);
	return (legacy_s16)((legacy_rand_seed >> LEGACY_RAND_OUTPUT_SHIFT) &
		LEGACY_RAND_MAX);
}
