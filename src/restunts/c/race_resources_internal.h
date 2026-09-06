#ifndef RESTUNTS_RACE_RESOURCES_INTERNAL_H
#define RESTUNTS_RACE_RESOURCES_INTERNAL_H

#include "legacy.h"

extern legacy_s8 skybox_resources_loaded;
extern legacy_s8 far* skybox_res_ofs;
extern legacy_s8 far* sdgame2ptr;
extern legacy_s16 sdgame2_widths[];
extern void far* sdgame2shapes[];
extern legacy_s8 loaded_skybox_index;
extern struct SHAPE2D far* skyboxes[];

extern void far* eng1ptr;
extern void far* engptr;
extern void far* fontledresptr;
extern void far* sdgameresptr;

#endif
