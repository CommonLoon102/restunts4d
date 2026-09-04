#include "restunts.h"

#define KEVINRANDOM_SEED_LEN 6

static legacy_u8 g_kevinrandom_seed[KEVINRANDOM_SEED_LEN];

void init_kevinrandom(const legacy_s8* seed)
{
	legacy_s16 i;

	for (i = 0; i < KEVINRANDOM_SEED_LEN; ++i)
		g_kevinrandom_seed[i] = (legacy_u8)seed[i];
}

void get_kevinrandom_seed(legacy_s8* seed)
{
	legacy_s16 i;

	for (i = 0; i < KEVINRANDOM_SEED_LEN; ++i)
		seed[i] = (legacy_s8)g_kevinrandom_seed[i];
}

legacy_s16 get_kevinrandom(void)
{
	legacy_s16 i;

	/* Fold every byte into the one below it, then count the seed up as one
	   big-endian number, carrying while a byte wraps to zero. */
	for (i = KEVINRANDOM_SEED_LEN - 2; i >= 0; --i)
		g_kevinrandom_seed[i] += g_kevinrandom_seed[i + 1];

	for (i = KEVINRANDOM_SEED_LEN - 1; i >= 0; --i) {
		g_kevinrandom_seed[i]++;
		if (g_kevinrandom_seed[i] != 0)
			break;
	}

	return g_kevinrandom_seed[0];
}
