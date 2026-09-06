#include "legacy.h"

#define HEAPSORT_GAP_DIVISOR 2
#define HEAPSORT_NO_GAP 0
#define HEAPSORT_FIRST_INDEX 0

void heapsort_by_order(legacy_s16 count, legacy_s16* values, legacy_s16* order) {
	legacy_s16 gap;
	legacy_s16 counter;
	legacy_s16 index;
	legacy_s16 temp;

	gap = LEGACY_S16_DIV_OR_ZERO(count, HEAPSORT_GAP_DIVISOR);
	while (gap > HEAPSORT_NO_GAP) {
		counter = gap;
		while (counter < count) {
			index = counter - gap;
			while (index >= HEAPSORT_FIRST_INDEX &&
				values[index + gap] > values[index]) {
				temp = values[index];
				values[index] = values[index + gap];
				values[index + gap] = temp;

				temp = order[index];
				order[index] = order[index + gap];
				order[index + gap] = temp;
				index -= gap;
			}
			counter++;
		}
		gap = LEGACY_S16_DIV_OR_ZERO(gap, HEAPSORT_GAP_DIVISOR);
	}
}
