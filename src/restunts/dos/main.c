#include <restunts.h>

legacy_s16 ported_stuntsmain_(legacy_s16 argc, legacy_s8 *argv[]);

#pragma aux ported_stuntsmain_ "ported_stuntsmain_"
#pragma aux stuntsmain "stuntsmain"

// call the implementation in seg010.asm
legacy_s16 stuntsmain(legacy_s16 argc, legacy_s8 *argv[])
{
	return ported_stuntsmain_(argc, argv);
}
