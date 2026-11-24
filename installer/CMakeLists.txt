set(INSTALLER_WARNINGS $<IF:$<C_COMPILER_ID:MSVC>,/W4;/WX,-Wall;-Wextra;-Wpedantic;-Werror>)

add_executable(test_path_list ${PROJECT_SOURCE_DIR}/tests/installer/test_path_list.c src/path_list.c)
target_include_directories(test_path_list PRIVATE src)
target_compile_options(test_path_list PRIVATE ${INSTALLER_WARNINGS})

find_program(NASM_EXECUTABLE nasm HINTS "$ENV{ProgramFiles}/NASM")
if(NOT NASM_EXECUTABLE)
    message(STATUS "nasm not found: the razum-setup target is not available")
    return()
endif()
get_filename_component(NASM_DIRECTORY "${NASM_EXECUTABLE}" DIRECTORY)
find_file(NASM_LICENSE NAMES LICENSE LICENSE.txt
          HINTS "${NASM_DIRECTORY}" "${NASM_DIRECTORY}/../share/licenses/nasm" NO_DEFAULT_PATH)
if(NOT NASM_LICENSE)
    message(STATUS "NASM's LICENSE file was not found next to ${NASM_EXECUTABLE}: the razum-setup target is not available")
    return()
endif()

enable_language(RC)

set(EXTENSION_DIRECTORY ${PROJECT_SOURCE_DIR}/editors/vscode)
file(READ ${EXTENSION_DIRECTORY}/package.json EXTENSION_MANIFEST)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${EXTENSION_DIRECTORY}/package.json)
string(JSON EXTENSION_VERSION GET "${EXTENSION_MANIFEST}" version)
set(VSIX ${CMAKE_BINARY_DIR}/razum-${EXTENSION_VERSION}.vsix)
set(RAZUM_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)
set(GUI_NOTICES ${PROJECT_SOURCE_DIR}/docs/linux-gui-notices.md)
file(GLOB_RECURSE EXTENSION_FILES CONFIGURE_DEPENDS ${EXTENSION_DIRECTORY}/*)
add_custom_command(
    OUTPUT ${VSIX}
    COMMAND powershell.exe -NoProfile -ExecutionPolicy Bypass -File ${EXTENSION_DIRECTORY}/package.ps1
            -Compiler $<TARGET_FILE:rzc> -OutputDirectory ${CMAKE_BINARY_DIR}
    DEPENDS rzc ${EXTENSION_FILES} ${RAZUM_LICENSE} ${GUI_NOTICES}
    VERBATIM
)

set(FILE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/rz.ico)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/setup.rc CONTENT
"#include \"${CMAKE_CURRENT_SOURCE_DIR}/src/resource_ids.h\"
RESOURCE_APP_ICON ICON \"${FILE_ICON}\"
RESOURCE_RZC RCDATA \"$<TARGET_FILE:rzc>\"
RESOURCE_NASM RCDATA \"${NASM_EXECUTABLE}\"
RESOURCE_NASM_LICENSE RCDATA \"${NASM_LICENSE}\"
RESOURCE_VSIX RCDATA \"${VSIX}\"
RESOURCE_FILE_ICON RCDATA \"${FILE_ICON}\"
RESOURCE_ICON_LICENSE RCDATA \"${CMAKE_CURRENT_SOURCE_DIR}/rz-icon-LICENSE.txt\"
RESOURCE_RAZUM_LICENSE RCDATA \"${RAZUM_LICENSE}\"
RESOURCE_GUI_NOTICES RCDATA \"${GUI_NOTICES}\"
RESOURCE_CMAKE_PACKAGE RCDATA \"${PROJECT_SOURCE_DIR}/cmake/package/RazumConfig.cmake\"
")
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/setup.rc PROPERTIES
    OBJECT_DEPENDS "${VSIX};${FILE_ICON};${CMAKE_CURRENT_SOURCE_DIR}/rz-icon-LICENSE.txt;${PROJECT_SOURCE_DIR}/cmake/package/RazumConfig.cmake;${NASM_EXECUTABLE};${NASM_LICENSE};${RAZUM_LICENSE};${GUI_NOTICES}")

file(GLOB SETUP_SOURCES CONFIGURE_DEPENDS src/*.c)
add_executable(razum-setup WIN32 EXCLUDE_FROM_ALL ${SETUP_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/setup.rc)
target_include_directories(razum-setup PRIVATE src)
target_compile_definitions(razum-setup PRIVATE UNICODE _UNICODE $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS> $<$<COMPILE_LANGUAGE:C>:RAZUM_VERSION=L"${PROJECT_VERSION}">)
target_compile_options(razum-setup PRIVATE $<$<COMPILE_LANGUAGE:C>:${INSTALLER_WARNINGS}>)
target_link_libraries(razum-setup PRIVATE shell32 advapi32 user32)
add_dependencies(razum-setup rzc)
if(MINGW)
    target_link_options(razum-setup PRIVATE -municode)
endif()
