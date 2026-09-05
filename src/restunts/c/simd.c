#include "externs.h"

struct SIMD_READER {
	const legacy_u8 far* source;
	legacy_u16 offset;
};

static legacy_u8 simd_read_u8(struct SIMD_READER* reader)
{
	legacy_u8 value;

	value = reader->source[reader->offset];
	reader->offset = LEGACY_U16_WRAP_ADD(reader->offset, 1U);
	return value;
}

static legacy_s8 simd_read_s8(struct SIMD_READER* reader)
{
	legacy_u8 value;

	value = simd_read_u8(reader);
	return LEGACY_S8_FROM_BITS(value);
}

static legacy_u16 simd_read_u16(struct SIMD_READER* reader)
{
	legacy_u16 value;

	value = LEGACY_READ_U16_LE(reader->source + reader->offset);
	reader->offset = LEGACY_U16_WRAP_ADD(reader->offset,
		LEGACY_WORD_BYTES);
	return value;
}

static legacy_s16 simd_read_s16(struct SIMD_READER* reader)
{
	legacy_u16 value;

	value = simd_read_u16(reader);
	return LEGACY_S16_FROM_BITS(value);
}

static void simd_read_point(struct SIMD_READER* reader,
	struct POINT2D* point)
{
	point->px = simd_read_s16(reader);
	point->py = simd_read_s16(reader);
}

static void simd_read_vector(struct SIMD_READER* reader,
	struct VECTOR* vector)
{
	vector->x = simd_read_s16(reader);
	vector->y = simd_read_s16(reader);
	vector->z = simd_read_s16(reader);
}

static void simd_read_s8_array(struct SIMD_READER* reader,
	legacy_s8* values, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++)
		values[index] = simd_read_s8(reader);
}

static void simd_read_u16_array(struct SIMD_READER* reader,
	legacy_u16* values, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++)
		values[index] = simd_read_u16(reader);
}

static void simd_read_s16_array(struct SIMD_READER* reader,
	legacy_s16* values, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++)
		values[index] = simd_read_s16(reader);
}

static void simd_read_point_array(struct SIMD_READER* reader,
	struct POINT2D* points, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++)
		simd_read_point(reader, &points[index]);
}

static void simd_read_vector_array(struct SIMD_READER* reader,
	struct VECTOR* vectors, legacy_u16 count)
{
	legacy_u16 index;

	for (index = 0U; index < count; index++)
		simd_read_vector(reader, &vectors[index]);
}

legacy_u16 simd_decode(struct SIMD* destination,
	const legacy_u8 far* source)
{
	struct SIMD_READER reader;

	reader.source = source;
	reader.offset = 0U;

	destination->num_gears = simd_read_s8(&reader);
	destination->simd_unk = simd_read_s8(&reader);
	destination->car_mass = simd_read_s16(&reader);
	destination->braking_eff = simd_read_s16(&reader);
	destination->idle_rpm = simd_read_s16(&reader);
	destination->downshift_rpm = simd_read_s16(&reader);
	destination->upshift_rpm = simd_read_s16(&reader);
	destination->max_rpm = simd_read_s16(&reader);
	simd_read_u16_array(&reader, destination->gear_ratios,
		SIMD_GEAR_RATIO_COUNT);
	simd_read_point_array(&reader, destination->knob_points,
		SIMD_KNOB_POINT_COUNT);
	destination->aero_resistance = simd_read_s16(&reader);
	destination->idle_torque = simd_read_s8(&reader);
	simd_read_s8_array(&reader, destination->torque_curve,
		SIMD_TORQUE_CURVE_SIZE);
	destination->field_A3 = simd_read_s8(&reader);
	destination->grip = simd_read_s16(&reader);
	simd_read_s16_array(&reader, destination->field_A6,
		SIMD_FIELD_A6_COUNT);
	destination->sliding = simd_read_s16(&reader);
	simd_read_s16_array(&reader, destination->surface_grip,
		SIMD_SURFACE_GRIP_COUNT);
	simd_read_s8_array(&reader, destination->simd_unk3,
		SIMD_UNKNOWN3_SIZE);
	simd_read_point_array(&reader, destination->collide_points,
		SIMD_COLLISION_POINT_COUNT);
	destination->car_height = simd_read_s16(&reader);
	simd_read_vector_array(&reader, destination->wheel_coords,
		SIMD_WHEEL_COORDINATE_COUNT);
	simd_read_s8_array(&reader, destination->steeringdots,
		SIMD_STEERING_DOT_COUNT);
	simd_read_point(&reader, &destination->spdcenter);
	destination->spdnumpoints = simd_read_s16(&reader);
	simd_read_s8_array(&reader, destination->spdpoints,
		SIMD_SPEEDOMETER_POINT_COUNT);
	simd_read_point(&reader, &destination->revcenter);
	destination->revnumpoints = simd_read_s16(&reader);
	simd_read_s8_array(&reader, destination->revpoints,
		SIMD_REV_COUNTER_POINT_COUNT);

	/* This is a runtime lookup table pointer, not part of the resource. */
	destination->aerorestable = 0;
	return reader.offset;
}
