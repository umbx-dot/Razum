# Projects and packages

Use `rz` when a project has named targets, compiler settings, or dependencies. From a directory inside the project, `rz` finds the nearest `razum.toml`. Pass `--manifest FILE` to pick another one. The `rzc` command remains available for compiling an individual source file.

## Build a project

The [project-build example](../examples/project_build/razum.toml) contains a portable executable and a Windows app that calls a DLL. From the repository root:

```powershell
rz build --manifest examples/project_build/razum.toml --profile release
rz build --manifest examples/project_build/razum.toml --target plugin-app
```

Windows produces `report.exe`, `plugin.dll`, `plugin.h`, `plugin.def` and `plugin-app.exe` under `build/windows-x64/<profile>/`. Linux builds `report`, `plugin.so` and `plugin.h` under `build/linux-x64/<profile>/`. Both paths are relative to the manifest. Each build validates target dependencies before invoking a compiler. A failed compile leaves the previous outputs in place.

### Manifest fields

| Table or field | Purpose |
|---|---|
| `manifest_version = 1` | Selects the manifest format. |
| `[project]` | Requires `name`. Accepts a `version` string. |
| `[profile.dev]`, `[profile.release]` | Select compiler settings. `dev` is the default. Release starts with optimization enabled. Add a table for each custom profile. |
| `optimize`, `keep_asm`, `cpu` | Map to `-O`, `--asm` and `-march=znver3`. The portable CPU is the default. Zen 3 requires optimization to be enabled in the profile or target. |
| `[[target]]` | Define `name`, `kind`, and `source`. Options on the target override the profile. |
| `kind = "exe"` | Build a native executable. On Windows, `subsystem = "console"` or `"gui"` can override the source. |
| `kind = "dll"` | Build a Windows-only DLL, its C ABI header and a module definition file. |
| `kind = "shared"` | Build a DLL on Windows or an `.so` on Linux, with a C ABI header. Linux needs `cc`. Set `RZ_CC` to the full path of another C compiler. |
| `kind = "object"` | Build a COFF `.obj` on Windows or an ELF `.o` on Linux, with a C ABI header. |
| `platforms` | Select `"windows"`, `"linux"`, or both where supported. A build of all targets skips excluded targets. A direct request for one reports an error. |
| `depends_on` | List local target names that must build first. An executable uses an `extern` declaration to call a DLL at run time. |
| `import_dirs` | Additional project-relative `-I` directories. |
| `[dependencies]` | Source package aliases with `git` and `tag` fields. |

The build tools use `build/` for outputs and state. Keep project sources and import roots outside that folder. Names and settings are validated, so typos and dependency cycles fail before compilation.

For pure Razum targets, `rz` checks the manifest, locked dependencies, compiler and assembler binaries, and source contents before reusing outputs. A project with native CMake targets lets CMake and its build tool track their own inputs. Unchanged Razum artifacts keep their timestamps when handed to CMake.

## Install a source package

```text
rz add OWNER/REPO
rz update ALIAS
rz fetch --locked
rz build --offline
```

`rz add OWNER/REPO` selects the highest stable tag. A repository name beginning with `razum-` uses the rest of that name as its import alias. Use `rz add OWNER/REPO@TAG` to pin an exact tag. For other Git servers or an absolute local `file:///` repository, write the dependency directly in `razum.toml`.

`rz update ALIAS` moves a direct dependency to the highest newer stable tag. Both commands compare numeric `MAJOR.MINOR.PATCH` tags, with an optional `v` prefix, and skip prereleases. Use `rz update ALIAS@TAG` for an exact upgrade, downgrade, or prerelease. An existing lock pin stays fixed when the requested tag is already selected. A failed resolution leaves the manifest and lockfile as they were.

```toml
[dependencies]
csv = { git = "https://example.org/owner/razum-csv.git", tag = "v0.1.0" }
```

A source package supplies `csv.rz` or `csv/` at its root for the alias `csv`. Its own `razum.toml` may declare further source packages. Resolution checks that all requests for an alias agree on the source and tag. The compiler uses the existing shared import namespace, so package authors should prefix public helper names.

Commit `razum.lock` together with the manifest. The lockfile records the peeled Git commit of each exact tag, including transitive dependencies. `fetch --locked` fills an empty cache from those commits. `build --offline` requires matching pins and cached sources. Git is needed to fetch. Set `RZ_CACHE` to override the cache directory. `rzc` reads local source paths and runs without package resolution.

Try `rz add umbx-dot/razum-csv` for a CSV field reader or `rz add umbx-dot/razum-discord` for a Discord bot library. Both packages are also included as source examples. [CSV example](../examples/packages/csv) · [Discord bot guide](discord.md)

## C and C++ builds

CMake projects can pass `MANIFEST razum.toml PROFILE release` to `add_razum_library` and `add_razum_executable`. `MANIFEST_TARGET` selects a different manifest target when the CMake target name differs. The functions use the same compiler flag and package resolution code as `rz` and track imported files. The native build still declares its own link dependencies. [CMake examples](cmake.md)

To build a native artifact from `rz`, declare a target with `kind = "cmake"` and point `source` at the CMake project. The default artifact is the platform's executable for the target name. Set `artifact` to override that path, relative to the CMake build tree. Set `cmake_target` when the CMake target name differs, and `preset` to select a configure preset. `depends_on` lists the input artifacts. CMake receives the path to those inputs through `RAZUM_ARTIFACT_DIR`. The native project controls downloads in its custom commands.

## One executable from seven components

Use x86-64 GCC and G++, Rust, cgo-enabled Go, CMake, Ninja and NASM. Windows needs MinGW-w64 and the Rust target `x86_64-pc-windows-gnu`. Linux needs the Rust target `x86_64-unknown-linux-gnu` and static C and C++ runtime archives. Run from the repository root.

```powershell
rz build --manifest examples/mixed_static/razum.toml --profile release --target integration
```

The [mixed static example](../examples/mixed_static) links C, C++, Rust, Go, Razum and NASM with Lua 5.4.9 and an embedded script. The first build obtains the pinned Lua source archive. The output is `examples/mixed_static/build/windows-x64/release/integration.exe` on Windows or `examples/mixed_static/build/linux-x64/release/integration` on Linux.

```text
C=10 C++=12 Rust=15 Go=19 Razum=24 NASM=30 Lua=37
```

Run `python tests/project/test_mixed_static.py build/rz.exe` on Windows or `python3 tests/project/test_mixed_static.py build/rz` on Linux. The test builds the fixture, checks that an offline rebuild reuses the native link, and runs the executable alone in an empty directory. It checks for system DLL imports on Windows and for a fully static ELF on Linux.

### Native ABI

`rzc module.rz --object` emits `module.obj` or `module.o` with `module.h`. Objects support exported C ABI functions and globals, local calls and relocated data. Linux exports use the System V x86-64 ABI. The generated Linux header marks function pointers `ms_abi` because callbacks use the internal Razum convention. Annotate C or C++ callbacks with `__attribute__((ms_abi))` to match. Linux `.so` data exports use protected ELF visibility. Use `dlsym` or compile the consumer with `-fPIC` to access the library storage directly. [NASM ELF symbol visibility](https://www.nasm.us/doc/nasm09.html#section-9.10.5)

Supply the entry point in the native host. Windows objects cannot use the PE imports required by libraries such as `stdio`. On Linux, libraries that rely on the standalone entry point or embedded HTTPS image need a standalone executable. Windows DLL hooks and direct `extern` DLL calls remain Windows features. Use a native host to load and call Linux shared libraries. Keep exceptions on their own side of the ABI boundary.

[TOML parser notice](../rz/vendor/tomlc17/LICENSE) · [Lua license](https://www.lua.org/license.html)
