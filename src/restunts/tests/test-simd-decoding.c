#include <assert.h>
#include <string.h>

#include "../c/externs.h"

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
		source[index] = (legacy_u8)(index * 37U + 11U);
	memset(&decoded, 0xA5, sizeof(decoded));

	assert(simd_decode(&decoded, source) == SIMD_RESOURCE_SIZE);
	assert(decoded.num_gears == expected_s8(source, 0U));
	assert(decoded.simd_unk == expected_s8(source, 1U));
	assert(decoded.car_mass == expected_s16(source, 2U));
	assert(decoded.braking_eff == expected_s16(source, 4U));
	assert(decoded.idle_rpm == expected_s16(source, 6U));
	assert(decoded.downshift_rpm == expected_s16(source, 8U));
	assert(decoded.upshift_rpm == expected_s16(source, 10U));
	assert(decoded.max_rpm == expected_s16(source, 12U));
	for (index = 0U; index < 7U; index++) {
		assert(decoded.gear_ratios[index] ==
			expected_u16(source, (legacy_u16)(14U + index * 2U)));
		assert(decoded.knob_points[index].px ==
			expected_s16(source, (legacy_u16)(28U + index * 4U)));
		assert(decoded.knob_points[index].py ==
			expected_s16(source, (legacy_u16)(30U + index * 4U)));
	}
	assert(decoded.aero_resistance == expected_s16(source, 56U));
	assert(decoded.idle_torque == expected_s8(source, 58U));
	for (index = 0U; index < 104U; index++)
		assert(decoded.torque_curve[index] ==
			expected_s8(source, (legacy_u16)(59U + index)));
	assert(decoded.field_A3 == expected_s8(source, 163U));
	assert(decoded.grip == expected_s16(source, 164U));
	for (index = 0U; index < 7U; index++)
		assert(decoded.field_A6[index] ==
			expected_s16(source, (legacy_u16)(166U + index * 2U)));
	assert(decoded.sliding == expected_s16(source, 180U));
	for (index = 0U; index < 4U; index++)
		assert(decoded.surface_grip[index] ==
			expected_s16(source, (legacy_u16)(182U + index * 2U)));
	for (index = 0U; index < 10U; index++)
		assert(decoded.simd_unk3[index] ==
			expected_s8(source, (legacy_u16)(190U + index)));
	for (index = 0U; index < 2U; index++) {
		assert(decoded.collide_points[index].px ==
			expected_s16(source, (legacy_u16)(200U + index * 4U)));
		assert(decoded.collide_points[index].py ==
			expected_s16(source, (legacy_u16)(202U + index * 4U)));
	}
	assert(decoded.car_height == expected_s16(source, 208U));
	for (index = 0U; index < 4U; index++) {
		assert(decoded.wheel_coords[index].x ==
			expected_s16(source, (legacy_u16)(210U + index * 6U)));
		assert(decoded.wheel_coords[index].y ==
			expected_s16(source, (legacy_u16)(212U + index * 6U)));
		assert(decoded.wheel_coords[index].z ==
			expected_s16(source, (legacy_u16)(214U + index * 6U)));
	}
	for (index = 0U; index < 62U; index++)
		assert(decoded.steeringdots[index] ==
			expected_s8(source, (legacy_u16)(234U + index)));
	assert(decoded.spdcenter.px == expected_s16(source, 296U));
	assert(decoded.spdcenter.py == expected_s16(source, 298U));
	assert(decoded.spdnumpoints == expected_s16(source, 300U));
	for (index = 0U; index < 208U; index++)
		assert(decoded.spdpoints[index] ==
			expected_s8(source, (legacy_u16)(302U + index)));
	assert(decoded.revcenter.px == expected_s16(source, 510U));
	assert(decoded.revcenter.py == expected_s16(source, 512U));
	assert(decoded.revnumpoints == expected_s16(source, 514U));
	for (index = 0U; index < 256U; index++)
		assert(decoded.revpoints[index] ==
			expected_s8(source, (legacy_u16)(516U + index)));
	assert(decoded.aerorestable == 0);
	return 0;
}
