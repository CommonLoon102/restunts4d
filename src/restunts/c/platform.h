#ifndef RESTUNTS_PLATFORM_H
#define RESTUNTS_PLATFORM_H

#include "legacy.h"

#ifdef RESTUNTS_DOS
void far* dos_memory_get_psp(void);
legacy_u16 dos_memory_allocate(legacy_u16 paragraphs);
legacy_u16 dos_memory_resize(legacy_u16 segment, legacy_u16 paragraphs);
#endif

#endif
