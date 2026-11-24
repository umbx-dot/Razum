# razum in CMake projects

razum fits into a CMake project next to C and C++: a razum DLL becomes an ordinary CMake library target that C and C++ targets link against, and razum programs can call DLLs built from C or C++ in the same project. [examples/mixed_project](../examples/mixed_project) has all four directions and is built by `tests/test_cmake.sh`.

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

add_razum_executable(report razum/report.rz DEPENDS textutils fastmath)   # razum uses both
```

```powershell
cmake -S . -B build -G Ninja
```

```powershell
cmake --build build
```

## Finding razum

The installer registers razum in CMake's user package registry (`HKEY_CURRENT_USER\Software\Kitware\CMake\Packages\Razum`), so `find_package(Razum)` works in every project without paths. Without the installer, pass the folder holding `RazumConfig.cmake` and, if `rzc` is not on `PATH`, the compiler:

```powershell
cmake -S . -B build -G Ninja "-DRazum_DIR=C:/path/to/razum/cmake/package" "-DRAZUM_COMPILER=C:/path/to/rzc.exe"
```

## Functions

| Function | Does |
|---|---|
| `add_razum_library(name source.rz [DEPENDS ...])` | Builds `name.dll` from the `export` functions of `source.rz` (and everything it imports), with `name.h` and `name.def`. Creates an imported library target `name`: `target_link_libraries(app PRIVATE name)` adds the include folder and links it. With MSVC, the import library `name.lib` is made from the `.def` with `lib.exe`; GCC and Clang link the DLL directly |
| `add_razum_executable(name source.rz [DEPENDS targets...])` | Builds `name.exe`; `DEPENDS` makes it build after the listed targets (for example the C DLLs it calls through `extern`) |

Both rebuild when any `.rz` or `.asm` file in the source's folder changes. Outputs go to `CMAKE_RUNTIME_OUTPUT_DIRECTORY` (or the top build folder), so set that variable to keep every `.exe` next to the DLLs it loads.

## Calling across languages

| From → to | How |
|---|---|
| C / C++ → razum | `#include "fastmath.h"` (generated) and link the target; see [language.md](language.md#making-a-dll) for how types map |
| razum → C / C++ | export the function from the C side with `__declspec(dllexport)` and `extern "C"`, build it as a DLL, and declare it in razum: `extern "textutils" long count_vowels(string text);` |
| razum → razum | `extern "fastmath" long fibonacci(int n);` against a razum DLL, or plain `import` to compile the code into the program |

## Limits

- Single-configuration generators (Ninja, Makefiles): multi-configuration generators such as Visual Studio put programs into `Debug/` and `Release/` folders, which the razum outputs do not follow.
- Windows x64 only, like razum itself.
