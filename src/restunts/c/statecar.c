#include "externs.h"
#include "math.h"

extern legacy_u8 oppnentSped[10];

static legacy_s16 scale_acceleration_by_mass(legacy_s16 acceleration,
	legacy_s16 mass)
{
	legacy_u32 product;
	legacy_u32 quotient;
	legacy_s16 low_word;

	product = (legacy_u32)LEGACY_S32_WRAP_MUL(
		(legacy_s32)acceleration, 0x19L);
	quotient = LEGACY_U32_DIV_OR_ZERO(product, (legacy_u16)mass);
	low_word = LEGACY_S16_FROM_BITS((legacy_u16)quotient);
	return LEGACY_S16_SAR(low_word, 1U);
}

static legacy_s16 apply_opponent_acceleration_drag(
	legacy_s16 acceleration, legacy_u8 drag)
{
	legacy_s32 product;
	legacy_s32 reduction;

	product = LEGACY_S32_WRAP_MUL(
		(legacy_s32)(legacy_u16)drag, (legacy_s32)acceleration);
	reduction = LEGACY_S32_DIV_OR_ZERO(product, 0xC8L);
	return LEGACY_S16_WRAP_SUB(acceleration,
		LEGACY_S16_FROM_BITS((legacy_u16)reduction));
}

static legacy_s8 gear_change_delay(legacy_u16 frame_rate)
{
	legacy_s16 signed_rate;
	legacy_s16 half_rate;

	signed_rate = LEGACY_S8_FROM_BITS((legacy_u8)frame_rate);
	half_rate = LEGACY_S16_SAR(signed_rate, 1U);
	return LEGACY_S8_FROM_BITS((legacy_u8)LEGACY_U16_WRAP_ADD(
		(legacy_u8)half_rate, (legacy_u8)frame_rate));
}

static legacy_s16 car_absolute_word(legacy_s16 value)
{
	return value < 0 ? LEGACY_S16_WRAP_NEGATE(value) : value;
}

legacy_u16 update_rpm_from_speed(legacy_u16 currpm, legacy_u16 speed, legacy_u16 gearratio, legacy_s16 changing_gear, legacy_u16 idle_rpm) {
	if (changing_gear == 0) {
		currpm = (legacy_u16)(
			LEGACY_U32_WRAP_MUL(speed, gearratio) >> 16);
	}

	if (currpm >= idle_rpm) {
		return currpm;
	}
	return idle_rpm;

}

void update_car_speed(legacy_s8 arg_carInputByte, legacy_s16 arg_MplayerFlag, struct CARSTATE* arg_carState, struct SIMD* arg_simd) {
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

loc_17A8E:
	var_2 = 0xC;
loc_17A93:
	if (arg_carState->car_engineLimiterTimer == 0)
		goto loc_17AA1;
	arg_carState->car_engineLimiterTimer = LEGACY_S8_WRAP_SUB(
		arg_carState->car_engineLimiterTimer, 1);

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

loc_17AE6:
	if (arg_carState->car_current_gear != 0)
		goto loc_17AF0;
	goto loc_17B86;

loc_17AF0:
	if (arg_carState->car_changing_gear == 0)
		goto loc_17AFA;
	goto loc_17B86;

loc_17AFA:
	if (arg_carState->car_sumSurfRearWheels != 0)
		goto loc_17B04;
	goto loc_17B86;

loc_17B04:
	if ((legacy_u16)arg_carState->car_currpm <= (legacy_u16)arg_simd->upshift_rpm)
		goto loc_17B20;

loc_17B0F:
	if (arg_carState->car_current_gear == arg_simd->num_gears)
		goto loc_17B86;
	arg_carState->car_current_gear = LEGACY_S8_WRAP_ADD(
		arg_carState->car_current_gear, 1);
	goto loc_17B39;

loc_17B20:
	if ((legacy_u16)arg_carState->car_currpm >= (legacy_u16)arg_simd->downshift_rpm)
		goto loc_17B86;

loc_17B2E:
	if (arg_carState->car_current_gear <= 1)
		goto loc_17B86;
	arg_carState->car_current_gear = LEGACY_S8_WRAP_SUB(
		arg_carState->car_current_gear, 1);

loc_17B39:
	arg_carState->car_changing_gear = 1;
	arg_carState->car_fpsmul2 = gear_change_delay(framespersec);
	arg_carState->car_knob_x2 = arg_simd->knob_points[arg_carState->car_current_gear].px;
	arg_carState->car_knob_y2 = arg_simd->knob_points[arg_carState->car_current_gear].py;


loc_17B86:
	if (arg_carState->car_changing_gear != 0)
		goto loc_17B93;
	goto loc_17C9E;

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

loc_17BDA:
	if (car_absolute_word(var_4) > var_2)
		goto loc_17BF6;
	arg_carState->car_knob_y = arg_carState->car_knob_y2;
	goto loc_17C84;

loc_17BF6:
	if (var_4 <= 0)
		goto loc_17BFF;
	goto loc_17C93;

loc_17BFF:
	arg_carState->car_knob_y = LEGACY_S16_WRAP_SUB(
		arg_carState->car_knob_y, var_2);
	goto loc_17CAC;

loc_17C0C:
	if (arg_simd->knob_points[0].py != arg_carState->car_knob_y)
		goto loc_17C5E;
	var_4 = LEGACY_S16_WRAP_SUB(
		arg_carState->car_knob_x2, arg_carState->car_knob_x);
	if (car_absolute_word(var_4) > var_2)
		goto loc_17C40;
	arg_carState->car_knob_x = arg_carState->car_knob_x2;
	goto loc_17CAC;

loc_17C40:
	if (var_4 <= 0)
		goto loc_17C52;
	arg_carState->car_knob_x = LEGACY_S16_WRAP_ADD(
		arg_carState->car_knob_x, var_2);
	goto loc_17CAC;

loc_17C52:
	arg_carState->car_knob_x = LEGACY_S16_WRAP_SUB(
		arg_carState->car_knob_x, var_2);
	goto loc_17CAC;

loc_17C5E:
	var_4 = LEGACY_S16_WRAP_SUB(
		arg_simd->knob_points[0].py, arg_carState->car_knob_y);
	if (car_absolute_word(var_4) > var_2)
		goto loc_17C8A;
	arg_carState->car_knob_y = arg_simd->knob_points[0].py;

loc_17C84:
	goto loc_17CAC;

loc_17C8A:
	if (var_4 > 0)
		goto loc_17C93;
	goto loc_17BFF;

loc_17C93:
	arg_carState->car_knob_y = LEGACY_S16_WRAP_ADD(
		arg_carState->car_knob_y, var_2);
	goto loc_17CAC;

loc_17C9E:
	if (arg_carState->car_fpsmul2 == 0)
		goto loc_17CAC;
	arg_carState->car_fpsmul2 = LEGACY_S8_WRAP_SUB(
		arg_carState->car_fpsmul2, 1);

loc_17CAC:
	var_updatedSpeed = arg_carState->car_speed;
	var_deltaSpeed = LEGACY_S16_WRAP_SUB(
		arg_carState->car_pseudoGravity,
		arg_simd->aerorestable[var_updatedSpeed >> 10]);
	if ((legacy_u16)arg_carState->car_currpm <= (legacy_u16)arg_simd->max_rpm)
		goto loc_17CEA;
	arg_carState->car_currpm = LEGACY_S16_WRAP_SUB(
		arg_simd->max_rpm, 1);

loc_17CE1:
	var_deltaSpeed = LEGACY_S16_WRAP_SUB(
		var_deltaSpeed, arg_simd->braking_eff);
	goto loc_17D36;

loc_17CEA:
	if ((arg_carInputByte & 3) != 1)
		goto loc_17CF8;
	goto loc_17D82;

loc_17CF8:
	if ((arg_carInputByte & 3) == 2)
		goto loc_17D10;
	arg_carState->car_is_accelerating = 0;
	arg_carState->car_is_braking = 0;
	goto loc_17D39;

loc_17D10:
	arg_carState->car_is_accelerating = 0;
	arg_carState->car_engineLimiterTimer = 0;
	arg_carState->car_is_braking = 1;
	if (arg_MplayerFlag == 0)
		goto loc_17CE1;
	var_deltaSpeed = LEGACY_S16_WRAP_SUB(var_deltaSpeed,
		LEGACY_S16_WRAP_MUL(arg_simd->braking_eff, 2));

loc_17D36:
loc_17D39:
	if (framespersec != 0xA)
		goto loc_17D46;
	var_deltaSpeed = LEGACY_S16_WRAP_ADD(
		var_deltaSpeed, var_deltaSpeed);

loc_17D46:
	if (var_deltaSpeed >= 0)
		goto loc_17D4F;
	goto loc_17EE2;

loc_17D4F:
	if (var_updatedSpeed < 0x8000)
		goto loc_17D59;
	goto loc_17EC2;

loc_17D59:
	var_updatedSpeed = LEGACY_U16_WRAP_ADD(
		var_updatedSpeed, var_deltaSpeed);

loc_17D5F:
	if (arg_carState->car_sumSurfRearWheels != 0)
		goto loc_17D6C;
	goto loc_17F3C;

loc_17D6C:
	var_4 = LEGACY_S16_WRAP_SUB(
		arg_carState->car_speed2, var_updatedSpeed);
	if (var_4 < 0)
		goto loc_17D7C;
	goto loc_17EF8;

loc_17D7C:
	var_4 = LEGACY_S16_WRAP_NEGATE(var_4);
	goto loc_17EFB;

loc_17D82:
	arg_carState->car_is_braking = 0;
	arg_carState->car_is_accelerating = 1;
	if (arg_carState->car_changing_gear == 0)
		goto loc_17DBC;
	arg_carState->car_engineLimiterTimer = 0;
	if (framespersec != 0xA)
		goto loc_17DB2;
	arg_carState->car_currpm = LEGACY_S16_WRAP_SUB(
		arg_carState->car_currpm, 0x50);
	goto loc_17D39;

loc_17DB2:
	arg_carState->car_currpm = LEGACY_S16_WRAP_SUB(
		arg_carState->car_currpm, 0x28);
	goto loc_17D39;

loc_17DBC:
	if (arg_carState->car_sumSurfRearWheels != 0)
		goto loc_17DE6;
	if ((legacy_u16)arg_carState->car_currpm < (legacy_u16)arg_simd->max_rpm)
		goto loc_17DD4;
	goto loc_17D39;

loc_17DD4:
	if (var_updatedSpeed < 0xFA00)
		goto loc_17DDE;
	goto loc_17D39;

loc_17DDE:
	var_deltaSpeed = LEGACY_S16_WRAP_ADD(var_deltaSpeed, 0x300);
	goto loc_17D39;

loc_17DE6:
	if (arg_carState->car_current_gear > 1)
		goto loc_17DFC;
	if (arg_carState->car_currpm >= 0xA28)
		goto loc_17DFC;
	var_currTorque = arg_simd->idle_torque;
	goto loc_17E0C;

loc_17DFC:
	var_currTorque = arg_simd->torque_curve[(legacy_u16)arg_carState->car_currpm >> 7];

loc_17E0C:

	if (arg_carState->car_engineLimiterTimer == 0)
		goto loc_17E34;
	if (arg_carState->car_currpm >= 0x1388)
		goto loc_17E34;
	var_currTorque = ((legacy_u8)arg_simd->idle_torque + var_currTorque) >> 1;

loc_17E34:
	var_deltaSpeed = LEGACY_S16_WRAP_ADD(var_deltaSpeed,
		LEGACY_S16_FROM_BITS((legacy_u16)(LEGACY_U16_WRAP_MUL(
			arg_carState->car_gearratioshr8, var_currTorque) >> 4)));
	var_deltaSpeed = scale_acceleration_by_mass(
		var_deltaSpeed, arg_simd->car_mass);


	if (arg_MplayerFlag == 0)
		goto loc_17EAD;
	var_currTorque = (legacy_u16)(0xC8 - *oppnentSped) >> 1;
	if (var_currTorque == 0)
		goto loc_17EAD;
	var_deltaSpeed = apply_opponent_acceleration_drag(
		var_deltaSpeed, var_currTorque);

loc_17EAD:
	if (var_deltaSpeed > 0x128)
		goto loc_17EB7;
	goto loc_17D39;

loc_17EB7:
	arg_carState->car_engineLimiterTimer = 5;
	goto loc_17D39;

loc_17EC2:
	var_updatedSpeed = LEGACY_U16_WRAP_ADD(
		var_updatedSpeed, var_deltaSpeed);
	if (var_updatedSpeed < 0x8000)
		goto loc_17ED9;
	if (var_updatedSpeed > 0xF500)
		goto loc_17ED9;
	goto loc_17D5F;

loc_17ED9:
	var_updatedSpeed = 0xF500;
	goto loc_17D5F;

loc_17EE2:
	if ((legacy_u16)LEGACY_S16_WRAP_NEGATE(var_deltaSpeed) >
		var_updatedSpeed)
		goto loc_17EEF;
	goto loc_17D59;

loc_17EEF:
	var_updatedSpeed = 0;
	goto loc_17D5F;

loc_17EF8:
loc_17EFB:
	if (var_4 <= 0x1400)
		goto loc_17F28;
	arg_carState->car_speed = (legacy_u16)(LEGACY_U32_WRAP_ADD(
		arg_carState->car_speed, arg_carState->car_speed2) >> 1);
	arg_carState->car_speed2 = arg_carState->car_speed;
	arg_carState->car_engineLimiterTimer = 5;
	goto loc_17F45;

loc_17F28:
	arg_carState->car_speed = var_updatedSpeed;
	arg_carState->car_speed2 = var_updatedSpeed;
	goto loc_17F45;

loc_17F3C:
	arg_carState->car_speed = var_updatedSpeed;

loc_17F45:
	arg_carState->car_currpm = update_rpm_from_speed(arg_carState->car_currpm, arg_carState->car_speed, arg_carState->car_gearratio, arg_carState->car_changing_gear, arg_simd->idle_rpm);

	if (arg_carState->car_sumSurfAllWheels == 0)
		goto loc_17FBF;
	if (arg_carState->car_lastrpm <= arg_carState->car_currpm)
		goto loc_17FBF;
	if (LEGACY_S16_WRAP_SUB(arg_carState->car_lastrpm,
		arg_carState->car_currpm) <= 0x7D0)
		goto loc_17FA4;
	if (arg_simd->idle_torque * arg_carState->car_gearratioshr8 <= 0x2EE0)
		goto loc_17FBF;
	arg_carState->car_engineLimiterTimer = 0x1E;
	goto loc_17FBF;

loc_17FA4:
	// NOTE: signed comparison:
	if (LEGACY_S16_WRAP_SUB(arg_carState->car_currpm,
		arg_carState->car_lastrpm) > 0x7D0) {
		arg_carState->car_engineLimiterTimer = 0xA;
		arg_carState->car_speed2 = LEGACY_U16_WRAP_SUB(
			arg_carState->car_speed2, 0x500U);
	}

loc_17FBF:
	if (arg_carState->car_speed2 > state.game_topSpeed) {
		state.game_topSpeed = arg_carState->car_speed2;
	}

loc_17FD0:
	return;
}
