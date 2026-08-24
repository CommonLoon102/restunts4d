/* stolen from bsort.c, written by
 *   Robert Fowler,  Computer Science Department, U of Rochester
 *   February, 1989.
 *
 *   Parts are inherited from
 *   a test application for PLATINUM that runs a variation of
 *   a Batcher Odd-Even Merge sort on a random array of integers.
 *
 *   This code is adapted for PLATINUM from  program written by
 *   John Mellor-Crummey to exercise the PUTTs Toolkit at the U of R.
 *   This in turn was derived from code by Takahide Ohkami.
 */

 /*
oct 11 2014: adapted for restunts from here:
    http://wotug.org/parallel/parlib/p4/msort/heapsort.c

changes:
	- sorts by descending order
	- sorts a data array based on the heap array contents
 */

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
