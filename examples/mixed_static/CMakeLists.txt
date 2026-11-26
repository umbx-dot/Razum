cmake_minimum_required(VERSION 3.24)
project(mixed_static LANGUAGES C CXX)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR (WIN32 AND NOT MINGW))
    message(FATAL_ERROR "This fixture requires an x64 GNU toolchain")
endif()
if(WIN32)
    set(RUST_TARGET x86_64-pc-windows-gnu)
    set(GO_SYSTEM windows)
    set(NASM_FORMAT win64)
    set(OBJECT_SUFFIX .obj)
    set(SYSTEM_LIBRARIES ntdll userenv ws2_32 dbghelp winmm)
else()
    find_package(Threads REQUIRED)
    list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES gcc_s gcc_s_asneeded)
    set(RUST_TARGET x86_64-unknown-linux-gnu)
    set(GO_SYSTEM linux)
    set(NASM_FORMAT elf64)
    set(OBJECT_SUFFIX .o)
    set(SYSTEM_LIBRARIES Threads::Threads dl rt util m)
endif()
set(RAZUM_OBJECT "${RAZUM_ARTIFACT_DIR}/parts${OBJECT_SUFFIX}")
if(NOT EXISTS "${RAZUM_OBJECT}" OR NOT EXISTS "${RAZUM_ARTIFACT_DIR}/parts.h")
    message(FATAL_ERROR "Build through rz so the Razum object and header are available")
endif()
if(DEFINED RAZUM_COMPILER AND NOT EXISTS "${RAZUM_COMPILER}")
    message(FATAL_ERROR "The Razum compiler path is invalid")
endif()

find_program(RUSTC rustc REQUIRED)
find_program(GO go REQUIRED)
find_program(NASM nasm REQUIRED)
set(RUST_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/rust.a")
set(GO_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/go.a")
set(ASM_OBJECT "${CMAKE_CURRENT_BINARY_DIR}/step${OBJECT_SUFFIX}")

add_custom_command(OUTPUT "${RUST_ARCHIVE}"
    COMMAND "${RUSTC}" --target "${RUST_TARGET}" --edition 2021 --crate-type staticlib -O
            "${CMAKE_CURRENT_SOURCE_DIR}/rust/step.rs" -o "${RUST_ARCHIVE}"
    DEPENDS rust/step.rs "${RUSTC}" VERBATIM)
add_custom_command(OUTPUT "${GO_ARCHIVE}"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/go.h"
    COMMAND "${CMAKE_COMMAND}" -E env "CGO_ENABLED=1" "GOOS=${GO_SYSTEM}" "GOARCH=amd64" "CC=${CMAKE_C_COMPILER}"
            "${GO}" build -trimpath -buildmode=c-archive -o "${GO_ARCHIVE}" main.go
    DEPENDS go/main.go "${GO}" VERBATIM
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/go")
add_custom_command(OUTPUT "${ASM_OBJECT}"
    COMMAND "${NASM}" -f "${NASM_FORMAT}" "${CMAKE_CURRENT_SOURCE_DIR}/asm/step.asm" -o "${ASM_OBJECT}"
    DEPENDS asm/step.asm "${NASM}" VERBATIM)

set_source_files_properties("${ASM_OBJECT}" "${RAZUM_OBJECT}" PROPERTIES EXTERNAL_OBJECT TRUE)
add_custom_target(archives DEPENDS "${RUST_ARCHIVE}" "${GO_ARCHIVE}")
add_executable(integration main.c cpp/step.cpp "${ASM_OBJECT}" "${RAZUM_OBJECT}")
add_dependencies(integration archives)
target_include_directories(integration PRIVATE "${RAZUM_ARTIFACT_DIR}")
target_compile_features(integration PRIVATE c_std_17 cxx_std_17)
target_compile_options(integration PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_link_options(integration PRIVATE -static "$<$<CONFIG:Release>:-Wl,--strip-debug>")

option(RZ_WITH_LUA "Include the Lua runtime and embedded script" ON)
if(RZ_WITH_LUA)
    include(FetchContent)
    FetchContent_Declare(lua
        URL https://www.lua.org/ftp/lua-5.4.9.tar.gz
        URL_HASH SHA256=2335b6c582a52654f94612bf10d2f4672805d05329aa6568b1d8cd9e5c6fb8e6
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(lua)
    file(GLOB lua_sources "${lua_SOURCE_DIR}/src/*.c")
    list(FILTER lua_sources EXCLUDE REGEX "/lua(c)?\\.c$")
    add_library(lua_runtime STATIC ${lua_sources})
    target_include_directories(lua_runtime PUBLIC "${lua_SOURCE_DIR}/src")
    target_sources(integration PRIVATE lua/step.c)
    target_compile_definitions(integration PRIVATE RZ_WITH_LUA=1)
    target_link_libraries(integration PRIVATE lua_runtime)
endif()
target_link_libraries(integration PRIVATE "${RUST_ARCHIVE}" "${GO_ARCHIVE}" ${SYSTEM_LIBRARIES})
