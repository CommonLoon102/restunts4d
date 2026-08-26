void heapsort_by_order(int count, int* values, int* order) {
	int gap;
	int counter;
	int index;
	int temp;

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