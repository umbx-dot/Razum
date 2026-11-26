#include <inttypes.h>
#include <stdio.h>

#include "parts.h"

int32_t cpp_step(int32_t value);
int32_t rust_step(int32_t value);
int32_t go_step(int32_t value);
int32_t asm_step(int32_t value);
#ifdef RZ_WITH_LUA
int32_t lua_step(int32_t value);
#endif

#ifndef _WIN32
__attribute__((ms_abi))
#endif
static int32_t add(int32_t left, int32_t right) {
    return left + right;
}

int main(void) {
    int32_t c = 10;
    int32_t cpp = cpp_step(c);
    int32_t rust = rust_step(cpp);
    int32_t go = go_step(rust);
    uint8_t marker = 0;
    int32_t razum = razum_step(go, &marker);
    int32_t assembly = asm_step(razum);
    if (cpp != 12 || rust != 15 || go != 19 || razum != 24 || marker != 19 || assembly != 30) {
        return 1;
    }
    int32_t values[] = {21, 22};
    double abi = razum_abi(-1, 2.25, values, 2, 3.5f, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    if (abi != 141.75 || shift != 5 || razum_invoke(add, 37) != 42) {
        return 3;
    }
    printf("C=%" PRId32 " C++=%" PRId32 " Rust=%" PRId32 " Go=%" PRId32 " Razum=%" PRId32 " NASM=%" PRId32,
           c, cpp, rust, go, razum, assembly);
#ifdef RZ_WITH_LUA
    int32_t lua = lua_step(assembly);
    if (lua != 37) return 2;
    printf(" Lua=%" PRId32, lua);
#endif
    puts("");
    return 0;
}
