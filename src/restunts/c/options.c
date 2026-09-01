#include "externs.h"

legacy_s8 legacy_penalty_route_enabled = 1;
static const legacy_s8 new_penalty_option[] = "/newpenalty";
static const legacy_s8 legacy_penalty_option[] = "/legacypenalty";

void parse_penalty_route_mode(legacy_s16 argc, legacy_s8* argv[])
{
	legacy_s16 index;

	legacy_penalty_route_enabled = 1;
	for (index = 1; index < argc; index++) {
		if (stricmp(argv[index], new_penalty_option) == 0)
			legacy_penalty_route_enabled = 0;
		else if (stricmp(argv[index], legacy_penalty_option) == 0)
			legacy_penalty_route_enabled = 1;
	}
}
