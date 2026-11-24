#include "textutils.h"

#include <ctype.h>
#include <string.h>

long long count_vowels(const char *text) {
    long long count = 0;
    for (; *text != '\0'; text++) {
        if (strchr("aeiouAEIOU", *text) != NULL) {
            count++;
        }
    }
    return count;
}

void shout(char *text) {
    for (; *text != '\0'; text++) {
        *text = (char)toupper((unsigned char)*text);
    }
}
