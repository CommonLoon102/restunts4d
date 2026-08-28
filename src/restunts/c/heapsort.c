#include "legacy.h"

void heapsort_by_order(legacy_s16 count, legacy_s16* values, legacy_s16* order) {
	legacy_s16 gap;
	legacy_s16 counter;
	legacy_s16 index;
	legacy_s16 temp;

	gap = count / 2;
	while (gap > 0) {
		counter = gap;
		while (counter < count) {
			index = counter - gap;
			while (index >= 0 && values[index + gap] > values[index]) {
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
		gap /= 2;
	}
}
