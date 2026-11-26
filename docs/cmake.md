# razum in CMake projects

C and C++ can link against Razum shared libraries on Windows and Linux. Windows Razum programs can also call the DLLs in the same CMake project. `tests/test_cmake.sh` builds and verifies the example on both hosts. [Mixed CMake example](../examples/mixed_project)

```cmake
cmake_minimum_required(VERSION 3.20)
project(mixed_project C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})    # all DLLs and programs side by side

find_package(Razum REQUIRED)

add_library(textutils SHARED c/textutils.c)                 # a C DLL
set_target_properties(textutils PROPERTIES PREFIX "")       # textutils.dll, also with MinGW

add_razum_library(fastmath razum/fastmath.rz)               # a razum DLL with export functions

add_executable(app cpp/main.cpp)                            # C++ uses both
target_link_libraries(app PRIVATE fastmath textutils)

if(WIN32)
    add_razum_executable(report razum/report.rz DEPENDS textutils fastmath)
endif()
```

```powershell
cmake -S . -B build -G Ninja
```

```powershell
cmake --build build
```

## Finding razum

Both installers register Razum for `find_package(Razum)` in the user package registry. For a source checkout, pass the folder holding `RazumConfig.cmake` and the compiler path when `rzc` is outside `PATH`.

```powershell
cmake -S . -B build -G Ninja "-DRazum_DIR=C:/path/to/razum/cmake/package" "-DRAZUM_COMPILER=C:/path/to/rzc.exe"
```

## Functions

| Function | Does |
|---|---|
| `add_razum_library(name source.rz [DEPENDS ...])` | Builds `name.dll` or `name.so`, plus `name.h`. `target_link_libraries(app PRIVATE name)` adds the include path and links the library. On Windows the compiler emits a `.def`, and MSVC's `lib.exe` uses it to create `name.lib`. |
| `add_razum_executable(name source.rz [DEPENDS targets...])` | Builds a native executable after the targets in `DEPENDS`. |

Both rebuild when a `.rz` or `.asm` file in the source folder changes. Outputs go to `CMAKE_RUNTIME_OUTPUT_DIRECTORY` or the top build folder.

Pass `MANIFEST razum.toml PROFILE release` to either function to use compiler flags and package paths from the project manifest. `MANIFEST_TARGET` selects a differently named entry. CMake tracks the manifest, lockfile and imported source paths. [Project guide](projects.md)

## Calling across languages

| From → to | How |
|---|---|
| C / C++ → razum | `#include "fastmath.h"` (generated) and link the target; see [language.md](language.md#making-a-dll) for how types map |
| razum → C / C++ | On Windows, export from a DLL and declare it in Razum with `extern "textutils" long count_vowels(string text);`. |
| razum → razum | Use `import` to compile the source into the program. On Windows, a Razum program can also call a Razum DLL through `extern`. |

## Limits

- Single-configuration generators (Ninja, Makefiles): multi-configuration generators such as Visual Studio put programs into `Debug/` and `Release/` folders, which the razum outputs do not follow.
- Windows and Linux x86-64. See the [native ABI guide](projects.md#native-abi) for the object and shared-library runtime limits.
