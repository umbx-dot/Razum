# Razum

Razum is a C-like language for x86-64 Windows and Linux. Its compiler, `rzc`, emits NASM assembly and builds native executables. The standard library is bundled with the compiler and is mostly written in Razum.

```c
import stdio;

int main() {
    print("Hello, world!");
    return 0;
}
```

Save this as `hello.rz` and run `rzc hello.rz`. Run `.\hello.exe` on Windows or `./hello` on Linux.

## Install

Download the installer from [Releases](https://github.com/umbx-dot/Razum/releases).

- **Windows x64.** Run `razum-setup.exe`. It installs the compiler and NASM for your user account.
- **Linux x86-64.** Run `bash razum-setup.sh`. The release notes list the system requirements for the prebuilt compiler.

Open a new terminal after installing. Both installers can also install the VS Code extension.

## Build and test

You need a C17 compiler, CMake 3.20 or later, and NASM. Use Ninja on Windows. Linux builds can also use Make.

**Windows**

Use a Developer PowerShell for MSVC or put MSYS2 UCRT64 GCC on your PATH.

```powershell
.\build.ps1
.\build\rzc.exe examples\hello.rz -o build\hello.exe
.\build\hello.exe
```

**Linux**

```sh
./build.sh
./build/rzc examples/hello.rz -o build/hello
./build/hello
```

Run `bash tests/run_all.sh` to test the compiler and libraries. See the [platform guide](docs/platforms.md) for test dependencies, supported targets, and platform differences.

## What you can build

- Console programs with text, JSON, filesystem, time, math, and process APIs.
- Concurrent programs with threads, channels, and atomics.
- HTTP and HTTPS servers, and desktop apps with the bundled GUI library.
- Windows DLLs and mixed C, C++, and Razum projects through [CMake](docs/cmake.md).

Use `-O` for portable x86-64 optimization. Use `-march=znver3` when targeting Zen 3. See the [optimizer guide](docs/optimizer.md) for what each mode does.

## Documentation

- [Tutorial](docs/tutorial.md)
- [Language reference](docs/language.md)
- [Standard library reference](docs/libraries.md)
- [Compiler architecture](ARCHITECTURE.md) and [code generation](docs/codegen.md)
- [Benchmark results and reproduction notes](KNOWLEDGE.md)

On Windows, run `.\website\start.ps1` to open the local site. It has searchable reference docs, a playground, benchmarks, and 26 lessons.

## Repository layout

- `src/` contains the compiler.
- `libraries/` contains the standard library and runtime.
- `examples/`, `tests/`, and `benchmarks/` contain sample programs and checks.
- `website/`, `editors/vscode/`, and `installer/` contain the site, editor extension, and installers.

Razum is at an early preview stage. File reproducible bug reports in [Issues](https://github.com/umbx-dot/Razum/issues).

## License

Razum uses the [MIT License](LICENSE). Bundled components and assets carry their own notices.
