# GAMESTATE

GAMESTATE is a memory structure in Stunts, defined with comprehensive labels in structs.inc in Restunts, that holds information about the player's and opponent's cars that is dynamic during game play, such as car coordinates and orientation. It is the structure that Repldump stores in its binary output files.

## Structure Definition

The structure contains the following fields:

```asm
GAMESTATE struc
game_longs1 dd 24 dup (?)
game_longs2 dd 24 dup (?)
game_longs3 dd 24 dup (?)
game_vec1 VECTOR <>
game_vec2 VECTOR <>
game_vec3 VECTOR <>
game_vec4 VECTOR <>
game_frame_in_sec dw ?
game_frames_per_sec dw ?
game_travDist dd ?
game_frame dw ?
game_total_finish dw ?
field_144 dw ?
game_pEndFrame dw ?
game_oEndFrame dw ?
game_penalty dw ?
game_impactSpeed dw ?
game_topSpeed dw ?
game_jumpCount dw ?
playerstate CARSTATE <>
opponentstate CARSTATE <>
field_2F2 dw ?
field_2F4 dw ?
game_startcol dw ?
game_startcol2 dw ?
game_startrow dw ?
game_startrow2 dw ?
field_2FE dw 24 dup (?)
field_32E dw 24 dup (?)
field_35E dw 24 dup (?)
field_38E db 48 dup (?)
field_3BE db 48 dup (?)
kevinseed db 6 dup (?)
field_3F4 db ?
game_inputmode db ?
game_3F6autoLoadEvalFlag db ?
field_3F7 db 2 dup (?)
field_3F9 db ?
field_3FA db 48 dup (?)
field_42A db ?
field_42B db 24 dup (?)
field_443 db 24 dup (?)
field_45B db ?
field_45C db ?
field_45D db ?
field_45E db ?
field_45F db ?
GAMESTATE ends
```

### Field Descriptions

Here, **VECTOR** corresponds to three words for the X, Y and Z coordinates and **VECTORLONG** uses three dwords, whereas **CARSTATE** is yet another structure with the following contents:

## CARSTATE Structure

```asm
CARSTATE struc
car_posWorld1 VECTORLONG <>
car_posWorld2 VECTORLONG <>
car_rotate VECTOR <>
car_pseudoGravity dw ?
car_steeringAngle dw ?
car_currpm dw ?
car_lastrpm dw ?
car_idlerpm2 dw ?
car_speeddiff dw ?
car_speed dw ?
car_speed2 dw ?
car_lastspeed dw ?
car_gearratio dw ?
car_gearratioshr8 dw ?
car_knob_x dw ?
car_36MwhlAngle dw ?
car_knob_y dw ?
car_knob_x2 dw ?
car_knob_y2 dw ?
car_angle_z dw ?
car_40MfrontWhlAngle dw ?
field_42 dw ?
car_demandedGrip dw ?
car_surfacegrip_sum dw ?
field_48 dw ?
car_trackdata3_index dw ?
car_rc1 dw 4 dup (?)
car_rc2 dw 4 dup (?)
car_rc3 dw 4 dup (?)
car_rc4 dw 4 dup (?)
car_rc5 dw 4 dup (?)
car_whlWorldCrds1 VECTOR 4 dup (<>)
car_whlWorldCrds2 VECTOR 4 dup (<>)
car_vec_unk3 VECTOR <>
car_vec_unk4 VECTOR <>
car_vec_unk5 VECTOR <>
field_B6 dw ?
field_B8 dw ?
field_BA dw ?
car_is_braking db ?
car_is_accelerating db ?
car_current_gear db ?
car_sumSurfFrontWheels db ?
car_sumSurfRearWheels db ?
car_sumSurfAllWheels db ?
car_surfaceWhl db 4 dup (?)
car_engineLimiterTimer db ?
car_slidingFlag db ?
field_C8 db ?
car_crashBmpFlag db ?
car_changing_gear db ?
car_fpsmul2 db ?
car_transmission db ?
field_CD db ?
field_CE db ?
field_CF db ?
CARSTATE ends
```

## Summary

All in all, the GAMESTATE structure is **1120 bytes** long.