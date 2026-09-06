#ifndef RESTUNTS_RESOURCE_BYTES_H
#define RESTUNTS_RESOURCE_BYTES_H

#include "legacy.h"

/* Unaligned little-endian fields in resource byte buffers. */

legacy_u16 resource_read_u16le(const legacy_u8 far* source);

legacy_u32 resource_read_u32le(const legacy_u8 far* source);

#endif
