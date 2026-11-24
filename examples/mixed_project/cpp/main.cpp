#include <cstdio>
#include <string>
#include <vector>

#include "fastmath.h"
#include "textutils.h"

int main() {
    std::vector<int64_t> values = {4, 8, 15, 16, 23, 42};
    Stats result{};
    stats(values.data(), static_cast<int64_t>(values.size()), &result);
    std::printf("C++ -> razum: fibonacci(50) = %lld\n", static_cast<long long>(fibonacci(50)));
    std::printf("C++ -> razum: min %lld, max %lld, average %.2f\n", static_cast<long long>(result.minimum),
                static_cast<long long>(result.maximum), result.average);
    std::string text = "mixed languages";
    shout(text.data());
    std::printf("C++ -> C: %s has %lld vowels\n", text.c_str(), count_vowels(text.c_str()));
    return 0;
}
