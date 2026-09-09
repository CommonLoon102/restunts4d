#ifndef RESTUNTS_PIXLDUMP_LEGACY_CONTEXT_H
#define RESTUNTS_PIXLDUMP_LEGACY_CONTEXT_H

#include "../c/legacy.h"

/* Call before removing extensions or otherwise changing the decoded arguments. */
legacy_s16 pixldump_legacy_argv_si(legacy_s16 argc, legacy_s8 *argv[]);
legacy_u16 pixldump_legacy_load_segment(void);
legacy_u16 pixldump_legacy_polygon_frame_pointer(legacy_s16 argv_si);
legacy_u16 pixldump_legacy_polygon_code_segment(void);
/* Call after the mouse, main resource, and fonts have been allocated. */
legacy_u16 pixldump_legacy_polyinfo_segment(void);

#endif
