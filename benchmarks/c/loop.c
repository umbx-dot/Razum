#include <stdio.h>

int main(void) {
    long long total = 0;
    for (long long i = 0; i < 400000000; i++) {
        total += (i * i) ^ (i >> 3);
    }
    printf("%lld\n", total);
    return 0;
}
