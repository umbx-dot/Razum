#include <stdint.h>

#include "lauxlib.h"

int32_t lua_step(int32_t value) {
    lua_State *state = luaL_newstate();
    if (state == NULL) return -1;
    static const char script[] = "local value = ...\nreturn value + 7";
    int32_t result = -1;
    if (luaL_loadbuffer(state, script, sizeof script - 1, "integration") == LUA_OK) {
        lua_pushinteger(state, value);
        if (lua_pcall(state, 1, 1, 0) == LUA_OK && lua_isinteger(state, -1)) {
            result = (int32_t)lua_tointeger(state, -1);
        }
    }
    lua_close(state);
    return result;
}
