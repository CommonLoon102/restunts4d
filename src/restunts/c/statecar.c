#include "externs.h"
#include "game_input.h"
#include "math.h"

extern legacy_u8 oppnentSped[OPPONENT_SPEED_COUNT];

#define ACCELERATION_MASS_NUMERATOR 25L
#define ACCELERATION_DRAG_SCALE 200L
#define NORMAL_FRAME_RATE 20U
#define LOW_FRAME_RATE 10U
#define NORMAL_GEAR_KNOB_STEP 6
#define LOW_RATE_GEAR_KNOB_STEP 12
#define GEAR_RATIO_BYTE_SHIFT 8U
#define AERODYNAMIC_RESISTANCE_SPEED_SHIFT 10U
#define LOW_RATE_GEAR_CHANGE_RPM_DROP 80
#define NORMAL_GEAR_CHANGE_RPM_DROP 40
#define AIRBORNE_MAX_SPEED 64000U
#define AIRBORNE_ACCELERATION 768
#define IDLE_TORQUE_RPM_THRESHOLD 2600
#define TORQUE_CURVE_RPM_SHIFT 7U
#define ENGINE_LIMITER_BLEND_RPM 5000
#define TORQUE_ACCELERATION_SHIFT 4U
#define OPPONENT_SPEED_SCALE 200U
#define OPPONENT_DRAG_SHIFT 1U
#define ENGINE_LIMITER_ACCELERATION_THRESHOLD 296
#define ENGINE_LIMITER_SHORT_TICKS 5
#define OPPONENT_BRAKING_MULTIPLIER 2
#define WRAPPED_REVERSE_SPEED_LIMIT 62720U
#define WHEEL_SPEED_SYNC_THRESHOLD 5120
#define RAPID_RPM_CHANGE_THRESHOLD 2000
#define ENGINE_TORQUE_LIMIT_THRESHOLD 12000
#define ENGINE_LIMITER_LONG_TICKS 30
#define ENGINE_LIMITER_RECOVERY_TICKS 10
#define ENGINE_SPEED_CORRECTION 1280U

static legacy_s16 scale_acceleration_by_mass(legacy_s16 acceleration,
	legacy_s16 mass)
{
	legacy_u32 product;
	legacy_u32 quotient;
	legacy_s16 low_word;

	product = (legacy_u32)LEGACY_S32_WRAP_MUL(
		(legacy_s32)acceleration, ACCELERATION_MASS_NUMERATOR);
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
	reduction = LEGACY_S32_DIV_OR_ZERO(product, ACCELERATION_DRAG_SCALE);
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

static legacy_s16 move_gear_knob_toward(legacy_s16 current,
	legacy_s16 target, legacy_s16 step)
{
	legacy_s16 difference;

	difference = LEGACY_S16_WRAP_SUB(target, current);
	if (absolute_word(difference) <= step)
		return target;
	if (difference > 0)
		return LEGACY_S16_WRAP_ADD(current, step);
	return LEGACY_S16_WRAP_SUB(current, step);
}

legacy_u16 update_rpm_from_speed(legacy_u16 currpm, legacy_u16 speed, legacy_u16 gearratio, legacy_s16 changing_gear, legacy_u16 idle_rpm) {
	if (changing_gear == 0) {
		currpm = (legacy_u16)(
			LEGACY_U32_WRAP_MUL(speed, gearratio) >> LEGACY_WORD_BITS);
	}

	if (currpm >= idle_rpm) {
		return currpm;
	}
	return idle_rpm;

}

void update_car_speed(legacy_s8 arg_carInputByte, legacy_s16 arg_MplayerFlag, struct CARSTATE* arg_carState, struct SIMD* arg_simd) {
	legacy_s16 var_2;
	legacy_s16 var_4;
	legacy_u16 var_updatedSpeed;
	legacy_s16 var_deltaSpeed;
	legacy_u8 var_currTorque;

	var_2 = framespersec == NORMAL_FRAME_RATE ?
		NORMAL_GEAR_KNOB_STEP : LOW_RATE_GEAR_KNOB_STEP;
	if (arg_carState->car_engineLimiterTimer != 0) {
		arg_carState->car_engineLimiterTimer = LEGACY_S8_WRAP_SUB(
			arg_carState->car_engineLimiterTimer, 1);
	}

	arg_carState->car_speeddiff = LEGACY_S16_WRAP_SUB(
		arg_carState->car_speed2, arg_carState->car_lastspeed);
	arg_carState->car_lastspeed = arg_carState->car_speed2;
	arg_carState->car_lastrpm = arg_carState->car_currpm;
	var_4 = 0;
	if (arg_carState->car_transmission == 0 &&
		arg_carState->car_changing_gear == 0) {
		if ((arg_carInputByte & INPUT_SHIFT_UP_FLAG) != 0)
			var_4 = 1;
		else if ((arg_carInputByte & INPUT_SHIFT_DOWN_FLAG) != 0)
			var_4 = -1;
	} else if (arg_carState->car_current_gear != 0 &&
		arg_carState->car_changing_gear == 0 &&
		arg_carState->car_sumSurfRearWheels != 0) {
		if ((legacy_u16)arg_carState->car_currpm >
			(legacy_u16)arg_simd->upshift_rpm) {
			var_4 = 1;
		} else if ((legacy_u16)arg_carState->car_currpm <
			(legacy_u16)arg_simd->downshift_rpm) {
			var_4 = -1;
		}
	}
	if (var_4 > 0 &&
		arg_carState->car_current_gear != arg_simd->num_gears) {
		arg_carState->car_current_gear = LEGACY_S8_WRAP_ADD(
			arg_carState->car_current_gear, 1);
	} else if (var_4 < 0 && arg_carState->car_current_gear > 1) {
		arg_carState->car_current_gear = LEGACY_S8_WRAP_SUB(
			arg_carState->car_current_gear, 1);
	} else {
		var_4 = 0;
	}
	if (var_4 != 0) {
		arg_carState->car_changing_gear = 1;
		arg_carState->car_fpsmul2 = gear_change_delay(framespersec);
		arg_carState->car_knob_x2 =
			arg_simd->knob_points[arg_carState->car_current_gear].px;
		arg_carState->car_knob_y2 =
			arg_simd->knob_points[arg_carState->car_current_gear].py;
	}

	if (arg_carState->car_changing_gear != 0) {
		if (arg_carState->car_knob_x == arg_carState->car_knob_x2) {
			var_4 = LEGACY_S16_WRAP_SUB(
				arg_carState->car_knob_y2, arg_carState->car_knob_y);
			if (var_4 == 0) {
				arg_carState->car_changing_gear = 0;
				arg_carState->car_gearratio =
					arg_simd->gear_ratios[arg_carState->car_current_gear];
				arg_carState->car_gearratioshr8 =
					arg_carState->car_gearratio >> GEAR_RATIO_BYTE_SHIFT;
			} else {
				arg_carState->car_knob_y = move_gear_knob_toward(
					arg_carState->car_knob_y,
					arg_carState->car_knob_y2, var_2);
			}
		} else if (arg_simd->knob_points[0].py ==
			arg_carState->car_knob_y) {
			arg_carState->car_knob_x = move_gear_knob_toward(
				arg_carState->car_knob_x,
				arg_carState->car_knob_x2, var_2);
		} else {
			arg_carState->car_knob_y = move_gear_knob_toward(
				arg_carState->car_knob_y,
				arg_simd->knob_points[0].py, var_2);
		}
	} else if (arg_carState->car_fpsmul2 != 0) {
		arg_carState->car_fpsmul2 = LEGACY_S8_WRAP_SUB(
			arg_carState->car_fpsmul2, 1);
	}

	var_updatedSpeed = arg_carState->car_speed;
	var_deltaSpeed = LEGACY_S16_WRAP_SUB(
		arg_carState->car_pseudoGravity,
		arg_simd->aerorestable[
			var_updatedSpeed >> AERODYNAMIC_RESISTANCE_SPEED_SHIFT]);
	if ((legacy_u16)arg_carState->car_currpm >
		(legacy_u16)arg_simd->max_rpm) {
		arg_carState->car_currpm = LEGACY_S16_WRAP_SUB(
			arg_simd->max_rpm, 1);
		var_deltaSpeed = LEGACY_S16_WRAP_SUB(
			var_deltaSpeed, arg_simd->braking_eff);
	} else if ((arg_carInputByte & INPUT_PEDAL_MASK) ==
		INPUT_ACCELERATE_FLAG) {
		arg_carState->car_is_braking = 0;
		arg_carState->car_is_accelerating = 1;
		if (arg_carState->car_changing_gear != 0) {
			arg_carState->car_engineLimiterTimer = 0;
			if (framespersec == LOW_FRAME_RATE) {
				arg_carState->car_currpm = LEGACY_S16_WRAP_SUB(
					arg_carState->car_currpm,
					LOW_RATE_GEAR_CHANGE_RPM_DROP);
			} else {
				arg_carState->car_currpm = LEGACY_S16_WRAP_SUB(
					arg_carState->car_currpm,
					NORMAL_GEAR_CHANGE_RPM_DROP);
			}
		} else if (arg_carState->car_sumSurfRearWheels == 0) {
			if ((legacy_u16)arg_carState->car_currpm <
				(legacy_u16)arg_simd->max_rpm &&
				var_updatedSpeed < AIRBORNE_MAX_SPEED) {
				var_deltaSpeed = LEGACY_S16_WRAP_ADD(
					var_deltaSpeed, AIRBORNE_ACCELERATION);
			}
		} else {
			if (arg_carState->car_current_gear <= 1 &&
				arg_carState->car_currpm < IDLE_TORQUE_RPM_THRESHOLD) {
				var_currTorque = arg_simd->idle_torque;
			} else {
				var_currTorque = arg_simd->torque_curve[
					(legacy_u16)arg_carState->car_currpm >>
					TORQUE_CURVE_RPM_SHIFT];
			}
			if (arg_carState->car_engineLimiterTimer != 0 &&
				arg_carState->car_currpm < ENGINE_LIMITER_BLEND_RPM) {
				var_currTorque = ((legacy_u8)arg_simd->idle_torque +
					var_currTorque) >> 1;
			}
			var_deltaSpeed = LEGACY_S16_WRAP_ADD(var_deltaSpeed,
				LEGACY_S16_FROM_BITS((legacy_u16)(LEGACY_U16_WRAP_MUL(
					arg_carState->car_gearratioshr8, var_currTorque) >>
					TORQUE_ACCELERATION_SHIFT)));
			var_deltaSpeed = scale_acceleration_by_mass(
				var_deltaSpeed, arg_simd->car_mass);
			if (arg_MplayerFlag != 0) {
				var_currTorque = (legacy_u16)(
					OPPONENT_SPEED_SCALE - *oppnentSped) >>
					OPPONENT_DRAG_SHIFT;
				if (var_currTorque != 0) {
					var_deltaSpeed = apply_opponent_acceleration_drag(
						var_deltaSpeed, var_currTorque);
				}
			}
			if (var_deltaSpeed > ENGINE_LIMITER_ACCELERATION_THRESHOLD)
				arg_carState->car_engineLimiterTimer =
					ENGINE_LIMITER_SHORT_TICKS;
		}
	} else if ((arg_carInputByte & INPUT_PEDAL_MASK) == INPUT_BRAKE_FLAG) {
		arg_carState->car_is_accelerating = 0;
		arg_carState->car_engineLimiterTimer = 0;
		arg_carState->car_is_braking = 1;
		if (arg_MplayerFlag == 0) {
			var_deltaSpeed = LEGACY_S16_WRAP_SUB(
				var_deltaSpeed, arg_simd->braking_eff);
		} else {
			var_deltaSpeed = LEGACY_S16_WRAP_SUB(var_deltaSpeed,
				LEGACY_S16_WRAP_MUL(
					arg_simd->braking_eff, OPPONENT_BRAKING_MULTIPLIER));
		}
	} else {
		arg_carState->car_is_accelerating = 0;
		arg_carState->car_is_braking = 0;
	}
	if (framespersec == LOW_FRAME_RATE) {
		var_deltaSpeed = LEGACY_S16_WRAP_ADD(
			var_deltaSpeed, var_deltaSpeed);
	}

	if (var_deltaSpeed >= 0) {
		if (var_updatedSpeed < LEGACY_U16_SIGN_BIT) {
			var_updatedSpeed = LEGACY_U16_WRAP_ADD(
				var_updatedSpeed, var_deltaSpeed);
		} else {
			var_updatedSpeed = LEGACY_U16_WRAP_ADD(
				var_updatedSpeed, var_deltaSpeed);
			if (var_updatedSpeed < LEGACY_U16_SIGN_BIT ||
				var_updatedSpeed > WRAPPED_REVERSE_SPEED_LIMIT)
				var_updatedSpeed = WRAPPED_REVERSE_SPEED_LIMIT;
		}
	} else if ((legacy_u16)LEGACY_S16_WRAP_NEGATE(var_deltaSpeed) >
		var_updatedSpeed) {
		var_updatedSpeed = 0;
	} else {
		var_updatedSpeed = LEGACY_U16_WRAP_ADD(
			var_updatedSpeed, var_deltaSpeed);
	}

	if (arg_carState->car_sumSurfRearWheels == 0) {
		arg_carState->car_speed = var_updatedSpeed;
	} else {
		var_4 = absolute_word(LEGACY_S16_WRAP_SUB(
			arg_carState->car_speed2, var_updatedSpeed));
		if (var_4 > WHEEL_SPEED_SYNC_THRESHOLD) {
			arg_carState->car_speed = (legacy_u16)(LEGACY_U32_WRAP_ADD(
				arg_carState->car_speed,
				arg_carState->car_speed2) >> 1);
			arg_carState->car_speed2 = arg_carState->car_speed;
			arg_carState->car_engineLimiterTimer =
				ENGINE_LIMITER_SHORT_TICKS;
		} else {
			arg_carState->car_speed = var_updatedSpeed;
			arg_carState->car_speed2 = var_updatedSpeed;
		}
	}

	arg_carState->car_currpm = update_rpm_from_speed(
		arg_carState->car_currpm, arg_carState->car_speed,
		arg_carState->car_gearratio, arg_carState->car_changing_gear,
		arg_simd->idle_rpm);

	if (arg_carState->car_sumSurfAllWheels != 0 &&
		arg_carState->car_lastrpm > arg_carState->car_currpm) {
		if (LEGACY_S16_WRAP_SUB(arg_carState->car_lastrpm,
			arg_carState->car_currpm) > RAPID_RPM_CHANGE_THRESHOLD) {
			if (arg_simd->idle_torque *
				arg_carState->car_gearratioshr8 >
				ENGINE_TORQUE_LIMIT_THRESHOLD) {
				arg_carState->car_engineLimiterTimer =
					ENGINE_LIMITER_LONG_TICKS;
			}
		} else if (LEGACY_S16_WRAP_SUB(arg_carState->car_currpm,
			arg_carState->car_lastrpm) > RAPID_RPM_CHANGE_THRESHOLD) {
			arg_carState->car_engineLimiterTimer =
				ENGINE_LIMITER_RECOVERY_TICKS;
			arg_carState->car_speed2 = LEGACY_U16_WRAP_SUB(
				arg_carState->car_speed2, ENGINE_SPEED_CORRECTION);
		}
	}

	if (arg_carState->car_speed2 > state.game_topSpeed) {
		state.game_topSpeed = arg_carState->car_speed2;
	}
}
