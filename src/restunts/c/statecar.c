#include "externs.h"
#include "math.h"

legacy_u16 update_rpm_from_speed(
	legacy_u16 currpm,
	legacy_u16 speed,
	legacy_u16 gearratio,
	legacy_u16 changing_gear,
	legacy_u16 idle_rpm)
{
	if (changing_gear == 0) {
		currpm = LEGACY_U32_HIGH_WORD(
			LEGACY_U32_WRAP_MUL(speed, gearratio));
	}

	if (currpm >= idle_rpm) {
		return currpm;
	}
	return idle_rpm;
/*
    mov     cx, [bp+arg_currRpm]
    mov     ax, [bp+arg_speed]
    cmp     [bp+arg_isChangingGear], 0
    jnz     short loc_19DDB
    mul     [bp+arg_gearRatio]
    mov     cx, dx
loc_19DDB:
    cmp     cx, [bp+arg_idleRpm]
    jnb     short loc_19DE3
    mov     cx, [bp+arg_idleRpm]
loc_19DE3:
    mov     ax, cx*/
}

static legacy_s16 accelerate_delta_speed_for_mass(
	legacy_s16 delta_speed,
	legacy_u16 gearratio_shift8,
	legacy_u8 torque,
	legacy_u16 mass)
{
	legacy_u16 torque_product;
	legacy_u16 delta_bits;
	legacy_u32 signed_delta_bits;
	legacy_u32 quotient;

	torque_product = LEGACY_U32_LOW_WORD(
		LEGACY_U32_WRAP_MUL(gearratio_shift8, torque));
	delta_bits = LEGACY_U16_WRAP_ADD(
		delta_speed, torque_product >> 4);
	signed_delta_bits = LEGACY_U32_FROM_WORDS(
		(delta_bits & 0x8000U) != 0 ? 0xFFFFU : 0U,
		delta_bits);
	quotient = LEGACY_U32_WRAP_MUL(signed_delta_bits, 0x19U) / mass;
	return LEGACY_S16_FROM_BITS(
		LEGACY_U16_SAR1(LEGACY_U32_LOW_WORD(quotient)));
}

static legacy_u8 opponent_speed_damping_factor(legacy_u8 opponent_speed)
{
	legacy_u16 factor_word;

	factor_word = LEGACY_U16_WRAP_SUB(opponent_speed, 0x00C8U);
	factor_word = LEGACY_U16_WRAP_NEGATE(factor_word);
	return (legacy_u8)(factor_word >> 1);
}

static legacy_s16 damp_delta_speed_for_opponent(
	legacy_s16 delta_speed,
	legacy_u8 damping_factor)
{
	legacy_u16 delta_bits;
	legacy_s32 product;
	legacy_s32 quotient;

	delta_bits = (legacy_u16)delta_speed;
	product = (legacy_s32)delta_speed * (legacy_s32)damping_factor;
#if defined(__BORLANDC__)
	/* Borland's signed division matches the original truncation toward zero. */
	quotient = product / (legacy_s32)0xC8L;
#else
	if (product < 0) {
		quotient = -((-product) / (legacy_s32)0xC8L);
	} else {
		quotient = product / (legacy_s32)0xC8L;
	}
#endif
	return LEGACY_S16_FROM_BITS(LEGACY_U16_WRAP_SUB(
		delta_bits, (legacy_u16)quotient));
}

static legacy_u16 average_speed_words(
	legacy_u16 first,
	legacy_u16 second)
{
	legacy_u32 sum;

	sum = LEGACY_U32_WRAP_ADD(first, second);
	return LEGACY_U32_LOW_WORD(sum >> 1);
}

#if defined(__BORLANDC__)
#define gear_change_frame_count(fps) \
	((char)(((char)(fps) >> 1) + (unsigned char)(fps)))
#else
static legacy_s8 gear_change_frame_count(legacy_u16 fps)
{
	legacy_u8 fps_byte;
	legacy_u16 signed_word;
	legacy_u8 half_byte;

	fps_byte = (legacy_u8)fps;
	signed_word = fps_byte <= 0x7FU ?
		fps_byte : (legacy_u16)(0xFF00U | fps_byte);
	half_byte = (legacy_u8)LEGACY_U16_SAR1(signed_word);
	return LEGACY_S8_FROM_BITS((legacy_u8)(half_byte + fps_byte));
}
#endif

#if defined(__BORLANDC__)
#define STATECAR_INCREMENT_BYTE(target) ((target)++)
#define STATECAR_DECREMENT_BYTE(target) ((target)--)
#else
static legacy_s8 statecar_increment_byte(legacy_u8 value)
{
	return LEGACY_S8_FROM_BITS((legacy_u8)(value + 1U));
}

static legacy_s8 statecar_decrement_byte(legacy_u8 value)
{
	return LEGACY_S8_FROM_BITS((legacy_u8)(value - 1U));
}

#define STATECAR_INCREMENT_BYTE(target) \
	((target) = statecar_increment_byte(target))
#define STATECAR_DECREMENT_BYTE(target) \
	((target) = statecar_decrement_byte(target))
#endif

#if defined(__BORLANDC__)
#define STATECAR_S16_SUB(left, right) ((left) - (right))
#define STATECAR_S16_DOUBLE(value) ((value) << 1)
#define STATECAR_S16_NEGATE(value) (-(value))
#define STATECAR_U16_NEGATE(value) (-(value))
#define STATECAR_S16_ADD_ASSIGN(target, value) ((target) += (value))
#define STATECAR_S16_SUB_ASSIGN(target, value) ((target) -= (value))
#define STATECAR_U16_ADD_ASSIGN(target, value) ((target) += (value))
#define STATECAR_U16_SUB_ASSIGN(target, value) ((target) -= (value))
#define STATECAR_S16_LOW_PRODUCT(left, right) \
	((legacy_s16)((left) * (right)))
#else
#define STATECAR_S16_SUB(left, right) LEGACY_S16_WRAP_SUB(left, right)
#define STATECAR_S16_DOUBLE(value) LEGACY_S16_WRAP_ADD(value, value)
#define STATECAR_S16_NEGATE(value) LEGACY_S16_WRAP_NEGATE(value)
#define STATECAR_U16_NEGATE(value) LEGACY_U16_WRAP_NEGATE(value)
#define STATECAR_S16_ADD_ASSIGN(target, value) \
	((target) = LEGACY_S16_WRAP_ADD(target, value))
#define STATECAR_S16_SUB_ASSIGN(target, value) \
	((target) = LEGACY_S16_WRAP_SUB(target, value))
#define STATECAR_U16_ADD_ASSIGN(target, value) \
	((target) = LEGACY_U16_WRAP_ADD(target, value))
#define STATECAR_U16_SUB_ASSIGN(target, value) \
	((target) = LEGACY_U16_WRAP_SUB(target, value))
#define STATECAR_S16_LOW_PRODUCT(left, right) \
	LEGACY_S16_FROM_BITS(LEGACY_U32_LOW_WORD( \
		LEGACY_U32_WRAP_MUL((legacy_u16)(left), (legacy_u16)(right))))
#endif

void update_car_speed(char arg_carInputByte, int arg_MplayerFlag, struct CARSTATE* arg_carState, struct SIMD* arg_simd) {
/*update_car_speed proc far
    var_currTorque = byte ptr -10
    var_deltaSpeed = word ptr -8
    var_updatedSpeed = word ptr -6
    var_4 = word ptr -4
    var_2 = word ptr -2
     s = byte ptr 0
     r = byte ptr 2
    arg_carInputByte = byte ptr 6
    arg_MplayerFlag = byte ptr 8
    arg_carState = word ptr 10
    arg_simd = word ptr 12
*/
	legacy_s16 var_2;
	legacy_s16 var_4;
	legacy_u16 var_updatedSpeed;
	legacy_s16 var_deltaSpeed;
	legacy_u8 var_currTorque;

	if (framespersec != 0x14)
		goto loc_17A8E;
	var_2 = 6;
	goto loc_17A93;
/*
    push    bp              ; former update_car_state
    mov     bp, sp
    sub     sp, 0Ah
    push    di
    push    si
    cmp     framespersec, 14h
    jnz     short loc_17A8E
    mov     [bp+var_2], 6
    jmp     short loc_17A93*/
loc_17A8E:
	var_2 = 0xC;
loc_17A93:
	if (arg_carState->car_engineLimiterTimer == 0)
		goto loc_17AA1;
	STATECAR_DECREMENT_BYTE(arg_carState->car_engineLimiterTimer);
/*    mov     bx, [bp+arg_carState]
    cmp     [bx+CARSTATE.car_engineLimiterTimer], 0
    jz      short loc_17AA1
    dec     [bx+CARSTATE.car_engineLimiterTimer]*/
loc_17AA1:
	arg_carState->car_speeddiff = LEGACY_S16_WRAP_SUB(
		arg_carState->car_speed2, arg_carState->car_lastspeed);
	arg_carState->car_lastspeed = arg_carState->car_speed2;
	arg_carState->car_lastrpm = arg_carState->car_currpm;
	if (arg_carState->car_transmission != 0)
		goto loc_17AE6;
	if (arg_carState->car_changing_gear != 0)
		goto loc_17AE6;
	if ((arg_carInputByte & 0x10) != 0)
		goto loc_17B0F;
	if ((arg_carInputByte & 0x20) != 0)
		goto loc_17B2E;
	goto loc_17B86;
/*
    mov     bx, [bp+arg_carState]
    mov     si, bx
    mov     ax, [si+CARSTATE.car_speed2]
    sub     ax, [si+CARSTATE.car_lastspeed]
    mov     [bx+CARSTATE.car_speeddiff], ax
    mov     bx, [bp+arg_carState]
    mov     si, bx
    mov     ax, [si+CARSTATE.car_speed2]
    mov     [bx+CARSTATE.car_lastspeed], ax
    mov     bx, [bp+arg_carState]
    mov     si, bx
    mov     ax, [si+CARSTATE.car_currpm]
    mov     [bx+CARSTATE.car_lastrpm], ax
    mov     bx, [bp+arg_carState]
    cmp     [bx+CARSTATE.car_transmission], 0
    jnz     short loc_17AE6
    cmp     [bx+CARSTATE.car_changing_gear], 0
    jnz     short loc_17AE6
    test    [bp+arg_carInputByte], 10h
    jnz     short loc_17B0F
    test    [bp+arg_carInputByte], 20h
    jnz     short loc_17B2E
    jmp     loc_17B86
    ; align 2
    db 144*/
loc_17AE6:
	if (arg_carState->car_current_gear != 0)
		goto loc_17AF0;
	goto loc_17B86;
/*    cmp     [bx+CARSTATE.car_current_gear], 0
    jnz     short loc_17AF0
    jmp     loc_17B86*/
loc_17AF0:
	if (arg_carState->car_changing_gear == 0)
		goto loc_17AFA;
	goto loc_17B86;
/*    cmp     [bx+CARSTATE.car_changing_gear], 0
    jz      short loc_17AFA
    jmp     loc_17B86*/
loc_17AFA:
	if (arg_carState->car_sumSurfRearWheels != 0)
		goto loc_17B04;
	goto loc_17B86;
/*    cmp     [bx+CARSTATE.car_sumSurfRearWheels], 0
    jnz     short loc_17B04
    jmp     loc_17B86*/
loc_17B04:
	if ((unsigned short)arg_carState->car_currpm <= (unsigned short)arg_simd->upshift_rpm)
		goto loc_17B20;
/*    mov     si, [bp+arg_simd]
    mov     ax, [si+SIMD.upshift_rpm]
    cmp     [bx+CARSTATE.car_currpm], ax
    jbe     short loc_17B20*/
loc_17B0F:
	if (arg_carState->car_current_gear == arg_simd->num_gears)
		goto loc_17B86;
	STATECAR_INCREMENT_BYTE(arg_carState->car_current_gear);
	goto loc_17B39;
/*    mov     si, [bp+arg_simd]
    mov     al, [si+SIMD.num_gears]
    cmp     [bx+CARSTATE.car_current_gear], al
    jz      short loc_17B86
    inc     [bx+CARSTATE.car_current_gear]
    jmp     short loc_17B39*/
loc_17B20:
	if ((unsigned short)arg_carState->car_currpm >= (unsigned short)arg_simd->downshift_rpm)
		goto loc_17B86;
/*    mov     bx, [bp+arg_carState]
    mov     si, [bp+arg_simd]
    mov     ax, [si+SIMD.downshift_rpm]
    cmp     [bx+CARSTATE.car_currpm], ax
    jnb     short loc_17B86*/
loc_17B2E:
	if (arg_carState->car_current_gear <= 1)
		goto loc_17B86;
	STATECAR_DECREMENT_BYTE(arg_carState->car_current_gear);
/*    cmp     [bx+CARSTATE.car_current_gear], 1
    jle     short loc_17B86
    dec     [bx+CARSTATE.car_current_gear]*/
loc_17B39:
	arg_carState->car_changing_gear = 1;
	arg_carState->car_fpsmul2 = gear_change_frame_count(framespersec);
	arg_carState->car_knob_x2 = arg_simd->knob_points[arg_carState->car_current_gear].px;
	arg_carState->car_knob_y2 = arg_simd->knob_points[arg_carState->car_current_gear].py;
	
/*    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_changing_gear], 1
    mov     al, byte ptr framespersec
    cbw
    sar     ax, 1
    add     al, byte ptr framespersec
    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_fpsmul2], al
    mov     bx, [bp+arg_carState]
    mov     al, [bx+CARSTATE.car_current_gear]
    cbw
    mov     si, ax
    shl     si, 1
    shl     si, 1
    mov     bx, [bp+arg_simd]
    mov     ax, [bx+si+SIMD.knob_points.x2]
    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_knob_x2], ax
    mov     bx, [bp+arg_carState]
    mov     al, [bx+CARSTATE.car_current_gear]
    cbw
    mov     si, ax
    shl     si, 1
    shl     si, 1
    mov     bx, [bp+arg_simd]
    mov     ax, [bx+si+SIMD.knob_points.y2]
    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_knob_y2], ax*/
loc_17B86:
	if (arg_carState->car_changing_gear != 0)
		goto loc_17B93;
	goto loc_17C9E;
/*    mov     bx, [bp+arg_carState]
    cmp     [bx+CARSTATE.car_changing_gear], 0
    jnz     short loc_17B93
    jmp     loc_17C9E*/
loc_17B93:
	if (arg_carState->car_knob_x != arg_carState->car_knob_x2)
		goto loc_17C0C;
	var_4 = LEGACY_S16_WRAP_SUB(
		arg_carState->car_knob_y2, arg_carState->car_knob_y);
	if (var_4 != 0)
		goto loc_17BDA;
	arg_carState->car_changing_gear = 0;
	arg_carState->car_gearratio = arg_simd->gear_ratios[arg_carState->car_current_gear];
	arg_carState->car_gearratioshr8 = arg_carState->car_gearratio >> 8;
	goto loc_17CAC;
/*    mov     si, bx
    mov     ax, [si+CARSTATE.car_knob_x2]
    cmp     [bx+CARSTATE.car_knob_x], ax
    jnz     short loc_17C0C
    mov     ax, [bx+CARSTATE.car_knob_y2]
    sub     ax, [bx+CARSTATE.car_knob_y]
    mov     [bp+var_4], ax
    or      ax, ax
    jnz     short loc_17BDA
    mov     [bx+CARSTATE.car_changing_gear], 0
    mov     bx, [bp+arg_carState]
    mov     al, [bx+CARSTATE.car_current_gear]
    cbw
    mov     si, ax
    shl     si, 1
    mov     bx, [bp+arg_simd]
    mov     ax, [bx+si+SIMD.gear_ratios]
    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_gearratio], ax
    mov     bx, [bp+arg_carState]
    mov     si, bx
    mov     ax, [si+CARSTATE.car_gearratio]
    mov     cl, 8
    shr     ax, cl
    mov     [bx+CARSTATE.car_gearratioshr8], ax
    jmp     loc_17CAC
    ; align 2
    db 144*/
loc_17BDA:
	if (LEGACY_S16_ABS_WORD(var_4) > var_2)
		goto loc_17BF6;
	arg_carState->car_knob_y = arg_carState->car_knob_y2;
	goto loc_17C84;
/*    push    [bp+var_4]      ; int
    call    _abs
    add     sp, 2
    cmp     ax, [bp+var_2]
    jg      short loc_17BF6
    mov     bx, [bp+arg_carState]
    mov     si, bx
    mov     ax, [si+CARSTATE.car_knob_y2]
    jmp     loc_17C84
    ; align 2
    db 144*/
loc_17BF6:
	if (var_4 <= 0)
		goto loc_17BFF;
	goto loc_17C93;
/*    cmp     [bp+var_4], 0
    jle     short loc_17BFF
    jmp     loc_17C93*/
loc_17BFF:
	arg_carState->car_knob_y = LEGACY_S16_WRAP_SUB(
		arg_carState->car_knob_y, var_2);
	goto loc_17CAC;
/*    mov     bx, [bp+arg_carState]
    mov     ax, [bp+var_2]
    sub     [bx+CARSTATE.car_knob_y], ax
    jmp     loc_17CAC
    ; align 2
    db 144*/
loc_17C0C:
	if (arg_simd->knob_points[0].py != arg_carState->car_knob_y)
		goto loc_17C5E;
	var_4 = LEGACY_S16_WRAP_SUB(
		arg_carState->car_knob_x2, arg_carState->car_knob_x);
	if (LEGACY_S16_ABS_WORD(var_4) > var_2)
		goto loc_17C40;
	arg_carState->car_knob_x = arg_carState->car_knob_x2;
	goto loc_17CAC;
/*    mov     bx, [bp+arg_simd]
    mov     si, [bp+arg_carState]
    mov     ax, [si+CARSTATE.car_knob_y]
    cmp     [bx+SIMD.knob_points.y2], ax
    jnz     short loc_17C5E
    mov     bx, si
    mov     ax, [bx+CARSTATE.car_knob_x2]
    sub     ax, [bx+CARSTATE.car_knob_x]
    mov     [bp+var_4], ax
    push    ax              ; int
    call    _abs
    add     sp, 2
    cmp     ax, [bp+var_2]
    jg      short loc_17C40
    mov     bx, [bp+arg_carState]
    mov     si, bx
    mov     ax, [si+CARSTATE.car_knob_x2]
    mov     [bx+CARSTATE.car_knob_x], ax
    jmp     short loc_17CAC*/
loc_17C40:
	if (var_4 <= 0)
		goto loc_17C52;
	arg_carState->car_knob_x = LEGACY_S16_WRAP_ADD(
		arg_carState->car_knob_x, var_2);
	goto loc_17CAC;
/*    cmp     [bp+var_4], 0
    jle     short loc_17C52
    mov     bx, [bp+arg_carState]
    mov     ax, [bp+var_2]
    add     [bx+CARSTATE.car_knob_x], ax
    jmp     short loc_17CAC
    ; align 2
    db 144*/
loc_17C52:
	arg_carState->car_knob_x = LEGACY_S16_WRAP_SUB(
		arg_carState->car_knob_x, var_2);
	goto loc_17CAC;
    /*mov     bx, [bp+arg_carState]
    mov     ax, [bp+var_2]
    sub     [bx+CARSTATE.car_knob_x], ax
    jmp     short loc_17CAC
    ; align 2
    db 144*/
loc_17C5E:
	var_4 = LEGACY_S16_WRAP_SUB(
		arg_simd->knob_points[0].py, arg_carState->car_knob_y);
	if (LEGACY_S16_ABS_WORD(var_4) > var_2)
		goto loc_17C8A;
	arg_carState->car_knob_y = arg_simd->knob_points[0].py;
/*    mov     bx, [bp+arg_simd]
    mov     ax, [bx+SIMD.knob_points.y2]
    mov     bx, [bp+arg_carState]
    sub     ax, [bx+CARSTATE.car_knob_y]
    mov     [bp+var_4], ax
    push    ax              ; int
    call    _abs
    add     sp, 2
    cmp     ax, [bp+var_2]
    jg      short loc_17C8A
    mov     bx, [bp+arg_carState]
    mov     si, [bp+arg_simd]
    mov     ax, [si+SIMD.knob_points.y2]*/
loc_17C84:
    //mov     [bx+CARSTATE.car_knob_y], ax
	goto loc_17CAC;
/*    jmp     short loc_17CAC
    ; align 2
    db 144*/
loc_17C8A:
	if (var_4 > 0)
		goto loc_17C93;
	goto loc_17BFF;
/*    cmp     [bp+var_4], 0
    jg      short loc_17C93
    jmp     loc_17BFF*/
loc_17C93:
	arg_carState->car_knob_y = LEGACY_S16_WRAP_ADD(
		arg_carState->car_knob_y, var_2);
	goto loc_17CAC;
/*    mov     bx, [bp+arg_carState]
    mov     ax, [bp+var_2]
    add     [bx+CARSTATE.car_knob_y], ax
    jmp     short loc_17CAC*/
loc_17C9E:
	if (arg_carState->car_fpsmul2 == 0)
		goto loc_17CAC;
	STATECAR_DECREMENT_BYTE(arg_carState->car_fpsmul2);
/*    mov     bx, [bp+arg_carState]
    cmp     [bx+CARSTATE.car_fpsmul2], 0
    jz      short loc_17CAC
    dec     [bx+CARSTATE.car_fpsmul2]*/
loc_17CAC:
	var_updatedSpeed = arg_carState->car_speed;
	var_deltaSpeed = STATECAR_S16_SUB(
		arg_carState->car_pseudoGravity,
		arg_simd->aerorestable[var_updatedSpeed >> 10]);
	if ((unsigned short)arg_carState->car_currpm <= (unsigned short)arg_simd->max_rpm)
		goto loc_17CEA;
	arg_carState->car_currpm = STATECAR_S16_SUB(
		arg_simd->max_rpm, 1);
/*    mov     bx, [bp+arg_carState]
    mov     ax, [bx+CARSTATE.car_speed]
    mov     [bp+var_updatedSpeed], ax
    mov     si, ax
    mov     cl, 0Ah
    shr     si, cl
    shl     si, 1           ; this is NOT part of the calculations *LOL*
    mov     bx, [bp+arg_simd]
    les     bx, dword ptr [bx+SIMD.aerorestable]
    mov     di, [bp+arg_carState]
    mov     ax, [di+CARSTATE.car_pseudoGravity]
    sub     ax, es:[bx+si]
    mov     [bp+var_deltaSpeed], ax
    mov     bx, di
    mov     si, [bp+arg_simd]
    mov     ax, [si+SIMD.max_rpm]
    cmp     [bx+CARSTATE.car_currpm], ax
    jbe     short loc_17CEA
    dec     ax
    mov     [bx+CARSTATE.car_currpm], ax*/
loc_17CE1:
	STATECAR_S16_SUB_ASSIGN(var_deltaSpeed, arg_simd->braking_eff);
	goto loc_17D36;
/*    mov     bx, [bp+arg_simd]
    mov     ax, [bx+SIMD.braking_eff]
    jmp     short loc_17D36
    ; align 2
    db 144*/
loc_17CEA:
	if ((arg_carInputByte & 3) != 1)
		goto loc_17CF8;
	goto loc_17D82;
/*    mov     al, [bp+arg_carInputByte]
smart
    and     ax, 3
nosmart
    cmp     ax, 1
    jnz     short loc_17CF8
    jmp     loc_17D82*/
loc_17CF8:
	if ((arg_carInputByte & 3) == 2)
		goto loc_17D10;
	arg_carState->car_is_accelerating = 0;
	arg_carState->car_is_braking = 0;
	goto loc_17D39;
/*    cmp     ax, 2
    jz      short loc_17D10
    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_is_accelerating], 0
    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_is_braking], 0
    jmp     short loc_17D39
    ; align 2
    db 144*/
loc_17D10:
	arg_carState->car_is_accelerating = 0;
	arg_carState->car_engineLimiterTimer = 0;
	arg_carState->car_is_braking = 1;
	if (arg_MplayerFlag == 0)
		goto loc_17CE1;
	STATECAR_S16_SUB_ASSIGN(
		var_deltaSpeed,
		STATECAR_S16_DOUBLE(arg_simd->braking_eff));
/*  mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_is_accelerating], 0
    mov     [bx+CARSTATE.car_engineLimiterTimer], 0
    mov     [bx+CARSTATE.car_is_braking], 1
    cmp     [bp+arg_MplayerFlag], 0
    jz      short loc_17CE1
    mov     bx, [bp+arg_simd]
    mov     ax, [bx+SIMD.braking_eff]
    shl     ax, 1*/
loc_17D36:
    //sub     [bp+var_deltaSpeed], ax
loc_17D39:
	if (framespersec != 0xA)
		goto loc_17D46;
	STATECAR_S16_ADD_ASSIGN(var_deltaSpeed, var_deltaSpeed);
/*    cmp     framespersec, 0Ah
    jnz     short loc_17D46
    mov     ax, [bp+var_deltaSpeed]
    add     [bp+var_deltaSpeed], ax*/
loc_17D46:
	if (var_deltaSpeed >= 0)
		goto loc_17D4F;
	goto loc_17EE2;
/*    cmp     [bp+var_deltaSpeed], 0
    jge     short loc_17D4F
    jmp     loc_17EE2*/
loc_17D4F:
	if (var_updatedSpeed < 0x8000)
		goto loc_17D59;
	goto loc_17EC2;
/*    cmp     [bp+var_updatedSpeed], 8000h
    jb      short loc_17D59
    jmp     loc_17EC2*/
loc_17D59:
	STATECAR_U16_ADD_ASSIGN(var_updatedSpeed, var_deltaSpeed);
/*    mov     ax, [bp+var_deltaSpeed]
    add     [bp+var_updatedSpeed], ax*/
loc_17D5F:
	if (arg_carState->car_sumSurfRearWheels != 0)
		goto loc_17D6C;
	goto loc_17F3C;
/*    mov     bx, [bp+arg_carState]
    cmp     [bx+CARSTATE.car_sumSurfRearWheels], 0
    jnz     short loc_17D6C
    jmp     loc_17F3C*/
loc_17D6C:
	var_4 = STATECAR_S16_SUB(
		arg_carState->car_speed2, var_updatedSpeed);
	if (var_4 < 0)
		goto loc_17D7C;
	goto loc_17EF8;
/*    mov     ax, [bx+CARSTATE.car_speed2]
    sub     ax, [bp+var_updatedSpeed]
    mov     [bp+var_4], ax
    or      ax, ax
    jl      short loc_17D7C
    jmp     loc_17EF8*/
loc_17D7C:
	var_4 = STATECAR_S16_NEGATE(var_4);
	goto loc_17EFB;
/*    neg     ax
    jmp     loc_17EFB*/
loc_17D82:
	arg_carState->car_is_braking = 0;
	arg_carState->car_is_accelerating = 1;
	if (arg_carState->car_changing_gear == 0)
		goto loc_17DBC;
	arg_carState->car_engineLimiterTimer = 0;
	if (framespersec != 0xA)
		goto loc_17DB2;
	STATECAR_S16_SUB_ASSIGN(arg_carState->car_currpm, 0x50);
	goto loc_17D39;
/*    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_is_braking], 0
    mov     [bx+CARSTATE.car_is_accelerating], 1
    cmp     [bx+CARSTATE.car_changing_gear], 0
    jz      short loc_17DBC
    mov     [bx+CARSTATE.car_engineLimiterTimer], 0
    cmp     framespersec, 0Ah
    jnz     short loc_17DB2
    sub     [bx+CARSTATE.car_currpm], 50h ; 'P'
    jmp     short loc_17D39*/
loc_17DB2:
	STATECAR_S16_SUB_ASSIGN(arg_carState->car_currpm, 0x28);
	goto loc_17D39;
/*    mov     bx, [bp+arg_carState]
    sub     [bx+CARSTATE.car_currpm], 28h ; '('
    jmp     loc_17D39*/
loc_17DBC:
	if (arg_carState->car_sumSurfRearWheels != 0)
		goto loc_17DE6;
	if ((unsigned short)arg_carState->car_currpm < (unsigned short)arg_simd->max_rpm)
		goto loc_17DD4;
	goto loc_17D39;
/*    mov     bx, [bp+arg_carState]
    cmp     [bx+CARSTATE.car_sumSurfRearWheels], 0
    jnz     short loc_17DE6
    mov     si, [bp+arg_simd]
    mov     ax, [si+SIMD.max_rpm]
    cmp     [bx+CARSTATE.car_currpm], ax
    jb      short loc_17DD4
    jmp     loc_17D39*/
loc_17DD4:
	if (var_updatedSpeed < 0xFA00)
		goto loc_17DDE;
	goto loc_17D39;
/*    cmp     [bp+var_updatedSpeed], 0FA00h
    jb      short loc_17DDE
    jmp     loc_17D39*/
loc_17DDE:
	STATECAR_S16_ADD_ASSIGN(var_deltaSpeed, 0x300);
	goto loc_17D39;
/*    add     byte ptr [bp+var_deltaSpeed+1], 3
    jmp     loc_17D39
    ; align 2
    db 144*/
loc_17DE6:
	if (arg_carState->car_current_gear > 1)
		goto loc_17DFC;
	if (arg_carState->car_currpm >= 0xA28)
		goto loc_17DFC;
	var_currTorque = arg_simd->idle_torque;
	goto loc_17E0C;
/*    cmp     [bx+CARSTATE.car_current_gear], 1
    jg      short loc_17DFC
    cmp     [bx+CARSTATE.car_currpm], 0A28h
    jge     short loc_17DFC
    mov     bx, [bp+arg_simd]
    mov     al, [bx+SIMD.idle_torque]
    jmp     short loc_17E0C*/
loc_17DFC:
	var_currTorque = arg_simd->torque_curve[(unsigned short)arg_carState->car_currpm >> 7];
/*    mov     bx, [bp+arg_carState]
    mov     si, [bx+CARSTATE.car_currpm]
    mov     cl, 7
    shr     si, cl          ; divide rpm by 2^7 to find offset.
    mov     bx, [bp+arg_simd]
    mov     al, [bx+si+SIMD.torque_curve]*/
loc_17E0C:
    //mov     [bp+var_currTorque], al
	
	if (arg_carState->car_engineLimiterTimer == 0)
		goto loc_17E34;
	if (arg_carState->car_currpm >= 0x1388)
		goto loc_17E34;
	var_currTorque = ((unsigned char)arg_simd->idle_torque + var_currTorque) >> 1;
	/*
    mov     bx, [bp+arg_carState]
    cmp     [bx+CARSTATE.car_engineLimiterTimer], 0
    jz      short loc_17E34
    cmp     [bx+CARSTATE.car_currpm], 1388h
    jge     short loc_17E34
    mov     bx, [bp+arg_simd]
    mov     al, [bx+SIMD.idle_torque]
    sub     ah, ah
    mov     cl, [bp+var_currTorque]
    sub     ch, ch
    add     ax, cx
    shr     ax, 1
    mov     [bp+var_currTorque], al*/
loc_17E34:
	var_deltaSpeed = accelerate_delta_speed_for_mass(
		var_deltaSpeed,
		arg_carState->car_gearratioshr8,
		var_currTorque,
		(legacy_u16)arg_simd->car_mass);

/*    mov     al, [bp+var_currTorque]
    sub     ah, ah
    mov     cx, ax
    mov     bx, [bp+arg_carState]
    mov     ax, [bx+CARSTATE.car_gearratioshr8]
    mul     cx
    mov     cl, 4
    shr     ax, cl          ; torque * ratio / 4096
    add     [bp+var_deltaSpeed], ax
    mov     bx, [bp+arg_simd]
    sub     ax, ax
    push    ax
    push    [bx+SIMD.car_mass]; push cwd(MASS)
    mov     ax, 19h
    cwd
    push    dx
    push    ax
    mov     ax, [bp+var_deltaSpeed]
    cwd
    push    dx
    push    ax
    call    __aFlmul
    push    dx
    push    ax
    call    __aFuldiv
    sar     ax, 1
    mov     [bp+var_deltaSpeed], ax*/
	if (arg_MplayerFlag == 0)
		goto loc_17EAD;
	var_currTorque = opponent_speed_damping_factor(*oppnentSped);
	if (var_currTorque == 0)
		goto loc_17EAD;
	var_deltaSpeed = damp_delta_speed_for_opponent(
		var_deltaSpeed,
		var_currTorque);
/*    cmp     [bp+arg_MplayerFlag], 0
    jz      short loc_17EAD
    mov     al, oppnentSped
    sub     ah, ah
    sub     ax, 0C8h ; 'È'
    neg     ax
    shr     ax, 1
    mov     [bp+var_currTorque], al
    or      al, al
    jz      short loc_17EAD
    mov     ax, 0C8h ; 'È'
    cwd
    push    dx
    push    ax
    mov     ax, [bp+var_deltaSpeed]
    cwd
    push    dx
    push    ax
    mov     al, [bp+var_currTorque]
    sub     ah, ah
    sub     cx, cx
    push    cx
    push    ax
    call    __aFlmul
    push    dx
    push    ax
    call    __aFldiv
    sub     [bp+var_deltaSpeed], ax*/
loc_17EAD:
	if (var_deltaSpeed > 0x128)
		goto loc_17EB7;
	goto loc_17D39;
/*    cmp     [bp+var_deltaSpeed], 128h; (37/32)mph, a quite large value
    jg      short loc_17EB7
    jmp     loc_17D39*/
loc_17EB7:
	arg_carState->car_engineLimiterTimer = 5;
	goto loc_17D39;
/*    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_engineLimiterTimer], 5
    jmp     loc_17D39*/
loc_17EC2:
	STATECAR_U16_ADD_ASSIGN(var_updatedSpeed, var_deltaSpeed);
	if (var_updatedSpeed < 0x8000)
		goto loc_17ED9;
	if (var_updatedSpeed > 0xF500)
		goto loc_17ED9;
	goto loc_17D5F;
/*    mov     ax, [bp+var_deltaSpeed]
    add     [bp+var_updatedSpeed], ax
    cmp     [bp+var_updatedSpeed], 8000h
    jb      short loc_17ED9
    cmp     [bp+var_updatedSpeed], 0F500h
    ja      short loc_17ED9
    jmp     loc_17D5F*/
loc_17ED9:
	var_updatedSpeed = 0xF500;
	goto loc_17D5F;
/*    mov     [bp+var_updatedSpeed], 0F500h
    jmp     loc_17D5F
    ; align 2
    db 144*/
loc_17EE2:
	if (STATECAR_U16_NEGATE(var_deltaSpeed) > var_updatedSpeed)
		goto loc_17EEF;
	goto loc_17D59;
/*    mov     ax, [bp+var_deltaSpeed]
    neg     ax
    cmp     ax, [bp+var_updatedSpeed]
    ja      short loc_17EEF
    jmp     loc_17D59*/
loc_17EEF:
	var_updatedSpeed = 0;
	goto loc_17D5F;
/*    mov     [bp+var_updatedSpeed], 0
    jmp     loc_17D5F
    ; align 2
    db 144*/
loc_17EF8:
    //mov     ax, [bp+var_4]
loc_17EFB:
	if (var_4 <= 0x1400)
		goto loc_17F28;
	arg_carState->car_speed = average_speed_words(
		arg_carState->car_speed, arg_carState->car_speed2);
	arg_carState->car_speed2 = arg_carState->car_speed;
	arg_carState->car_engineLimiterTimer = 5;
	goto loc_17F45;
/*    cmp     ax, 1400h       ; abs.delta = 20mph: a lot
    jle     short loc_17F28
    mov     si, bx
    mov     ax, [si+CARSTATE.car_speed]
    sub     dx, dx
    add     ax, [si+CARSTATE.car_speed2]
    adc     dx, dx
    shr     dx, 1
    rcr     ax, 1           ; an average
    mov     [bx+CARSTATE.car_speed], ax
    mov     bx, [bp+arg_carState]
    mov     si, bx
    mov     ax, [si+CARSTATE.car_speed]
    mov     [bx+CARSTATE.car_speed2], ax
    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_engineLimiterTimer], 5
    jmp     short loc_17F45*/
loc_17F28:
	arg_carState->car_speed = var_updatedSpeed;
	arg_carState->car_speed2 = var_updatedSpeed;
	goto loc_17F45;
/*    mov     bx, [bp+arg_carState]
    mov     ax, [bp+var_updatedSpeed]
    mov     [bx+CARSTATE.car_speed], ax
    mov     bx, [bp+arg_carState]
    mov     ax, [bp+var_updatedSpeed]
    mov     [bx+CARSTATE.car_speed2], ax
    jmp     short loc_17F45*/
loc_17F3C:
	arg_carState->car_speed = var_updatedSpeed;
/*    mov     bx, [bp+arg_carState]
    mov     ax, [bp+var_updatedSpeed]
    mov     [bx+CARSTATE.car_speed], ax*/
loc_17F45:
	arg_carState->car_currpm = update_rpm_from_speed(arg_carState->car_currpm, arg_carState->car_speed, arg_carState->car_gearratio, arg_carState->car_changing_gear, arg_simd->idle_rpm);
	/*
    mov     bx, [bp+arg_simd]
    push    [bx+SIMD.idle_rpm]
    mov     bx, [bp+arg_carState]
    mov     al, [bx+CARSTATE.car_changing_gear]
    cbw
    push    ax
    push    [bx+CARSTATE.car_gearratio]
    push    [bx+CARSTATE.car_speed]
    push    [bx+CARSTATE.car_currpm]
    call    update_rpm_from_speed
    add     sp, 0Ah
    mov     bx, [bp+arg_carState]
    mov     [bx+CARSTATE.car_currpm], ax*/
	if (arg_carState->car_sumSurfAllWheels == 0)
		goto loc_17FBF;
	if (arg_carState->car_lastrpm <= arg_carState->car_currpm)
		goto loc_17FBF;
	if (STATECAR_S16_SUB(
		arg_carState->car_lastrpm, arg_carState->car_currpm) <= 0x7D0)
		goto loc_17FA4;
	if (STATECAR_S16_LOW_PRODUCT(
		arg_simd->idle_torque,
		arg_carState->car_gearratioshr8) <= 0x2EE0)
		goto loc_17FBF;
	arg_carState->car_engineLimiterTimer = 0x1E;
	goto loc_17FBF;
/*    mov     bx, [bp+arg_carState]
    cmp     [bx+CARSTATE.car_sumSurfAllWheels], 0
    jz      short loc_17FBF
    mov     si, bx
    mov     ax, [si+CARSTATE.car_currpm]
    cmp     [bx+CARSTATE.car_lastrpm], ax
    jle     short loc_17FBF
    mov     ax, [bx+CARSTATE.car_lastrpm]
    sub     ax, [bx+CARSTATE.car_currpm]
    cmp     ax, 7D0h        ; 2000rpm
    jle     short loc_17FA4
    mov     bx, [bp+arg_simd]
    mov     al, [bx+SIMD.idle_torque]
    sub     ah, ah
    mov     bx, si
    mul     [bx+CARSTATE.car_gearratioshr8]
    cmp     ax, 2EE0h
    jle     short loc_17FBF
    mov     [bx+CARSTATE.car_engineLimiterTimer], 1Eh
    jmp     short loc_17FBF
    ; align 2
    db 144*/
loc_17FA4:
	// NOTE: signed comparison:
	if (STATECAR_S16_SUB(
		arg_carState->car_currpm, arg_carState->car_lastrpm) > 0x7D0) {
		arg_carState->car_engineLimiterTimer = 0xA;
		STATECAR_U16_SUB_ASSIGN(arg_carState->car_speed2, 0x500);
	}
/*    mov     bx, [bp+arg_carState]
    mov     ax, [bx+CARSTATE.car_currpm]
    sub     ax, [bx+CARSTATE.car_lastrpm]
    cmp     ax, 7D0h        ; 2000rpm
    jle     short loc_17FBF
    mov     [bx+CARSTATE.car_engineLimiterTimer], 0Ah
    mov     bx, [bp+arg_carState]
    sub     [bx+CARSTATE.car_speed2], 500h; 5mph*/
loc_17FBF:
	if (arg_carState->car_speed2 > state.game_topSpeed) {
		state.game_topSpeed = arg_carState->car_speed2;
	}
/*    mov     bx, [bp+arg_carState]
    mov     ax, state.game_topSpeed
    cmp     [bx+CARSTATE.car_speed2], ax
    jbe     short loc_17FD0
    mov     ax, [bx+CARSTATE.car_speed2]
    mov     state.game_topSpeed, ax*/
loc_17FD0:
	return;
}
#undef STATECAR_S16_SUB
#undef STATECAR_S16_DOUBLE
#undef STATECAR_S16_NEGATE
#undef STATECAR_U16_NEGATE
#undef STATECAR_S16_ADD_ASSIGN
#undef STATECAR_S16_SUB_ASSIGN
#undef STATECAR_U16_ADD_ASSIGN
#undef STATECAR_U16_SUB_ASSIGN
#undef STATECAR_S16_LOW_PRODUCT
/*
    pop     si
    pop     di
    mov     sp, bp
    pop     bp
    retf
update_car_speed endp
*/
