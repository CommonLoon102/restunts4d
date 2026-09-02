#ifndef RESTUNTS_REPLAY_RECORD_H
#define RESTUNTS_REPLAY_RECORD_H

#include "legacy.h"

void audio_allocate_car_state_records(void);
void set_frame_callback(void);
void remove_frame_callback(void);
void frame_callback(void);
void replay_unk2(legacy_s16 mode);
void replay_unk(void);

#endif
