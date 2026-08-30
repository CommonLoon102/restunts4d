#include <restunts.h>

legacy_s16 ported_stuntsmain_(legacy_s16 argc, legacy_s8* argv[]);

// call the implementation in seg010.asm
legacy_s16 stuntsmain(legacy_s16 argc, legacy_s8* argv[]) {
	return ported_stuntsmain_(argc, argv);
}
