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
	reader->offset = LEGACY_U16_WRAP_ADD(reader->offset, 2U);
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

legacy_u16 simd_decode(struct SIMD* destination,
	const legacy_u8 far* source)
{
	struct SIMD_READER reader;
	legacy_u16 index;

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
	for (index = 0U; index < 7U; index++)
		destination->gear_ratios[index] = simd_read_u16(&reader);
	for (index = 0U; index < 7U; index++)
		simd_read_point(&reader, &destination->knob_points[index]);
	destination->aero_resistance = simd_read_s16(&reader);
	destination->idle_torque = simd_read_s8(&reader);
	for (index = 0U; index < 104U; index++)
		destination->torque_curve[index] = simd_read_s8(&reader);
	destination->field_A3 = simd_read_s8(&reader);
	destination->grip = simd_read_s16(&reader);
	for (index = 0U; index < 7U; index++)
		destination->field_A6[index] = simd_read_s16(&reader);
	destination->sliding = simd_read_s16(&reader);
	for (index = 0U; index < 4U; index++)
		destination->surface_grip[index] = simd_read_s16(&reader);
	for (index = 0U; index < 10U; index++)
		destination->simd_unk3[index] = simd_read_s8(&reader);
	for (index = 0U; index < 2U; index++)
		simd_read_point(&reader, &destination->collide_points[index]);
	destination->car_height = simd_read_s16(&reader);
	for (index = 0U; index < 4U; index++)
		simd_read_vector(&reader, &destination->wheel_coords[index]);
	for (index = 0U; index < 62U; index++)
		destination->steeringdots[index] = simd_read_s8(&reader);
	simd_read_point(&reader, &destination->spdcenter);
	destination->spdnumpoints = simd_read_s16(&reader);
	for (index = 0U; index < 208U; index++)
		destination->spdpoints[index] = simd_read_s8(&reader);
	simd_read_point(&reader, &destination->revcenter);
	destination->revnumpoints = simd_read_s16(&reader);
	for (index = 0U; index < 256U; index++)
		destination->revpoints[index] = simd_read_s8(&reader);

	/* This is a runtime lookup table pointer, not part of the resource. */
	destination->aerorestable = 0;
	return reader.offset;
}
