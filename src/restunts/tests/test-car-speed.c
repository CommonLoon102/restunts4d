#include <assert.h>
#include <string.h>

#include "../c/externs.h"
#include "../c/car_speed.h"
#include "../c/game_input.h"

struct GAMESTATE state;
legacy_u16 framespersec;
legacy_u8 oppnentSped[1];

static struct SIMD simd;
static struct CARSTATE car;
static legacy_s16 aerodynamic_drag[64];

static void reset_car(void)
{
	unsigned index;

	memset(&state, 0, sizeof(state));
	memset(&simd, 0, sizeof(simd));
	memset(&car, 0, sizeof(car));
	memset(aerodynamic_drag, 0, sizeof(aerodynamic_drag));
	framespersec = GAME_FRAME_RATE_NORMAL;
	oppnentSped[0] = 200;
	simd.num_gears = 5;
	simd.car_mass = 25;
	simd.braking_eff = 100;
	simd.idle_rpm = 1000;
	simd.downshift_rpm = 2000;
	simd.upshift_rpm = 6000;
	simd.max_rpm = 12000;
	simd.idle_torque = 32;
	simd.aerorestable = aerodynamic_drag;
	for (index = 0; index < SIMD_GEAR_RATIO_COUNT; index++) {
		simd.gear_ratios[index] = 4096;
		simd.knob_points[index].px = (legacy_s16)(index * 12);
		simd.knob_points[index].py = 12;
	}
	simd.knob_points[0].py = 0;
	memset(simd.torque_curve, 32, sizeof(simd.torque_curve));
	car.car_transmission = TRANSMISSION_MANUAL;
	car.car_current_gear = 1;
	car.car_currpm = 1000;
	car.car_gearratio = 4096;
	car.car_gearratioshr8 = 16;
	car.car_sumSurfRearWheels = 2;
	car.car_sumSurfAllWheels = 4;
	car.car_rev_speed = 16000;
	car.car_actual_speed = 16000;
	car.car_knob_x = simd.knob_points[1].px;
	car.car_knob_y = simd.knob_points[1].py;
}

static void test_shift_precedence_and_limits(void)
{
	reset_car();
	update_car_speed(INPUT_SHIFT_UP_FLAG | INPUT_SHIFT_DOWN_FLAG, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_current_gear == 2);
	assert(car.car_changing_gear == CAR_GEAR_CHANGE_ACTIVE);
	assert(car.car_gear_change_delay == 30);
	assert(car.car_knob_y == 6);

	reset_car();
	car.car_current_gear = simd.num_gears;
	update_car_speed(INPUT_SHIFT_UP_FLAG | INPUT_SHIFT_DOWN_FLAG, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_current_gear == simd.num_gears);
	assert(car.car_changing_gear == CAR_GEAR_CHANGE_INACTIVE);

	reset_car();
	update_car_speed(INPUT_SHIFT_DOWN_FLAG, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_current_gear == 1);
	assert(car.car_changing_gear == CAR_GEAR_CHANGE_INACTIVE);
}

static void test_automatic_shift_contact_and_threshold(void)
{
	reset_car();
	car.car_transmission = TRANSMISSION_AUTOMATIC;
	car.car_currpm = simd.upshift_rpm;
	update_car_speed(INPUT_NONE, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_current_gear == 1);

	car.car_currpm = simd.upshift_rpm + 1;
	car.car_sumSurfRearWheels = 0;
	update_car_speed(INPUT_NONE, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_current_gear == 1);

	car.car_currpm = simd.upshift_rpm + 1;
	car.car_sumSurfRearWheels = 2;
	update_car_speed(INPUT_NONE, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_current_gear == 2);
}

static void test_shift_completion_and_delay(void)
{
	reset_car();
	car.car_current_gear = 2;
	car.car_changing_gear = CAR_GEAR_CHANGE_ACTIVE;
	car.car_knob_x2 = car.car_knob_x;
	car.car_knob_y2 = car.car_knob_y;
	car.car_gear_change_delay = 10;
	simd.gear_ratios[2] = 2048;
	update_car_speed(INPUT_NONE, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_changing_gear == CAR_GEAR_CHANGE_INACTIVE);
	assert(car.car_gearratio == 2048);
	assert(car.car_gearratioshr8 == 8);
	assert(car.car_gear_change_delay == 10);
	update_car_speed(INPUT_NONE, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_gear_change_delay == 9);
}

static void test_pedals_and_frame_rates(void)
{
	reset_car();
	update_car_speed(INPUT_ACCELERATE_FLAG, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_actual_speed == 16016);
	assert(car.car_is_accelerating == CAR_PEDAL_PRESSED);
	assert(state.game_topSpeed == 16016);

	reset_car();
	framespersec = GAME_FRAME_RATE_LOW;
	update_car_speed(INPUT_ACCELERATE_FLAG, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_actual_speed == 16032);

	reset_car();
	oppnentSped[0] = 100;
	update_car_speed(INPUT_ACCELERATE_FLAG, OPPONENT_CAR_INDEX, &car, &simd);
	assert(car.car_actual_speed == 16012);

	reset_car();
	update_car_speed(INPUT_BRAKE_FLAG, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_actual_speed == 15900);
	assert(car.car_is_braking == CAR_PEDAL_PRESSED);

	reset_car();
	update_car_speed(INPUT_BRAKE_FLAG, OPPONENT_CAR_INDEX, &car, &simd);
	assert(car.car_actual_speed == 15800);

	reset_car();
	update_car_speed(INPUT_PEDAL_MASK, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_actual_speed == 16000);
	assert(car.car_is_braking == CAR_PEDAL_RELEASED);
	assert(car.car_is_accelerating == CAR_PEDAL_RELEASED);
}

static void test_overrev_preserves_pedal_state(void)
{
	reset_car();
	car.car_currpm = simd.max_rpm + 1;
	car.car_is_accelerating = CAR_PEDAL_PRESSED;
	update_car_speed(INPUT_BRAKE_FLAG, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_actual_speed == 15900);
	assert(car.car_is_accelerating == CAR_PEDAL_PRESSED);
	assert(car.car_is_braking == CAR_PEDAL_RELEASED);
}

static void test_airborne_and_wheel_synchronization(void)
{
	reset_car();
	car.car_sumSurfRearWheels = 0;
	update_car_speed(INPUT_ACCELERATE_FLAG, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_rev_speed == 16768);
	assert(car.car_actual_speed == 16000);

	reset_car();
	car.car_rev_speed = 10000;
	update_car_speed(INPUT_NONE, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_rev_speed == 13000);
	assert(car.car_actual_speed == 13000);
	assert(car.car_engineLimiterTimer == 5);

	reset_car();
	car.car_rev_speed = 10880;
	update_car_speed(INPUT_NONE, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_rev_speed == 10880);
	assert(car.car_actual_speed == 10880);
}

static void test_speed_wrap_and_stop(void)
{
	reset_car();
	car.car_rev_speed = car.car_actual_speed = 50;
	update_car_speed(INPUT_BRAKE_FLAG, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_actual_speed == CAR_SPEED_STOPPED);

	reset_car();
	car.car_sumSurfRearWheels = 0;
	car.car_rev_speed = 65000;
	car.car_pseudoGravity = 1000;
	update_car_speed(INPUT_NONE, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_rev_speed == 62720);

	reset_car();
	car.car_sumSurfRearWheels = 0;
	car.car_rev_speed = 32760;
	car.car_pseudoGravity = 1000;
	update_car_speed(INPUT_NONE, PLAYER_CAR_INDEX, &car, &simd);
	assert(car.car_rev_speed == 33760);
}

int main(void)
{
	test_shift_precedence_and_limits();
	test_automatic_shift_contact_and_threshold();
	test_shift_completion_and_delay();
	test_pedals_and_frame_rates();
	test_overrev_preserves_pedal_state();
	test_airborne_and_wheel_synchronization();
	test_speed_wrap_and_stop();
	return 0;
}
