#ifndef RESTUNTS_SHAPE3D_INTERNAL_H
#define RESTUNTS_SHAPE3D_INTERNAL_H

#include "shape3d.h"

extern struct SHAPE3D game3dshapes[130];
extern legacy_s8 aStxxx[];
extern legacy_s8 far* carresptr;
extern legacy_s8 far* car2resptr;
extern struct VECTOR carshapevec[2];
extern struct VECTOR carshapevecs[];
extern struct VECTOR oppcarshapevec[2];
extern struct VECTOR oppcarshapevecs[];
extern legacy_s16 word_443E8[];
extern legacy_s16 word_4448A[];
extern legacy_s16 unk_3E710[];
extern void (*spritefunc)(legacy_s16*, legacy_s16*, legacy_u16,
	legacy_u16, legacy_u16);
extern void (*imagefunc)(legacy_u16, legacy_u16, legacy_u16,
	legacy_u16, legacy_u16);
extern legacy_u8* off_3F3C8[];

#endif
