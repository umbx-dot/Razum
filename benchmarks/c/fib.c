#include <stdio.h>
#include <stdlib.h>

static long long fib(long long n) {
    if (n < 2) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char **argv) {
    long long n = argc > 1 ? strtoll(argv[1], NULL, 10) : 38;
    printf("%lld\n", fib(n));
    return 0;
}
