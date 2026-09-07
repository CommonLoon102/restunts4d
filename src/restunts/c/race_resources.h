#ifndef RESTUNTS_RACE_RESOURCES_H
#define RESTUNTS_RACE_RESOURCES_H

#include "legacy.h"

void load_sdgame2_shapes(void);
void free_sdgame2(void);
legacy_s16 setup_player_cars(void);
legacy_s16 setup_player_cars_without_dashboard(void);
void free_player_cars(void);

#endif
