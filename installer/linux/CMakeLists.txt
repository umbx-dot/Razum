find_program(NASM_EXECUTABLE nasm)
find_file(NASM_LICENSE LICENSE PATHS /usr/share/licenses/nasm /usr/share/doc/nasm NO_DEFAULT_PATH)
find_program(ZIP_EXECUTABLE zip)
find_program(PYTHON_EXECUTABLE python3)

set(EXTENSION_DIRECTORY ${PROJECT_SOURCE_DIR}/editors/vscode)
file(READ ${EXTENSION_DIRECTORY}/package.json EXTENSION_MANIFEST)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${EXTENSION_DIRECTORY}/package.json)
string(JSON EXTENSION_VERSION GET "${EXTENSION_MANIFEST}" version)
string(JSON EXTENSION_NAME GET "${EXTENSION_MANIFEST}" name)
string(JSON EXTENSION_PUBLISHER GET "${EXTENSION_MANIFEST}" publisher)
set(SETUP ${CMAKE_BINARY_DIR}/razum-setup.sh)
set(RAZUM_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)
set(GUI_NOTICES ${PROJECT_SOURCE_DIR}/docs/linux-gui-notices.md)

set(VSIX_ARGUMENTS "")
set(VSIX_DEPENDENCY "")
if(ZIP_EXECUTABLE AND PYTHON_EXECUTABLE)
    set(VSIX ${CMAKE_BINARY_DIR}/razum-${EXTENSION_VERSION}.vsix)
    file(GLOB_RECURSE EXTENSION_FILES CONFIGURE_DEPENDS ${EXTENSION_DIRECTORY}/*)
    add_custom_command(
        OUTPUT ${VSIX}
        COMMAND bash ${EXTENSION_DIRECTORY}/package.sh $<TARGET_FILE:rzc> ${CMAKE_BINARY_DIR}
        DEPENDS rzc ${EXTENSION_FILES} ${RAZUM_LICENSE} ${GUI_NOTICES}
        VERBATIM
    )
    set(VSIX_ARGUMENTS ${VSIX} ${EXTENSION_PUBLISHER}.${EXTENSION_NAME})
    set(VSIX_DEPENDENCY ${VSIX})
else()
    message(STATUS "zip or python3 not found: razum-setup.sh will not contain the VS Code extension")
endif()

set(BUNDLED_NASM "")
set(BUNDLED_NASM_LICENSE "")
if(NASM_EXECUTABLE AND NASM_LICENSE)
    set(BUNDLED_NASM "${NASM_EXECUTABLE}")
    set(BUNDLED_NASM_LICENSE "${NASM_LICENSE}")
endif()
if(NOT VSIX_ARGUMENTS)
    set(VSIX_ARGUMENTS "" "")
endif()

add_custom_command(
    OUTPUT ${SETUP}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/make_setup.sh ${SETUP} $<TARGET_FILE:rzc> $<TARGET_FILE:rz> ${PROJECT_SOURCE_DIR}/cmake/package/RazumConfig.cmake
            "${BUNDLED_NASM}" "${BUNDLED_NASM_LICENSE}" ${VSIX_ARGUMENTS}
    DEPENDS rzc rz ${VSIX_DEPENDENCY} ${BUNDLED_NASM} ${BUNDLED_NASM_LICENSE} ${PROJECT_SOURCE_DIR}/rz/vendor/tomlc17/LICENSE
            ${CMAKE_CURRENT_SOURCE_DIR}/setup.sh ${CMAKE_CURRENT_SOURCE_DIR}/make_setup.sh
            ${PROJECT_SOURCE_DIR}/cmake/package/RazumConfig.cmake ${RAZUM_LICENSE} ${GUI_NOTICES}
    VERBATIM
)
add_custom_target(razum-setup DEPENDS ${SETUP})
