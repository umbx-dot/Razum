#include <stdio.h>
#include <stdlib.h>

int main(void) {
    long long limit = 50000000;
    unsigned char *composite = (unsigned char *)calloc((size_t)limit + 1, 1);
    long long count = 0;
    for (long long i = 2; i <= limit; i++) {
        if (composite[i] == 0) {
            count++;
            for (long long multiple = i * i; multiple <= limit; multiple += i) {
                composite[multiple] = 1;
            }
        }
    }
    free(composite);
    printf("%lld\n", count);
    return 0;
}
