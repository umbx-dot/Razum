find_program(RAZUM_COMPILER rzc HINTS "${CMAKE_CURRENT_LIST_DIR}/../bin")
if(NOT RAZUM_COMPILER)
    set(Razum_FOUND FALSE)
    set(Razum_NOT_FOUND_MESSAGE "rzc was not found; install razum or set RAZUM_COMPILER to rzc.exe")
    return()
endif()
set(Razum_FOUND TRUE)

function(razum_output_directory result)
    if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(${result} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" PARENT_SCOPE)
    else()
        set(${result} "${CMAKE_BINARY_DIR}" PARENT_SCOPE)
    endif()
endfunction()

function(razum_sources source result)
    get_filename_component(folder "${source}" DIRECTORY)
    file(GLOB_RECURSE found CONFIGURE_DEPENDS "${folder}/*.rz" "${folder}/*.asm")
    set(${result} ${found} PARENT_SCOPE)
endfunction()

function(add_razum_library name source)
    cmake_parse_arguments(RAZUM "" "" "DEPENDS" ${ARGN})
    get_filename_component(source "${source}" ABSOLUTE)
    razum_output_directory(output)
    file(MAKE_DIRECTORY "${output}")
    razum_sources("${source}" sources)
    set(dll "${output}/${name}.dll")
    set(def "${output}/${name}.def")
    set(header "${output}/${name}.h")
    add_custom_command(
        OUTPUT "${dll}" "${def}" "${header}"
        COMMAND "${RAZUM_COMPILER}" "${source}" --dll -o "${dll}"
        DEPENDS ${sources} ${RAZUM_DEPENDS} "${RAZUM_COMPILER}"
        WORKING_DIRECTORY "${output}"
        COMMENT "razum: building ${name}.dll"
        VERBATIM
    )
    if(MSVC)
        set(import_library "${output}/${name}.lib")
        add_custom_command(
            OUTPUT "${import_library}"
            COMMAND "${CMAKE_AR}" /nologo "/def:${def}" /machine:x64 "/out:${import_library}"
            DEPENDS "${def}"
            COMMENT "razum: import library ${name}.lib"
            VERBATIM
        )
    else()
        set(import_library "${dll}")
    endif()
    add_custom_target(${name}_razum ALL DEPENDS "${dll}" "${import_library}")
    add_library(${name} SHARED IMPORTED GLOBAL)
    set_target_properties(${name} PROPERTIES
        IMPORTED_LOCATION "${dll}"
        IMPORTED_IMPLIB "${import_library}"
        INTERFACE_INCLUDE_DIRECTORIES "${output}"
    )
    add_dependencies(${name} ${name}_razum)
endfunction()

function(add_razum_executable name source)
    cmake_parse_arguments(RAZUM "" "" "DEPENDS" ${ARGN})
    get_filename_component(source "${source}" ABSOLUTE)
    razum_output_directory(output)
    file(MAKE_DIRECTORY "${output}")
    razum_sources("${source}" sources)
    set(exe "${output}/${name}.exe")
    set(dependencies)
    foreach(dependency IN LISTS RAZUM_DEPENDS)
        if(TARGET ${dependency})
            list(APPEND dependencies ${dependency})
        endif()
    endforeach()
    add_custom_command(
        OUTPUT "${exe}"
        COMMAND "${RAZUM_COMPILER}" "${source}" -o "${exe}"
        DEPENDS ${sources} ${dependencies} "${RAZUM_COMPILER}"
        WORKING_DIRECTORY "${output}"
        COMMENT "razum: building ${name}.exe"
        VERBATIM
    )
    add_custom_target(${name} ALL DEPENDS "${exe}")
    if(dependencies)
        add_dependencies(${name} ${dependencies})
    endif()
    set_target_properties(${name} PROPERTIES RAZUM_EXECUTABLE "${exe}")
endfunction()
