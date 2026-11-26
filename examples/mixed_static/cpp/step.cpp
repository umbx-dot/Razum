#include <cstdint>
#include <cstring>

#include "parts.h"

extern "C" int32_t cpp_step(int32_t value) {
    return std::strcmp(razum_name(), "Razum") == 0 ? value + 2 : -1;
}
