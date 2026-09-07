#ifndef RESTUNTS_CAR_SPEED_H
#define RESTUNTS_CAR_SPEED_H

#include "legacy.h"

struct CARSTATE;
struct SIMD;
extern legacy_u8 oppnentSped[];

void update_car_speed(legacy_s8 input, legacy_s16 car_index, struct CARSTATE *carstate,
					  struct SIMD *simd);

#endif
