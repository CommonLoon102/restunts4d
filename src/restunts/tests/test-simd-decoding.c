#include <assert.h>
#include <string.h>

#include "../c/externs.h"

#define TEST_PATTERN_MULTIPLIER 37U
#define TEST_PATTERN_INCREMENT 11U
#define TEST_DECODE_FILL_BYTE 165

static legacy_s8 expected_s8(const legacy_u8* source, legacy_u16 offset)
{
	return LEGACY_S8_FROM_BITS(source[offset]);
}

static legacy_s16 expected_s16(const legacy_u8* source, legacy_u16 offset)
{
	return LEGACY_READ_S16_LE(source + offset);
}

static legacy_u16 expected_u16(const legacy_u8* source, legacy_u16 offset)
{
	return LEGACY_READ_U16_LE(source + offset);
}

int main(void)
{
	legacy_u8 source[SIMD_RESOURCE_SIZE];
	struct SIMD decoded;
	legacy_u16 index;

	for (index = 0U; index < SIMD_RESOURCE_SIZE; index++)
		source[index] = (legacy_u8)(index * TEST_PATTERN_MULTIPLIER +
			TEST_PATTERN_INCREMENT);
	memset(&decoded, TEST_DECODE_FILL_BYTE, sizeof(decoded));

	assert(simd_decode(&decoded, source) == SIMD_RESOURCE_SIZE);
	assert(decoded.num_gears == expected_s8(source,
		SIMD_RESOURCE_NUM_GEARS_OFFSET));
	assert(decoded.simd_unk == expected_s8(source,
		SIMD_RESOURCE_UNKNOWN_OFFSET));
	assert(decoded.car_mass == expected_s16(source,
		SIMD_RESOURCE_CAR_MASS_OFFSET));
	assert(decoded.braking_eff == expected_s16(source,
		SIMD_RESOURCE_BRAKING_EFFICIENCY_OFFSET));
	assert(decoded.idle_rpm == expected_s16(source,
		SIMD_RESOURCE_IDLE_RPM_OFFSET));
	assert(decoded.downshift_rpm == expected_s16(source,
		SIMD_RESOURCE_DOWNSHIFT_RPM_OFFSET));
	assert(decoded.upshift_rpm == expected_s16(source,
		SIMD_RESOURCE_UPSHIFT_RPM_OFFSET));
	assert(decoded.max_rpm == expected_s16(source,
		SIMD_RESOURCE_MAX_RPM_OFFSET));
	for (index = 0U; index < SIMD_GEAR_RATIO_COUNT; index++) {
		assert(decoded.gear_ratios[index] ==
			expected_u16(source, (legacy_u16)(
				SIMD_RESOURCE_GEAR_RATIOS_OFFSET +
				index * SIMD_RESOURCE_WORD_SIZE)));
		assert(decoded.knob_points[index].px ==
			expected_s16(source, (legacy_u16)(
				SIMD_RESOURCE_KNOB_POINTS_OFFSET +
				index * SIMD_RESOURCE_POINT_SIZE)));
		assert(decoded.knob_points[index].py ==
			expected_s16(source, (legacy_u16)(
				SIMD_RESOURCE_KNOB_POINTS_OFFSET +
				SIMD_RESOURCE_POINT_Y_OFFSET +
				index * SIMD_RESOURCE_POINT_SIZE)));
	}
	assert(decoded.aero_resistance == expected_s16(source,
		SIMD_RESOURCE_AERO_RESISTANCE_OFFSET));
	assert(decoded.idle_torque == expected_s8(source,
		SIMD_RESOURCE_IDLE_TORQUE_OFFSET));
	for (index = 0U; index < SIMD_TORQUE_CURVE_SIZE; index++)
		assert(decoded.torque_curve[index] ==
			expected_s8(source, (legacy_u16)(
				SIMD_RESOURCE_TORQUE_CURVE_OFFSET + index)));
	assert(decoded.field_A3 == expected_s8(source,
		SIMD_RESOURCE_FIELD_A3_OFFSET));
	assert(decoded.grip == expected_s16(source,
		SIMD_RESOURCE_GRIP_OFFSET));
	for (index = 0U; index < SIMD_FIELD_A6_COUNT; index++)
		assert(decoded.field_A6[index] ==
			expected_s16(source, (legacy_u16)(
				SIMD_RESOURCE_FIELD_A6_OFFSET +
				index * SIMD_RESOURCE_WORD_SIZE)));
	assert(decoded.sliding == expected_s16(source,
		SIMD_RESOURCE_SLIDING_OFFSET));
	for (index = 0U; index < SIMD_SURFACE_GRIP_COUNT; index++)
		assert(decoded.surface_grip[index] ==
			expected_s16(source, (legacy_u16)(
				SIMD_RESOURCE_SURFACE_GRIP_OFFSET +
				index * SIMD_RESOURCE_WORD_SIZE)));
	for (index = 0U; index < SIMD_UNKNOWN3_SIZE; index++)
		assert(decoded.simd_unk3[index] ==
			expected_s8(source, (legacy_u16)(
				SIMD_RESOURCE_UNKNOWN3_OFFSET + index)));
	for (index = 0U; index < SIMD_COLLISION_POINT_COUNT; index++) {
		assert(decoded.collide_points[index].px ==
			expected_s16(source, (legacy_u16)(
				SIMD_RESOURCE_COLLISION_POINTS_OFFSET +
				index * SIMD_RESOURCE_POINT_SIZE)));
		assert(decoded.collide_points[index].py ==
			expected_s16(source, (legacy_u16)(
				SIMD_RESOURCE_COLLISION_POINTS_OFFSET +
				SIMD_RESOURCE_POINT_Y_OFFSET +
				index * SIMD_RESOURCE_POINT_SIZE)));
	}
	assert(decoded.car_height == expected_s16(source,
		SIMD_RESOURCE_CAR_HEIGHT_OFFSET));
	for (index = 0U; index < SIMD_WHEEL_COORDINATE_COUNT; index++) {
		assert(decoded.wheel_coords[index].x ==
			expected_s16(source, (legacy_u16)(
				SIMD_RESOURCE_WHEEL_COORDINATES_OFFSET +
				index * SIMD_RESOURCE_VECTOR_SIZE)));
		assert(decoded.wheel_coords[index].y ==
			expected_s16(source, (legacy_u16)(
				SIMD_RESOURCE_WHEEL_COORDINATES_OFFSET +
				SIMD_RESOURCE_VECTOR_Y_OFFSET +
				index * SIMD_RESOURCE_VECTOR_SIZE)));
		assert(decoded.wheel_coords[index].z ==
			expected_s16(source, (legacy_u16)(
				SIMD_RESOURCE_WHEEL_COORDINATES_OFFSET +
				SIMD_RESOURCE_VECTOR_Z_OFFSET +
				index * SIMD_RESOURCE_VECTOR_SIZE)));
	}
	for (index = 0U; index < SIMD_STEERING_DOT_COUNT; index++)
		assert(decoded.steeringdots[index] ==
			expected_s8(source, (legacy_u16)(
				SIMD_RESOURCE_STEERING_DOTS_OFFSET + index)));
	assert(decoded.spdcenter.px == expected_s16(source,
		SIMD_RESOURCE_SPEEDOMETER_CENTER_OFFSET));
	assert(decoded.spdcenter.py == expected_s16(source,
		SIMD_RESOURCE_SPEEDOMETER_CENTER_OFFSET +
		SIMD_RESOURCE_POINT_Y_OFFSET));
	assert(decoded.spdnumpoints == expected_s16(source,
		SIMD_RESOURCE_SPEEDOMETER_POINT_COUNT_OFFSET));
	for (index = 0U; index < SIMD_SPEEDOMETER_POINT_COUNT; index++)
		assert(decoded.spdpoints[index] ==
			expected_s8(source, (legacy_u16)(
				SIMD_RESOURCE_SPEEDOMETER_POINTS_OFFSET + index)));
	assert(decoded.revcenter.px == expected_s16(source,
		SIMD_RESOURCE_REV_COUNTER_CENTER_OFFSET));
	assert(decoded.revcenter.py == expected_s16(source,
		SIMD_RESOURCE_REV_COUNTER_CENTER_OFFSET +
		SIMD_RESOURCE_POINT_Y_OFFSET));
	assert(decoded.revnumpoints == expected_s16(source,
		SIMD_RESOURCE_REV_COUNTER_POINT_COUNT_OFFSET));
	for (index = 0U; index < SIMD_REV_COUNTER_POINT_COUNT; index++)
		assert(decoded.revpoints[index] ==
			expected_s8(source, (legacy_u16)(
				SIMD_RESOURCE_REV_COUNTER_POINTS_OFFSET + index)));
	assert(decoded.aerorestable == 0);
	return 0;
}
