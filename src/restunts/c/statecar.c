#include "externs.h"
#include "math.h"

extern legacy_u8 oppnentSped[OPPONENT_SPEED_COUNT];

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
	legacy_s16 var_2;
	legacy_s16 var_4;
	legacy_u16 var_updatedSpeed;
	legacy_s16 var_deltaSpeed;
	legacy_u8 var_currTorque;

	var_2 = framespersec == 0x14 ? 6 : 0xC;
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
		if ((arg_carInputByte & 0x10) != 0)
			var_4 = 1;
		else if ((arg_carInputByte & 0x20) != 0)
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
					arg_carState->car_gearratio >> 8;
			} else if (car_absolute_word(var_4) <= var_2) {
				arg_carState->car_knob_y = arg_carState->car_knob_y2;
			} else if (var_4 > 0) {
				arg_carState->car_knob_y = LEGACY_S16_WRAP_ADD(
					arg_carState->car_knob_y, var_2);
			} else {
				arg_carState->car_knob_y = LEGACY_S16_WRAP_SUB(
					arg_carState->car_knob_y, var_2);
			}
		} else if (arg_simd->knob_points[0].py ==
			arg_carState->car_knob_y) {
			var_4 = LEGACY_S16_WRAP_SUB(
				arg_carState->car_knob_x2, arg_carState->car_knob_x);
			if (car_absolute_word(var_4) <= var_2) {
				arg_carState->car_knob_x = arg_carState->car_knob_x2;
			} else if (var_4 > 0) {
				arg_carState->car_knob_x = LEGACY_S16_WRAP_ADD(
					arg_carState->car_knob_x, var_2);
			} else {
				arg_carState->car_knob_x = LEGACY_S16_WRAP_SUB(
					arg_carState->car_knob_x, var_2);
			}
		} else {
			var_4 = LEGACY_S16_WRAP_SUB(
				arg_simd->knob_points[0].py, arg_carState->car_knob_y);
			if (car_absolute_word(var_4) <= var_2) {
				arg_carState->car_knob_y = arg_simd->knob_points[0].py;
			} else if (var_4 > 0) {
				arg_carState->car_knob_y = LEGACY_S16_WRAP_ADD(
					arg_carState->car_knob_y, var_2);
			} else {
				arg_carState->car_knob_y = LEGACY_S16_WRAP_SUB(
					arg_carState->car_knob_y, var_2);
			}
		}
	} else if (arg_carState->car_fpsmul2 != 0) {
		arg_carState->car_fpsmul2 = LEGACY_S8_WRAP_SUB(
			arg_carState->car_fpsmul2, 1);
	}

	var_updatedSpeed = arg_carState->car_speed;
	var_deltaSpeed = LEGACY_S16_WRAP_SUB(
		arg_carState->car_pseudoGravity,
		arg_simd->aerorestable[var_updatedSpeed >> 10]);
	if ((legacy_u16)arg_carState->car_currpm >
		(legacy_u16)arg_simd->max_rpm) {
		arg_carState->car_currpm = LEGACY_S16_WRAP_SUB(
			arg_simd->max_rpm, 1);
		var_deltaSpeed = LEGACY_S16_WRAP_SUB(
			var_deltaSpeed, arg_simd->braking_eff);
	} else if ((arg_carInputByte & 3) == 1) {
		arg_carState->car_is_braking = 0;
		arg_carState->car_is_accelerating = 1;
		if (arg_carState->car_changing_gear != 0) {
			arg_carState->car_engineLimiterTimer = 0;
			if (framespersec == 0xA) {
				arg_carState->car_currpm = LEGACY_S16_WRAP_SUB(
					arg_carState->car_currpm, 0x50);
			} else {
				arg_carState->car_currpm = LEGACY_S16_WRAP_SUB(
					arg_carState->car_currpm, 0x28);
			}
		} else if (arg_carState->car_sumSurfRearWheels == 0) {
			if ((legacy_u16)arg_carState->car_currpm <
				(legacy_u16)arg_simd->max_rpm &&
				var_updatedSpeed < 0xFA00) {
				var_deltaSpeed = LEGACY_S16_WRAP_ADD(
					var_deltaSpeed, 0x300);
			}
		} else {
			if (arg_carState->car_current_gear <= 1 &&
				arg_carState->car_currpm < 0xA28) {
				var_currTorque = arg_simd->idle_torque;
			} else {
				var_currTorque = arg_simd->torque_curve[
					(legacy_u16)arg_carState->car_currpm >> 7];
			}
			if (arg_carState->car_engineLimiterTimer != 0 &&
				arg_carState->car_currpm < 0x1388) {
				var_currTorque = ((legacy_u8)arg_simd->idle_torque +
					var_currTorque) >> 1;
			}
			var_deltaSpeed = LEGACY_S16_WRAP_ADD(var_deltaSpeed,
				LEGACY_S16_FROM_BITS((legacy_u16)(LEGACY_U16_WRAP_MUL(
					arg_carState->car_gearratioshr8, var_currTorque) >> 4)));
			var_deltaSpeed = scale_acceleration_by_mass(
				var_deltaSpeed, arg_simd->car_mass);
			if (arg_MplayerFlag != 0) {
				var_currTorque = (legacy_u16)(0xC8 - *oppnentSped) >> 1;
				if (var_currTorque != 0) {
					var_deltaSpeed = apply_opponent_acceleration_drag(
						var_deltaSpeed, var_currTorque);
				}
			}
			if (var_deltaSpeed > 0x128)
				arg_carState->car_engineLimiterTimer = 5;
		}
	} else if ((arg_carInputByte & 3) == 2) {
		arg_carState->car_is_accelerating = 0;
		arg_carState->car_engineLimiterTimer = 0;
		arg_carState->car_is_braking = 1;
		if (arg_MplayerFlag == 0) {
			var_deltaSpeed = LEGACY_S16_WRAP_SUB(
				var_deltaSpeed, arg_simd->braking_eff);
		} else {
			var_deltaSpeed = LEGACY_S16_WRAP_SUB(var_deltaSpeed,
				LEGACY_S16_WRAP_MUL(arg_simd->braking_eff, 2));
		}
	} else {
		arg_carState->car_is_accelerating = 0;
		arg_carState->car_is_braking = 0;
	}
	if (framespersec == 0xA) {
		var_deltaSpeed = LEGACY_S16_WRAP_ADD(
			var_deltaSpeed, var_deltaSpeed);
	}

	if (var_deltaSpeed >= 0) {
		if (var_updatedSpeed < 0x8000) {
			var_updatedSpeed = LEGACY_U16_WRAP_ADD(
				var_updatedSpeed, var_deltaSpeed);
		} else {
			var_updatedSpeed = LEGACY_U16_WRAP_ADD(
				var_updatedSpeed, var_deltaSpeed);
			if (var_updatedSpeed < 0x8000 || var_updatedSpeed > 0xF500)
				var_updatedSpeed = 0xF500;
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
		var_4 = car_absolute_word(LEGACY_S16_WRAP_SUB(
			arg_carState->car_speed2, var_updatedSpeed));
		if (var_4 > 0x1400) {
			arg_carState->car_speed = (legacy_u16)(LEGACY_U32_WRAP_ADD(
				arg_carState->car_speed,
				arg_carState->car_speed2) >> 1);
			arg_carState->car_speed2 = arg_carState->car_speed;
			arg_carState->car_engineLimiterTimer = 5;
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
			arg_carState->car_currpm) > 0x7D0) {
			if (arg_simd->idle_torque *
				arg_carState->car_gearratioshr8 > 0x2EE0) {
				arg_carState->car_engineLimiterTimer = 0x1E;
			}
		} else if (LEGACY_S16_WRAP_SUB(arg_carState->car_currpm,
			arg_carState->car_lastrpm) > 0x7D0) {
			arg_carState->car_engineLimiterTimer = 0xA;
			arg_carState->car_speed2 = LEGACY_U16_WRAP_SUB(
				arg_carState->car_speed2, 0x500U);
		}
	}

	if (arg_carState->car_speed2 > state.game_topSpeed) {
		state.game_topSpeed = arg_carState->car_speed2;
	}
}
