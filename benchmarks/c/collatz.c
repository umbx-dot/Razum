#include <stdio.h>

int main(void) {
    long long best_start = 0;
    long long best_length = 0;
    for (long long start = 1; start < 3000000; start++) {
        long long value = start;
        long long length = 1;
        while (value != 1) {
            if ((value & 1) == 0) {
                value = value >> 1;
            } else {
                value = 3 * value + 1;
            }
            length++;
        }
        if (length > best_length) {
            best_length = length;
            best_start = start;
        }
    }
    printf("%lld\n%lld\n", best_start, best_length);
    return 0;
}
