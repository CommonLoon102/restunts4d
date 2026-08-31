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
	g_kevinrandom_seed[4] += g_kevinrandom_seed[5];
	g_kevinrandom_seed[3] += g_kevinrandom_seed[4];
	g_kevinrandom_seed[2] += g_kevinrandom_seed[3];
	g_kevinrandom_seed[1] += g_kevinrandom_seed[2];
	g_kevinrandom_seed[0] += g_kevinrandom_seed[1];

	g_kevinrandom_seed[5]++;
	if (g_kevinrandom_seed[5] == 0) {
		g_kevinrandom_seed[4]++;
		if (g_kevinrandom_seed[4] == 0) {
			g_kevinrandom_seed[3]++;
			if (g_kevinrandom_seed[3] == 0) {
				g_kevinrandom_seed[2]++;
				if (g_kevinrandom_seed[2] == 0) {
					g_kevinrandom_seed[1]++;
					if (g_kevinrandom_seed[1] == 0)
						g_kevinrandom_seed[0]++;
				}
			}
		}
	}

	return g_kevinrandom_seed[0];
}
