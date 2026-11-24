#include <stdio.h>
#include <stdlib.h>

static void quicksort(long long *values, long long low, long long high) {
    while (low < high) {
        long long pivot = values[low + (high - low) / 2];
        long long left = low;
        long long right = high;
        while (left <= right) {
            while (values[left] < pivot) {
                left++;
            }
            while (values[right] > pivot) {
                right--;
            }
            if (left <= right) {
                long long swap = values[left];
                values[left] = values[right];
                values[right] = swap;
                left++;
                right--;
            }
        }
        if (right - low < high - left) {
            quicksort(values, low, right);
            low = left;
        } else {
            quicksort(values, left, high);
            high = right;
        }
    }
}

int main(void) {
    long long count = 5000000;
    long long *values = (long long *)calloc((size_t)count, sizeof *values);
    unsigned long long seed = 12345;
    for (long long i = 0; i < count; i++) {
        seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        values[i] = (long long)(seed >> 33);
    }
    quicksort(values, 0, count - 1);
    long long checksum = 0;
    for (long long i = 0; i < count; i += 1000) {
        checksum += values[i];
    }
    free(values);
    printf("%lld\n", checksum);
    return 0;
}
