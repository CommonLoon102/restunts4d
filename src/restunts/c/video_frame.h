#ifndef RESTUNTS_VIDEO_FRAME_H
#define RESTUNTS_VIDEO_FRAME_H

#include "math.h"

/* Frame rectangles and backbuffer policy shared with screen controllers. */

extern struct RECTANGLE* alternate_frame_rects;
extern struct RECTANGLE empty_rect;
extern struct RECTANGLE full_screen_rect;

legacy_s16 video_backbuffer_copy_required(void);

#endif
