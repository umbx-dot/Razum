#include <stdio.h>

#include "mathlib.h"

int main(void) {
    printf("%lld\n", (long long)add(40, 2));
    printf("%d\n", scale(-7, 6));
    int values[4] = {1, 2, 3, 4};
    printf("%lld\n", (long long)sum(values, 4));
    Point p = {1, 2};
    move_point(&p, 10, 20);
    printf("%d\n", p.x + p.y);
    long long counter = 41;
    increment((int64_t *)&counter);
    printf("%lld\n", counter);
    printf("%.1f\n", average(1.0, 2.0));
    printf("%s\n", describe());
    printf("%s\n", is_even(10) ? "true" : "false");
    printf("%s\n", is_even(7) ? "true" : "false");
    printf("%lld\n", (long long)call_count());
    return 0;
}
