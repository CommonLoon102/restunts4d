#ifndef RESTUNTS_DASHBOARD_H
#define RESTUNTS_DASHBOARD_H

#include "legacy.h"

enum DASHBOARD_OPERATION {
	DASHBOARD_OPERATION_LOAD = 0,
	DASHBOARD_OPERATION_REDRAW_STATIC = 1,
	DASHBOARD_OPERATION_UPDATE = 2,
	DASHBOARD_OPERATION_UNLOAD = 3
};

extern legacy_s16 meter_needle_color;
extern legacy_s8 far* stdaresptr;
extern legacy_s8 far* stdbresptr;
extern struct SHAPE2D far* whlshapes[];
extern struct SHAPE2D far* gnobshapes[];
extern struct SHAPE2D far* digshapes[];
extern struct SPRITE far* dashboard_instrument_sprite;
extern struct SPRITE far* dashboard_gearbox_sprite;
extern struct SPRITE far* dashboard_gearbox_background_sprite;
extern legacy_s16 dashboard_rpm_index_cache[];
extern legacy_s16 dashboard_gear_knob_x_cache[];
extern legacy_s16 dashboard_gear_knob_y_cache[];
extern legacy_s16 dashboard_speed_index_cache[];
extern legacy_s16 dashboard_steering_dot_x_cache[];
extern legacy_s16 dashboard_steering_dot_y_cache[];
extern legacy_s16 dashboard_steering_position_cache[];
extern legacy_u8 dashboard_wheel_shape_cache[];
extern legacy_u8 dashboard_gear_knob_visible_cache[];
extern legacy_s8 aWhl1whl2whl3ins2gboxins1i[];
extern legacy_s8 aGnobgnabdotDotadot1dot2[];
extern legacy_s8 aDig0dig1dig2dig3dig4dig5d[];
extern legacy_s8 aDash[];
extern legacy_s8 aRoof[];
extern legacy_s8 aDast[];
extern legacy_s8 aDasm[];
extern legacy_s8 aStdaxxxx[];
extern legacy_s8 aStdbxxxx[];

void setup_car_shapes(legacy_s16 operation);

#endif
