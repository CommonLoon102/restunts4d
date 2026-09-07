#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../c/externs.h"
#include "../c/math.h"
#include "../c/fatal.h"

#undef printf

extern void rectlist_add_rect(legacy_s8 *rectangle_count, struct RECTANGLE *rectangles,
							  struct RECTANGLE *rect);

static uint64_t trace_hash = UINT64_C(1469598103934665603);

void fatal_error(const legacy_s8 *format, ...)
{
	(void)format;
	assert(0);
}

static void trace_word(legacy_u16 value)
{
	trace_hash = (trace_hash ^ (value & 255U)) * UINT64_C(1099511628211);
	trace_hash = (trace_hash ^ (value >> 8)) * UINT64_C(1099511628211);
}

static void test_polar_boundaries(void)
{
	static const legacy_s16 coordinates[] = {-32767, -1025, -1024, -129, -128, -1,	 0,
											 1,		 128,	129,   1024, 1025, 32767};
	unsigned int x, y;

	assert(polarAngle(0, 0) == 0);
	assert(polarAngle(1, 0) == ANGLE_QUARTER_TURN);
	assert(polarAngle(0, -1) == ANGLE_HALF_TURN);
	assert(polarAngle(-1, 0) == -ANGLE_QUARTER_TURN);
	assert(polarAngle(-32768, -32768) == -384);
	for (x = 0; x < sizeof(coordinates) / sizeof(coordinates[0]); x++) {
		for (y = 0; y < sizeof(coordinates) / sizeof(coordinates[0]); y++) {
			trace_word(polarAngle(coordinates[x], coordinates[y]));
		}
	}
}

static void test_rectangle_splits(void)
{
	static const struct RECTANGLE inputs[] = {{0, 10, 0, 10},  {2, 8, 2, 8},	{-2, 12, -2, 12},
											  {2, 8, -4, 14},  {-4, 14, 2, 8},	{5, 15, -5, 5},
											  {5, 15, 5, 15},  {0, 10, 10, 20}, {10, 20, 0, 10},
											  {-10, 0, 0, 10}, {0, 10, -10, 0}, {30, 40, 30, 40}};
	struct RECTANGLE rectangles[64];
	struct RECTANGLE next;
	legacy_s8 count;
	unsigned int first, second, third, index;

	video_x_alignment = 1;
	for (first = 0; first < 12U; first++) {
		for (second = 0; second < 12U; second++) {
			for (third = 0; third < 12U; third++) {
				count = 0;
				next = inputs[first];
				rectlist_add_rect(&count, rectangles, &next);
				next = inputs[second];
				rectlist_add_rect(&count, rectangles, &next);
				next = inputs[third];
				rectlist_add_rect(&count, rectangles, &next);
				trace_word(count);
				for (index = 0; index < (unsigned int)count; index++) {
					trace_word(rectangles[index].left);
					trace_word(rectangles[index].right);
					trace_word(rectangles[index].top);
					trace_word(rectangles[index].bottom);
				}
			}
		}
	}
}

int main(void)
{
	test_polar_boundaries();
	test_rectangle_splits();
	/* Captured before extraction: quadrant boundaries and 1,728 ordered rectangle triples. */
	assert(trace_hash == UINT64_C(0x998e7d0d29ad404a));
	puts("Polar boundaries and rectangle splitting snapshots passed.");
	return 0;
}
