cmake_minimum_required(VERSION 3.20)
project(razum VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(WIN32)
    set(RAZUM_HOST windows)
    enable_language(RC)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(RAZUM_HOST linux)
else()
    message(FATAL_ERROR "rzc builds on Windows and Linux; ${CMAKE_SYSTEM_NAME} is not supported yet")
endif()

file(GLOB_RECURSE COMPILER_SOURCES CONFIGURE_DEPENDS src/*.c)
list(FILTER COMPILER_SOURCES EXCLUDE REGEX "/src/platform/")
file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS src/platform/${RAZUM_HOST}/*.c)
file(GLOB_RECURSE LIBRARY_FILES CONFIGURE_DEPENDS libraries/*.rz libraries/*.asm)

set(EMBEDDED_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/embedded_files.c)
add_custom_command(
    OUTPUT ${EMBEDDED_SOURCE}
    COMMAND ${CMAKE_COMMAND} "-DLIBRARIES=${CMAKE_CURRENT_SOURCE_DIR}/libraries" "-DOUTPUT=${EMBEDDED_SOURCE}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_libraries.cmake
    DEPENDS ${LIBRARY_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_libraries.cmake
    VERBATIM
)

add_executable(rzc ${COMPILER_SOURCES} ${PLATFORM_SOURCES} ${EMBEDDED_SOURCE})
target_include_directories(rzc PRIVATE src)
if(WIN32)
    target_sources(rzc PRIVATE src/platform/windows/rzc.rc)
    set_property(SOURCE src/platform/windows/rzc.rc APPEND PROPERTY
                 OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows/rzc.manifest")
    if(MSVC)
        target_link_options(rzc PRIVATE /MANIFEST:NO)
    endif()
endif()

if(MSVC)
    target_compile_options(rzc PRIVATE /W4 /WX)
    target_compile_definitions(rzc PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(rzc PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

install(TARGETS rzc RUNTIME DESTINATION bin)

if(WIN32)
    add_subdirectory(installer)
else()
    add_subdirectory(installer/linux)
endif()
