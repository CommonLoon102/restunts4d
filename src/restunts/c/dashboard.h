#ifndef RESTUNTS_DASHBOARD_H
#define RESTUNTS_DASHBOARD_H

#include "legacy.h"

extern legacy_s16 meter_needle_color;
extern legacy_s8 far* stdaresptr;
extern legacy_s8 far* stdbresptr;
extern struct SHAPE2D far* whlshapes[];
extern struct SHAPE2D far* gnobshapes[];
extern struct SHAPE2D far* digshapes[];
extern struct SPRITE far* whlsprite1;
extern struct SPRITE far* whlsprite2;
extern struct SPRITE far* whlsprite3;
extern legacy_s16 word_40D6C[];
extern legacy_s16 word_40D70[];
extern legacy_s16 word_40D74[];
extern legacy_s16 word_40D78[];
extern legacy_s16 word_40DF2[];
extern legacy_s16 word_40DF6[];
extern legacy_s16 word_40E00[];
extern legacy_u8 byte_40DF0[];
extern legacy_u8 byte_40DFA[];
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
