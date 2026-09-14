#ifndef RESTUNTS_PIXLDUMP_MURMUR3_H
#define RESTUNTS_PIXLDUMP_MURMUR3_H

#include "../c/legacy.h"

#define PIXLDUMP_MURMUR3_HEX_DIGITS 8U

/* MurmurHash3_x86_32 with seed 0. The source must fit in one DOS segment. */
legacy_u32 pixldump_murmur3(const legacy_u8 far *source, legacy_u16 length);

#endif
