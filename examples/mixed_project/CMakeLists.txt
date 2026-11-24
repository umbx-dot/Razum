cmake_minimum_required(VERSION 3.20)
project(mixed_project C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_CXX_STANDARD 17)

find_package(Razum REQUIRED)

add_library(textutils SHARED c/textutils.c)
target_compile_definitions(textutils PRIVATE TEXTUTILS_BUILD)
target_include_directories(textutils PUBLIC c)
set_target_properties(textutils PROPERTIES PREFIX "")

add_razum_library(fastmath razum/fastmath.rz)

add_executable(app cpp/main.cpp)
target_link_libraries(app PRIVATE fastmath textutils)

add_razum_executable(report razum/report.rz DEPENDS textutils fastmath)
