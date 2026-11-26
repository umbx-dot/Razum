find_program(RAZUM_COMPILER rzc HINTS "${CMAKE_CURRENT_LIST_DIR}/../bin")
if(NOT RAZUM_COMPILER)
    set(Razum_FOUND FALSE)
    set(Razum_NOT_FOUND_MESSAGE "rzc was not found; install razum or set RAZUM_COMPILER to its path")
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

function(razum_manifest_options output inputs manifest name profile)
    if(NOT manifest)
        return()
    endif()
    get_filename_component(manifest "${manifest}" ABSOLUTE)
    get_filename_component(folder "${manifest}" DIRECTORY)
    get_filename_component(compiler_dir "${RAZUM_COMPILER}" DIRECTORY)
    find_program(RAZUM_PROJECT_DRIVER rz HINTS "${compiler_dir}" REQUIRED)
    if(NOT profile)
        set(profile dev)
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env "RZC=${RAZUM_COMPILER}"
        "${RAZUM_PROJECT_DRIVER}" compiler-args --manifest "${manifest}" --target "${name}" --profile "${profile}"
        OUTPUT_VARIABLE arguments OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    string(REPLACE "\r" "" arguments "${arguments}")
    string(REPLACE ";" "\\;" arguments "${arguments}")
    string(REPLACE "\n" ";" arguments "${arguments}")
    set(follow FALSE)
    set(files "${manifest}")
    if(EXISTS "${folder}/razum.lock")
        list(APPEND files "${folder}/razum.lock")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${folder}/razum.lock")
    endif()
    foreach(argument IN LISTS arguments)
        if(follow)
            file(GLOB_RECURSE imported CONFIGURE_DEPENDS "${argument}/*.rz" "${argument}/*.asm")
            list(APPEND files ${imported})
            set(follow FALSE)
        elseif(argument STREQUAL "-I")
            set(follow TRUE)
        endif()
    endforeach()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${manifest}")
    set(${output} ${arguments} PARENT_SCOPE)
    set(${inputs} ${files} PARENT_SCOPE)
endfunction()

function(add_razum_library name source)
    cmake_parse_arguments(RAZUM "" "MANIFEST;MANIFEST_TARGET;PROFILE" "DEPENDS" ${ARGN})
    set(flags)
    set(inputs)
    if(NOT RAZUM_MANIFEST_TARGET)
        set(RAZUM_MANIFEST_TARGET "${name}")
    endif()
    razum_manifest_options(flags inputs "${RAZUM_MANIFEST}" "${RAZUM_MANIFEST_TARGET}" "${RAZUM_PROFILE}")
    get_filename_component(source "${source}" ABSOLUTE)
    razum_output_directory(output)
    file(MAKE_DIRECTORY "${output}")
    razum_sources("${source}" sources)
    set(library "${output}/${name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(header "${output}/${name}.h")
    set(outputs "${library}" "${header}")
    set(byproducts)
    if("--asm" IN_LIST flags)
        set(byproducts BYPRODUCTS "${output}/${name}.asm")
    endif()
    set(linker_env)
    if(WIN32)
        set(def "${output}/${name}.def")
        list(APPEND outputs "${def}")
    elseif(CMAKE_C_COMPILER)
        set(linker_env "${CMAKE_COMMAND}" -E env "RZ_CC=${CMAKE_C_COMPILER}")
        list(APPEND inputs "${CMAKE_C_COMPILER}")
    endif()
    add_custom_command(
        OUTPUT ${outputs}
        ${byproducts}
        COMMAND ${linker_env} "${RAZUM_COMPILER}" "${source}" --shared -o "${library}" ${flags}
        DEPENDS ${sources} ${inputs} ${RAZUM_DEPENDS} "${RAZUM_COMPILER}"
        WORKING_DIRECTORY "${output}"
        COMMENT "razum: building ${name}${CMAKE_SHARED_LIBRARY_SUFFIX}"
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
        set(import_library "${library}")
    endif()
    add_custom_target(${name}_razum ALL DEPENDS ${outputs} "${import_library}")
    add_library(${name} SHARED IMPORTED GLOBAL)
    set_target_properties(${name} PROPERTIES
        IMPORTED_LOCATION "${library}"
        INTERFACE_INCLUDE_DIRECTORIES "${output}"
    )
    if(WIN32)
        set_target_properties(${name} PROPERTIES IMPORTED_IMPLIB "${import_library}")
    endif()
    add_dependencies(${name} ${name}_razum)
endfunction()

function(add_razum_executable name source)
    cmake_parse_arguments(RAZUM "" "MANIFEST;MANIFEST_TARGET;PROFILE" "DEPENDS" ${ARGN})
    set(flags)
    set(inputs)
    if(NOT RAZUM_MANIFEST_TARGET)
        set(RAZUM_MANIFEST_TARGET "${name}")
    endif()
    razum_manifest_options(flags inputs "${RAZUM_MANIFEST}" "${RAZUM_MANIFEST_TARGET}" "${RAZUM_PROFILE}")
    get_filename_component(source "${source}" ABSOLUTE)
    razum_output_directory(output)
    file(MAKE_DIRECTORY "${output}")
    razum_sources("${source}" sources)
    set(exe "${output}/${name}${CMAKE_EXECUTABLE_SUFFIX}")
    set(dependencies)
    foreach(dependency IN LISTS RAZUM_DEPENDS)
        if(TARGET ${dependency})
            list(APPEND dependencies ${dependency})
        endif()
    endforeach()
    set(byproducts)
    if("--asm" IN_LIST flags)
        set(byproducts BYPRODUCTS "${output}/${name}.asm")
    endif()
    add_custom_command(
        OUTPUT "${exe}"
        ${byproducts}
        COMMAND "${RAZUM_COMPILER}" "${source}" -o "${exe}" ${flags}
        DEPENDS ${sources} ${inputs} ${dependencies} "${RAZUM_COMPILER}"
        WORKING_DIRECTORY "${output}"
        COMMENT "razum: building ${name}${CMAKE_EXECUTABLE_SUFFIX}"
        VERBATIM
    )
    add_custom_target(${name} ALL DEPENDS "${exe}")
    if(dependencies)
        add_dependencies(${name} ${dependencies})
    endif()
    set_target_properties(${name} PROPERTIES RAZUM_EXECUTABLE "${exe}")
endfunction()
