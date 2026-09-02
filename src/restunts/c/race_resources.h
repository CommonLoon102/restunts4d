#ifndef RESTUNTS_RACE_RESOURCES_H
#define RESTUNTS_RACE_RESOURCES_H

#include "legacy.h"

void load_sdgame2_shapes(void);
void free_sdgame2(void);
void load_skybox(legacy_s8 skybox_index);
void unload_skybox(void);
legacy_s16 setup_player_cars(void);
legacy_s16 setup_player_cars_repldump(void);
void free_player_cars(void);

#endif
