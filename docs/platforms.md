# Platforms

razum runs on **Windows** and **Linux**, both x86-64, from the same source tree. `rzc` makes programs for the system it was built on: `rzc` built on Windows writes Windows `.exe` files, `rzc` built on Linux writes Linux executables. Nothing is switched at run time, and a Windows build contains no Linux code paths (and the other way round), so adding Linux changed no byte of any Windows program: the generated `.exe` files of every test, example and benchmark, with and without `-O`, were compared byte for byte before and after.

## What works where

| | Windows 10/11 x64 | Linux x86-64 |
|---|---|---|
| Building `rzc` | `.\build.ps1` (GCC from MSYS2 or MSVC) | `./build.sh` (GCC or Clang) |
| Output | PE `.exe` and DLLs | Static ELF executables and shared `.so` libraries |
| Language, `-O`, `import` of your own modules | yes | yes |
| `stdio`, `text`, `math`, `array`, `json`, `time` | yes | yes |
| `program` (`main(string args[])`, `new`, `free`) | yes | yes |
| `filesystem` | yes | yes (see the differences in [libraries.md](libraries.md#filesystem)) |
| `threads` | yes | yes: `clone` and futexes (see [libraries.md](libraries.md#threads)) |
| `process` | yes | yes: the command line is split with the Windows rules and the program started directly, as on Windows (see [libraries.md](libraries.md#process)) |
| `net` | IPv4 and IPv6 TCP, UDP, DNS, ICMP and native raw IP, plus IPv4 neighbor lookup | Same transport API, with Linux raw Ethernet and ARP frames through `AF_PACKET` (see [libraries.md](libraries.md#net)) |
| `http` | HTTP/HTTPS server and verified HTTPS requests and WebSockets through WinHTTP | HTTP/HTTPS server with native Razum TLS. Verified client requests and WSS use the embedded libcurl backend. [Transport requirements](libraries.md#secure-websocket-client) |
| `crypt` | yes | yes: the algorithms in razum, `getrandom` for random bytes (see [libraries.md](libraries.md#crypt)) |
| `gui` | yes, via GDI+ | yes, native Wayland or local X11, with the same widget API and a software renderer |
| `windows`, `winsock` | yes | by definition no: they declare the Windows API itself. Their Linux counterpart is the `linux` library (`system_call`, constants), which in turn is not available on Windows |
| `extern` functions from DLLs | yes | no (an error names the function and its DLL) |
| `--dll` | yes | no |
| `--object` | x64 COFF `.obj` and C ABI header | x64 ELF `.o` and C ABI header |
| `--shared` | DLL, C ABI header and `.def` | `.so` and C ABI header. Requires `cc` or `RZ_CC`. |
| `rz build` | Manifest targets, profiles and locked source packages | Same driver for Linux targets |
| Installer | `razum-setup.exe` installs `rzc` and `rz` | `razum-setup.sh` installs `rzc` and `rz`, NASM if needed, the CMake package and the VS Code extension |
| Razum website server | yes | planned. The Windows site displays Linux results. |

Tested on Windows 11 build 26200 (GCC 16.2, MSYS2 UCRT64) and Fedora Linux 45 under WSL 2 (GCC 16.2, NASM 3.02, curl, OpenSSL, gawk). ARM64 Linux (for example a Raspberry Pi) can build `rzc`. Generated programs require x86-64. macOS is not supported.

Unicode source, import directory, and output paths in Windows `rzc` require Windows 10 version 1903 or later. NASM receives ASCII temporary names from the output directory when the assembly or binary path contains non-ASCII characters.

## How the source is split

| Where | Windows | Linux |
|---|---|---|
| The compiler's own file and process code | `src/platform/windows/` | `src/platform/linux/` |
| Which target `rzc` writes | `host_target()` in `src/platform/<system>/host_target.c` | the same |
| Program headers and start | `codegen/emit_pe_header.c`, `emit_entry_point.c`, import, export and relocation tables | `codegen/emit_elf_header.c`, `emit_linux_entry_point.c` |
| Library code | files under a folder named `windows/` | files under a folder named `linux/` |

CMake compiles only `src/platform/<host>/`. In the libraries, any file below a folder named `windows` or `linux` belongs to that system only; the other system never loads it. The routines in both folders have the same names (`rz@write`, `rz@read_line`, `rz@allocate`, ...) and the same register rules, so everything above them, every `.rz` file of `stdio`, `text`, `math` and `array`, is shared. The `windows` library itself is a folder named `windows`, so it disappears from Linux programs by the same rule.

## Reserved names

`windows` and `linux` are folder names with a meaning (see above), and `import windows;` and `import linux;` always load the built-in libraries, never a folder of yours. When a module is checked on its own and it sits inside a `windows/` or `linux/` folder, the files of the parent folder are loaded too, so `filesystem/file/linux/open.rz` sees the `File` struct from `filesystem/file/file.rz`.

The `linux` library holds what Linux library code needs: `long system_call(long number, long first, long second, long third, long fourth)` (an `asm` block around the `syscall` instruction that keeps `rsi` and `rdi`) and constants such as `SYS_OPEN`, `O_CREAT`, `SEEK_SET`. It is to Linux what the `windows` library is to Windows.

## The Linux runtime

A Linux program is one static ELF file with no C library and no dynamic linker: an ELF header, two program headers (one loaded segment, readable, writable and executable like the Windows image's one section; and a non-executable stack), then the code and data. It talks to the kernel with `syscall` only:

| Need | System call |
|---|---|
| Output (`rz@write_to`, buffered by the shared `rz@write`) | `write`, repeated until everything is written and retried on `EINTR` |
| Is standard output a terminal (flush at each line end, as on a Windows console) | `ioctl(1, TCGETS)` |
| Input (`rz@read_line`) | `read` from standard input |
| `new` and `free` | `mmap`: blocks of 16 bytes to 32 KiB come from 1 MiB chunks with a free list per power-of-two size; larger blocks get their own `mmap` and `munmap`. Memory from `new` is zeroed, as on Windows |
| Output lock while another thread holds it | `sched_yield` |
| End of the program | `exit_group` with `main`'s result |
| The environment (for `process`) | read from the start stack, after `argv`; only programs that link `rz@environment` store it |

`main(string args[])` reads `argc` and `argv` from the stack the kernel prepared, so arguments arrive exactly as the shell split them. Line ends are `\n` on Linux and `\r\n` on Windows.

razum code keeps the Windows x64 calling convention on both systems (arguments in `rcx`, `rdx`, `r8`, `r9`, 32 bytes of shadow space, `rbx`, `rsi`, `rdi`, `r12`-`r15` and `xmm6`-`xmm15` preserved). On Linux, direct C library calls would need the System V convention. The HTTPS client keeps the outer executable static and runs an embedded backend in a separate process, where glibc and libcurl use their native ABI and thread-local storage.

## Linux desktop sessions

`gui_open` tries the native Wayland socket when `WAYLAND_DISPLAY` is set, then falls back to a local X11 socket from `DISPLAY`. Wayland uses the standard `xdg-shell` and `wl_shm` protocols. This is the path for GNOME/Mutter, KDE/KWin, and Hyprland Wayland sessions. Xfce and other X11 sessions use the X11 path. The app needs the display server for its session and has no dependency on GTK, Qt, libX11, or libwayland. [Wayland wire protocol](https://wayland.freedesktop.org/docs/book/Protocol.html) · [XDG shell](https://wayland.app/protocols/xdg-shell)

WSLg worked with both paths. Native Wayland was tested with `DISPLAY` unset. X11 was tested through WSLg's XWayland with `WAYLAND_DISPLAY` unset. GNOME, KDE, Hyprland, and Xfce have not been tested individually. [GUI details](libraries.md#gui)

## Tests per platform

`bash tests/run_all.sh` runs on both. Tests marked `// platform: windows` or `// platform: linux` run on the named host. The manifest, CMake, HTTPS and parallel-print suites run on both. The Windows console, DLL lifecycle and installer PATH tests run on Windows. The Linux suite checks the ELF entry point, an offscreen GUI regression and each display protocol whose local socket is available.

Linux CI starts headless Weston and Xvfb and sets `RZ_REQUIRE_GUI=1` so missing displays fail the suite. It also installs into a relative prefix in a temporary home, checks PATH, and uninstalls.

The build folder can be chosen with `RZ_BUILD`, which lets Windows and WSL share one checkout: `RZ_BUILD=$HOME/razum-build ./build.sh --test` in WSL builds outside the Windows `build/` folder.
