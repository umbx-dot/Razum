# Knowledge

## Package version selection

`rz add` and `rz update` query Git tags only when selecting a stable version. An explicit tag skips that query. The lockfile records the exact tag and commit, so `rz fetch --locked` and `rz build --offline` keep working after a tag moves. Stable selection compares three unsigned 64-bit numeric components. Use an explicit tag for prereleases or packages with another version scheme. Tag selection and resolution finish before the manifest and lockfile are staged. [Package guide](docs/projects.md#install-a-source-package)

## Project build performance

Measured on a Ryzen 7 5700X with Windows 11 and Fedora 45 under WSL 2. The manifest example builds a report and shared library on both systems, plus a DLL client on Windows. The cold build fetches a tagged CSV package from a local Git repository into an empty cache, and the warm build reuses the outputs with `--offline`. The first four rows are medians of five runs. The mixed-language example uses one fresh build directory and one offline repeat. The native link time comes from Ninja. The initial native total includes downloading and building Lua.

| Workload | Windows | WSL |
|---|---:|---:|
| Project cold build | 262.08 ms | 251.78 ms |
| Cold dependency resolution | 193.40 ms | 16.30 ms |
| Cold Razum targets | 59.80 ms | 229.40 ms |
| Project warm build | 10.38 ms | 5.30 ms |
| Mixed-language cold build | 3988.22 ms | 7206.36 ms |
| Mixed-language Razum object | 23.30 ms | 6.00 ms |
| Mixed-language native link | 92.00 ms | 483.00 ms |
| Mixed-language warm build | 177.32 ms | 47.99 ms |

Ninja skipped the final link on both warm runs. The wrapper object has identical bytes across staging directories. Linux NASM reads the emitted assembly on standard input so the ELF file symbol has a stable name. Windows NASM uses `--reproducible` to remove COFF timestamp and file-name variation.

Run `python benchmarks/run_projects.py build/rz.exe --native` on Windows or `python3 benchmarks/run_projects.py build/rz --native` on Linux. Omit `--native` to measure the project driver without the Rust, Go and Lua integration. [Project guide](docs/projects.md)

## Library performance

Use `benchmarks/run_libraries.py` with `--baseline PATH` to run the previous and current portable `-O` builds in alternating order. It discards a warm-up and checks each run's result. The suite covers all 17 bundled modules across Windows and Linux. The `linux` API runs on Linux. The `windows` and `winsock` API probes run on Windows. A baseline that predates `net` skips its two workloads.

The probes measure representative workloads. Most times include process startup. The two `text` probes time the search loop in the program. The `http` probe times one local connection carrying 25,600 pipelined plain HTTP requests and includes the driver's response parsing. The two `net` probes each run 20,000 loopback request/reply pairs with 32-byte messages and check the echoed bytes. Filesystem tests use each host's temporary directory. GUI tests render offscreen.

### Windows and Linux baseline

Ryzen 7 5700X, Windows 11 and Fedora under WSL 2 on November 24, 2026. Times are five-run medians in milliseconds using portable `-O`. The baseline is `77565be`. The candidate adds buffered TCP lines, native multi-socket waits, and HTTPS GET. Every measured run passed its output check. The legacy `http` and `process` probe binaries changed on both platforms. The remaining legacy probe binaries matched byte for byte.

| Workload | Windows before → after | WSL before → after |
|---|---:|---:|
| `array`, 900 integer sorts | 26.520 → 27.562 | 26.717 → 25.609 |
| `crypt.sha256`, 23.4 MiB | 16.662 → 15.528 | 289.170 → 281.518 |
| `crypt.aes_gcm`, 8 MiB | 5.041 → 4.926 | 34.346 → 35.468 |
| `filesystem`, 32 MiB written and read back | 74.903 → 73.862 | 34.180 → 33.579 |
| `gui`, 400 offscreen frames | 50.960 → 50.494 | 10.481 → 9.374 |
| `http`, 25,600 loopback responses | 60.776 → 60.493 | 72.635 → 73.071 |
| `json`, 300,000 parse and lookup cycles | 88.984 → 90.721 | 91.204 → 91.077 |
| `linux`, 500,000 environment lookups | linux only | 38.602 → 38.473 |
| `math`, 1.5 million square root, sine and log groups | 87.806 → 87.368 | 82.673 → 83.013 |
| `net.tcp`, 20,000 loopback round trips | 204.070 → 197.474 | 180.182 → 182.528 |
| `net.udp`, 20,000 loopback round trips | 191.757 → 187.920 | 137.355 → 135.290 |
| `process`, 25 child launches with captured output | 258.067 → 258.365 | 78.460 → 68.015 |
| `program`, 25,000 small allocations and frees | 4.454 → 4.222 | 3.997 → 3.577 |
| `stdio`, 120,000 numbered lines written to a pipe | 14.601 → 14.637 | 10.409 → 9.859 |
| `text.early`, 65,000 searches with the match at byte 7 | 0.268 → 0.245 | 0.228 → 0.182 |
| `text.late`, 65,000 searches with the match at byte 8,191 | 128.468 → 127.676 | 125.555 → 120.326 |
| `threads`, 250,000 channel handoffs | 17.140 → 16.873 | 165.106 → 180.219 |
| `time`, 200,000 UTC formats | 23.565 → 23.564 | 19.784 → 19.864 |
| `windows`, 10 million process ID calls | 26.194 → 25.272 | windows only |
| `winsock`, 750,000 IPv4 address round trips | 91.924 → 91.506 | windows only |

The earlier chunked binary I/O change took the 32 MiB workload from 175 → 58 ms on Windows and 147 → 33 ms on WSL against `bf6f416`. Only the filesystem probe executable changed in that run. A separate probe measured two million one-byte calls in each direction. Writes went from 14.67 → 9.32 ms on Windows and 8.07 → 7.73 ms on WSL. Reads went from 6.85 → 6.42 ms and 6.49 → 6.47 ms. The one-byte path accesses the buffer directly to avoid the bulk copy call.

The earlier `find_char` change took the early-match probe from 116 ms to 0.20 ms on Windows and from 119 ms to 0.23 ms on WSL. A match at the end took 346 → 123 ms on Windows and 475 → 119 ms on WSL.

### Discord transport regression

Ran the existing library benchmarks against `9c1d76f` on Windows 11 and Fedora 45 under WSL 2. Values are five-run medians in milliseconds. The HTTP workload measures the existing pipelined loopback server. The crypt workloads check the existing digest and AES-GCM paths after adding Ed25519 verification.

| Workload | Windows before → after | WSL before → after |
|---|---:|---:|
| HTTP loopback | 51.643 → 52.067 | 66.445 → 66.802 |
| SHA-256 | 14.288 → 14.518 | 276.947 → 276.368 |
| AES-GCM | 4.350 → 4.349 | 34.103 → 34.536 |
| Process capture | 209.908 → 210.250 | 78.164 → 68.266 |

The Discord transport fixture uses a generated local TLS certificate and checks a binary PATCH request, WebSocket framing, and Gateway messages. The public Discord Gateway Hello was verified separately on both hosts. Linux requires a libcurl build with WSS enabled. [Discord package](docs/discord.md) · [WebSocket transport](docs/libraries.md#secure-websocket-client)

### Linux AES-GCM phase timings

`rzc benchmarks/gcm_phases.rz -O -o /tmp/gcm-phases` builds a Linux probe that times counter-mode encryption and GHASH plus tag generation separately on a single 8 MiB message. On the same host, nine in-process samples before and after gave medians of **1,046.237 → 21.724 ms** for encryption and **441.383 → 13.626 ms** for authentication. The original 512-message comparison against `a3ef290` included key setup and took 1,476.841 → 38.842 ms on WSL. The Windows AES-GCM probe binary matched byte for byte in that comparison.

Linux records the CPU feature flags during AES key expansion. AES-NI runs the AES block rounds, and PCLMULQDQ multiplies the GHASH polynomial when SSSE3 is also available for bit reflection. The accelerated GHASH uses four 64-bit products and reduces the high half modulo the GCM polynomial. The scalar paths cover CPUs without the corresponding flags. AES key expansion and CBC decryption still use the scalar tables. [Intel feature bits](https://www.intel.com/content/www/us/en/developer/articles/technical/trusted-cpu-feature-detection-library-for-intel-sgx.html) · [GCM definition](https://csrc.nist.gov/pubs/sp/800/38/d/final)

### Linux channel wakeups

The probe now accepts capacity and item count. Build it with `rzc benchmarks/libraries/threads_probe.rz -O -o /tmp/threads-probe` on Linux. Run `/tmp/threads-probe 256 250000` for the default workload. It verifies every received value in order.

The timing columns are seven-run WSL medians with a warm-up and alternating builds. Separate `ptrace` runs count completed `FUTEX_WAIT` and `FUTEX_WAKE` calls across both threads for 10,000 values. `Empty wakes` count syscalls that returned zero, including mutex wakes. Tracing changes scheduling, so use the untraced runs for timings.

In the separate channel-only library run, the default workload took 497.679 → 203.063 ms on WSL and 16.594 → 16.740 ms on Windows.

| Capacity | Timed values | Time before → after | Waits before → after | Wakes before → after | Empty wakes before → after |
|---:|---:|---:|---:|---:|---:|
| 0 | 40,000 | 1,952.216 → 1,969.595 ms | 28,203 → 11,393 | 48,526 → 12,780 | 37,712 → 4,170 |
| 1 | 40,000 | 1,967.056 → 1,955.044 ms | 18,344 → 11,064 | 39,760 → 12,418 | 31,065 → 4,106 |
| 256 | 250,000 | 505.296 → 198.262 ms | 17,723 → 10,798 | 39,572 → 12,386 | 31,293 → 4,955 |
| 4,096 | 250,000 | 486.583 → 181.702 ms | 17,838 → 6,366 | 39,614 → 7,133 | 31,266 → 2,221 |

A condition word stores its futex sequence in the low 32 bits and its registered waiter count in the high 32. The waiter registers and snapshots the sequence atomically while holding the associated mutex, then releases it. A signal seeing a registered waiter advances the sequence before calling `FUTEX_WAKE`. A waiter that arrived at the kernel after the sequence advanced gets `EAGAIN` and rechecks the predicate with the mutex held. A wait releases its registration on every return, including timeout and interruption. Close and wait-group completion publish the predicate under the associated mutex before broadcasting. [Linux futex semantics](https://www.kernel.org/pub/linux/docs/man-pages/book/man-pages-6.17.pdf)

### Quick source review

| Library | What to profile or improve |
|---|---|
| `array` | Radix sort allocates scratch space and builds a histogram for each byte pass on every sort. Separate allocation cost from histogram cost for repeated sorts. [Radix sort](libraries/array/integers/radix_sort.rz) |
| `crypt` | Linux AES-GCM uses AES-NI and carryless multiplication on supported CPUs. The remaining bulk cost includes per-byte buffer work, and the separate P-256 timing review remains open. [Phase timings](#linux-aes-gcm-phase-timings) |
| `filesystem` | Binary reads and writes copy contiguous buffer sections. The portable regression covers buffer boundaries, partial reads, seek and sticky write failures. [Read](libraries/filesystem/file/read_bytes.rz) · [Write](libraries/filesystem/file/write_bytes.rz) |
| `gui` | Linux rendering clears the whole buffer each frame. Add separate tests for full redraws and small updates before attempting less repaint work. [Linux rendering](libraries/gui/render/linux/gui_render.rz) |
| `http` | Static responses read chunks into a temporary buffer and copy them into the response. Benchmark large files and HTTPS handshakes separately from the lightweight route before changing this. [Linux file responses](libraries/http/routing/linux/http_serve_file.rz) · [Windows file responses](libraries/http/routing/windows/http_serve_file.rz) |
| `json` | Object member lookup uses the escape-aware byte loop for plain keys too. Measure that path separately before adding a fast path. [Key matching](libraries/json/strings/json_key_matches.rz) |
| `linux` | Environment lookups scan the original entries on each call. The probe measures that scan and depends on the environment size. [Environment lookup](libraries/linux/environment_value.rz) |
| `math` | The combined benchmark uses `sqrt`, `sin` and `log`. Split those timings and verify numerical error before changing the x87 routines. [Sine](libraries/math/real/sin.rz) · [Logarithm](libraries/math/real/log.rz) |
| `process` | Profile startup, pipe drain and wait separately. Preserve timeout and output limits when changing the capture loop. [Windows process run](libraries/process/windows/process_run.rz) · [Linux process run](libraries/process/linux/process_run.rz) |
| `program` | The Linux small-block allocator uses a global lock. Add a contended allocation probe before choosing an allocator change. [Linux allocator](libraries/program/assembly/linux/allocate.asm) |
| `stdio` | Integer formatting divides by 10 for each digit. Benchmark digit-pair formatting separately from pipe output. [Integer formatting](libraries/stdio/assembly/format_int.asm) |
| `text` | `find_char` used to scan the entire string for its length before searching. A single byte scan now returns as soon as it finds the match or the terminator. [Character search](libraries/text/find_char.rz) |
| `threads` | Linux conditions skip wake syscalls when their registered waiter count is zero. Capacity 0 and 1 still repeatedly hand off between threads. [Channel timings](#linux-channel-wakeups) |
| `time` | UTC formatting emits decimal fields one digit at a time. Measure a fixed-width digit-pair formatter alongside date boundary tests. [Decimal fields](libraries/time/utc/time_put_decimal.rz) |
| `windows` | Native DLL calls make a useful baseline for call overhead. This probe times `GetCurrentProcessId`. [Kernel32 bindings](libraries/windows/kernel32.rz) |
| `winsock` | Address conversion runs in Winsock. The probe checks the native binding and can help verify changes in its callers. [Winsock bindings](libraries/winsock/ws2_32.rz) |

## Current compiler benchmarks

Ryzen 7 5700X, Windows 11, November 24, 2026. The baseline is `da45598` and the candidate prioritizes loop-heavy values when integer registers fill. Times are elapsed milliseconds, with seven interleaved runs after a warm-up. Each executable produced identical output for its workload. GCC and G++ 16.2 use `-O3 -march=znver3 -fwrapv -ffp-contract=off`. Rust 1.99 uses `-C opt-level=3 -C target-cpu=znver3`.

| Workload | Before `-O` | After `-O` | Before Zen 3 | After Zen 3 | C | C++ | Rust |
|---|---:|---:|---:|---:|---:|---:|---:|
| Mandelbrot | 116.52 | 115.98 | 57.29 | 57.52 | 207.01 | 206.61 | 207.33 |
| Integer loop | 98.62 | 99.15 | 50.63 | 50.55 | 90.66 | 90.51 | 59.17 |
| Quicksort | 337.81 | 337.79 | 338.00 | 338.08 | 394.95 | 396.91 | 307.09 |
| Collatz | 329.45 | 329.81 | 330.11 | 331.33 | 341.69 | 339.52 | 294.26 |
| Fibonacci (runtime input 38) | 3.10 | 3.12 | 3.00 | 3.25 | 45.45 | 45.47 | 81.71 |
| Sieve | 204.61 | 204.67 | 203.92 | 204.66 | 183.91 | 184.81 | 202.41 |

The six Razum benchmark binaries matched the baseline byte for byte for each target on Windows and WSL. Their timing differences reflect run-to-run variation. The register-pressure probe exercises the allocator when more eligible values are present than there are registers.

The wider Windows comparison covered 69 programs from the tests, examples and standard compiler benchmarks. All 69 plain binaries matched byte for byte. For each optimized target, 44 matched and 25 changed.

### Register allocation under pressure

The allocator keeps declaration order until all seven non-volatile integer registers are in use. It weights loop references by 1, 4, 16 and 64 per nesting level. Bases and indices that can form a scaled x86 address get four times that weight. A replacement needs more than three times the incumbent's score. The allocator also keeps the single wide parameter eligible for tail-recursion lowering. The pressure probe runs 100 million iterations with eight parameters and a side-effecting helper. Run `rzc tests/programs/register_pressure.rz -O -o register-pressure`, then pass `100000000` to the executable. The second row group uses the explicitly cached keys from the [Quicksort experiment](#quicksort-load-reuse-experiment). Times are milliseconds, with 11 interleaved timed runs after a warm-up and matching output.

| Probe | Target | Windows before → after | WSL before → after |
|---|---|---:|---:|
| Register pressure | `-O` | 422.10 → 122.72 | 399.33 → 109.89 |
| Register pressure | `-march=znver3` | 422.22 → 122.79 | 396.83 → 109.97 |
| Quicksort with explicit keys | `-O` | 312.63 → 312.33 | 329.74 → 334.90 |
| Quicksort with explicit keys | `-march=znver3` | 313.04 → 311.25 | 329.62 → 332.32 |

The loop's two accumulators and induction variable move from stack slots into registers in the pressure probe. The explicitly cached Quicksort keys also fit in registers, but this follow-up timing showed no clear speedup. The Linux environment probe retains its original scaled addressing and matches the baseline byte for byte.

In the separate allocator-only library run against `da45598`, 900 bundled integer sorts took 35.602 → 26.269 ms on Windows and 33.264 → 25.271 ms on WSL. All 18 cross-platform library workloads were rerun.

WSL process-startup times varied across library runs. A focused 31-pair repeat of the process probe measured a median paired difference of +0.75%. Individual measurements ranged from 42 to 94 ms on both builds.

### WSL compiler measurements

Fedora under WSL 2 on the same host. Seven interleaved timed runs after warm-up. GCC 16.2.1 uses the same C flags as on Windows. The table reports elapsed milliseconds.

| Workload | Portable `-O` before → after | Zen 3 before → after | C |
|---|---:|---:|---:|
| Mandelbrot | 115.56 → 115.44 | 56.54 → 56.28 | 206.39 |
| Integer loop | 97.33 → 97.48 | 49.52 → 49.56 | 89.44 |
| Quicksort | 332.29 → 332.68 | 333.08 → 334.85 | 411.56 |
| Collatz | 328.89 → 329.42 | 329.41 → 328.85 | 341.05 |
| Fibonacci (runtime input 38) | 0.60 → 0.56 | 0.59 → 0.66 | 40.92 |
| Sieve | 242.26 → 246.94 | 237.85 → 251.33 | 228.07 |

## Masked double loops

Mandelbrot on Windows 11 and a Ryzen 7 5700X on November 23, 2026. Each entry is the median elapsed process time of nine interleaved rounds after one warm-up. All executables printed `439736`. The baseline compiler is `39b57bb`. GCC and G++ 16.2 use `-O3 -march=znver3 -fwrapv -ffp-contract=off`. Rust 1.99 uses `-C opt-level=3 -C target-cpu=znver3`.

| Build | Median elapsed time | Executable bytes |
|---|---:|---:|
| Previous portable `-O` | 243.82 ms | 1,992 |
| Portable `-O` with SSE2 | 119.03 ms | 2,608 |
| Previous `-march=znver3` | 244.40 ms | 1,976 |
| `-march=znver3` with AVX2 | 59.51 ms | 2,488 |
| C | 211.07 ms | 38,747 |
| C++ | 215.64 ms | 38,747 |
| Rust | 212.46 ms | 1,230,291 |

The optimizer accepts a specific shape of independent nested double loops. The counted total cannot feed a lane's calculations, and the bounds and step limit must remain invariant. The packed inner loop has enough registers for up to 15 values including comparison scratch, with one more register for the active-lane mask. A candidate that cannot fit or that needs an unsupported operation follows the scalar path. Division is available in initializers, where every lane runs. A division in the guard or update forces the scalar path because inactive lanes would otherwise divide zero by zero. Enabled floating-point traps also take the scalar path. The SSE2 path resumes the scalar loop from the current group on an unordered comparison so a quiet NaN does not set the invalid flag. See [optimizer behavior](docs/optimizer.md).

Both variants agree with the scalar count and floating-point sticky flags across 480 differential cases and all four rounding modes. Of 61 comparable Windows binaries, all matched byte for byte without optimization. With `-O` and with `-march=znver3`, only Mandelbrot changed. In the Quicksort assembly the two partition scan comparisons read their keys from memory, then the swap loads the same keys again.

### Quicksort load-reuse experiment

Ryzen 7 5700X on November 24, 2026. First, a hand-edited assembly prototype loaded each key into a scratch register for the scan comparison and reused it in the swap. Over 21 alternating timed runs on Windows, portable `-O` took **343.17 → 335.29 ms** and `-march=znver3` took **345.43 → 334.62 ms**.

A separate source prototype initialized each key before its scan loop, refreshed it after moving the index, and wrote the two saved values during the swap. The standard benchmark sorted 5 million `long` values and checked the output checksum. The bundled sort probe exercised the same rewrite on 2,000 arrays of 2,048 values each, timing the sorts and checking every resulting order and the checksum. Times are milliseconds over 11 alternating measured runs after a warm-up.

| Workload | Target | Windows before → after | WSL before → after |
|---|---|---:|---:|
| Quicksort benchmark | `-O` | 341.92 → 313.41 | 333.54 → 331.59 |
| Quicksort benchmark | `-march=znver3` | 340.26 → 313.44 | 334.36 → 331.76 |
| Bundled Quicksort probe | `-O` | 134.50 → 137.58 | 136.69 → 137.38 |
| Bundled Quicksort probe | `-march=znver3` | 132.63 → 136.31 | 137.89 → 137.30 |

The allocator used for this experiment put the right key on the stack because seven register values were already live. The benchmark gain varied by host and the bundled sort showed no consistent gain. See the [later register-allocation measurements](#register-allocation-under-pressure).

## Inlining and loop invariants

Windows 11 on the Ryzen 7 5700X, November 23, 2026. Median process milliseconds from seven interleaved runs after one warm-up. Each row checks identical output across its executables. The baseline compiler is from `7e905e7`. GCC and G++ 16.2 use `-O3 -march=znver3 -fwrapv -ffp-contract=off`. Rust 1.99 uses `-C opt-level=3 -C target-cpu=znver3`. Peer columns are from the Zen 3 run.

| Workload | Before `-O` | After `-O` | Before Zen 3 | After Zen 3 | C | C++ | Rust |
|---|---:|---:|---:|---:|---:|---:|---:|
| Library `min` in the loop | 332.54 | 133.64 | 330.90 | 132.67 | | | |
| Invariant signed division | 155.09 | 24.92 | 155.29 | 25.10 | | | |
| Quicksort | 338.87 | 339.09 | 341.24 | 340.80 | 400.52 | 406.46 | 311.80 |
| Mandelbrot | 238.91 | 243.01 | 239.93 | 242.27 | 207.82 | 208.98 | 210.97 |
| Integer loop | 99.68 | 99.10 | 51.46 | 51.38 | 91.72 | 91.31 | 59.95 |

The helper probe imports `math.integer` and sums `min(state, cap)` over 100 million iterations with `cap = 38000` and `state = (state * 1664525 + 1013904223) & 65535`, starting from `state = 1`. The division probe sums `quotient ^ i` for `i` from zero through 99,999,999. Each iteration declares `long quotient = numerator / denominator;` with runtime inputs `987654321` and `173`. The helper executable stays at 1,552 bytes. The division probe grows from 2,064 to 2,072 bytes.

All six standard benchmark binaries match the previous compiler byte for byte with the same optimized target flags. Their timing differences show run-to-run variation. All 60 comparable Windows outputs without optimization also match byte for byte. With either optimized target, 35 of those 60 outputs match and 25 contain changed code. Mandelbrot is next for independent floating-point SIMD lanes because it has a larger measured Zen 3 gap than Quicksort. See [optimizer behavior](docs/optimizer.md).

## Loop optimization

Windows 11 on the Ryzen 7 5700X, November 23, 2026. Median process milliseconds over seven rounds after one warm-up, with interleaved execution and identical output checks. Each before/after pair uses the same target flags. The baseline is `c723e9a`. G++ 16.2 uses `-O3 -march=znver3 -fwrapv -ffp-contract=off`. Rust 1.99 uses `-C opt-level=3 -C target-cpu=znver3`.

| Workload | Before `-O` | After `-O` | Before Zen 3 | After Zen 3 | C++ | Rust |
|---|---:|---:|---:|---:|---:|---:|
| Integer loop | 175.30 | 98.46 | 175.50 | 50.96 | 90.44 | 58.61 |
| Mandelbrot | 325.23 | 238.16 | 282.73 | 237.89 | 207.74 | 206.40 |

SSE2 processes two 64-bit or four 32-bit elements per group. AVX2 doubles those widths. The wide induction square uses a packed 32-by-32-to-64 multiply when its range fits, or includes the high-word contribution when the range is unknown. Floating-product reuse keeps the test and body calculations at the same precision and preserves the short-circuit test order. See [optimizer behavior and benchmark commands](docs/optimizer.md).

## Compiler optimization on Zen 3

Windows 11 on Ryzen 7 5700X, November 23, 2026. Median elapsed milliseconds over five runs after one warm-up, including process startup. Benchmarks require identical output. The old compiler is `625d8f5` with `-O`. The C and C++ columns use GCC/G++ 16.2 with `-O3 -march=znver3 -fwrapv -ffp-contract=off`. Rust 1.99 uses `-C opt-level=3 -C target-cpu=znver3`. Those peer timings are from the run with portable `-O`. The separate `-march=znver3` run changed only the Razum target flags.

| Workload | Old `-O` | New `-O` | New Zen 3 | C | C++ | Rust |
|---|---:|---:|---:|---:|---:|---:|
| Collatz | 516.86 | 329.18 | 330.37 | 339.50 | 339.67 | 293.93 |
| Fibonacci (runtime input 38) | 146.94 | 3.07 | 3.23 | 45.94 | 46.52 | 81.51 |
| Integer loop | 204.74 | 175.58 | 175.25 | 90.90 | 91.25 | 58.73 |
| Mandelbrot | 480.25 | 325.49 | 283.00 | 206.66 | 206.57 | 206.79 |
| Quicksort | 339.92 | 337.66 | 340.80 | 396.94 | 395.63 | 306.79 |
| Sieve | 222.54 | 206.37 | 216.36 | 184.77 | 188.85 | 203.64 |

Goose `a5ab89f`, built locally with its GCC backend, took 4.22 ms for Fibonacci in the same run. Razum's pure-recursion table removes many small recursive calls. At that runtime the process launch is a substantial part of the measurement. Quicksort took 300.77 ms with GCC `-O2` and 301.63 ms with G++ `-O2`, ahead of Razum. The integer loop and Mandelbrot have the largest gaps against the other compilers. See [optimizer behavior and commands](docs/optimizer.md).

A WSL loopback sanity check with two server workers, 64 connections, and 16 pipelined requests per write measured 1.23–1.45 million req/s before and 1.34–1.62 million req/s after, with zero failures. Two runs per compiler give overlapping ranges.

## Audit follow-up measurements

Compared against `5af3f11` on November 23, 2026. In Fedora under WSL, a loopback static `HEAD` for a 32 MiB file went from 4.73 ms to 0.21 ms (median of eight after warm-up). The server's resident memory increase went from 32 MiB to zero. A 64 MiB same-filesystem copy took 33.8 ms before and 31.4 ms with staging and the kernel copy fast path (median of seven). Cross-filesystem copies can use the existing buffered loop when the kernel fast path is unavailable.

On Windows, the HTTP example fell from 39.1 ms to 21.8 ms for `--check` and from 284.5 ms to 270.2 ms for a full build. The hello-world full build took 16.8 ms before and 17.3 ms after. These are wall-clock medians of 17 interleaved runs. Comparing executable outputs with and without `-O` gave 106 byte-for-byte matches and six changes from the static HTTP handler and its test.

## Measured

Measured 2026-11-21.

| What | Value | How |
|---|---|---|
| `build/rzc.exe`, GCC 16.2 Release | 168,216 bytes | `stat -c %s`; includes the embedded libraries |
| `build/msvc/rzc.exe`, MSVC 14.51 Release | 88,576 bytes | `stat -c %s` |
| `int main() { return 0; }` | 372 bytes | `stat -c %s` |
| `examples/hello.exe` | 576 bytes | `stat -c %s` |
| `examples/tour.exe` (prints doubles) | 1,880 bytes | `stat -c %s`; double printing adds about 900 bytes of library code |
| `tests/programs/all_types.exe` | 3,048 bytes | `stat -c %s` |
| `examples/tour.asm` | 606 lines | `wc -l` |
| Compiler sources | 89 `.c` files, 20 headers | `find src` |
| `razum-setup.exe` | 2,280,070 bytes (GCC), 2,143,232 bytes (MSVC); `nasm.exe` is about 1.9 MB of it | `stat -c %s`, `ls` |
| `http` on 2 logical cores (affinity mask 3, one physical core with SMT), `GET /` → 13-byte body, load generator on the other 14 | 1,566,000 req/s with 16 pipelined requests per write (64 connections); 161,000 req/s one request per round trip (64 connections); 91,000 req/s over 1,000 connections; 0 failures; server working set 5 MB | Go load generator in the session scratchpad (`-c`, `-p`, 5 s runs), 2026-11-22 |
| Learning website, one Run | compile 16–31 ms, run 16 ms for a small program, through `/api/run` | the server's own `milliseconds` fields, 2026-11-22 |
| `http` HTTPS on 2 logical cores, ECDSA P-256 test certificate, TLS 1.3 | 117,000 req/s on keep-alive connections (64); 1,072,000 req/s with 16 pipelined; 2,470 new handshakes/s (64 clients, new connection per request; part of each handshake runs in LSASS, which is not limited to the 2 cores); server working set 9 MB | Go load tool with `-tls` and `-new`, 2026-11-22 |
| `http` idle connections | 64,401 held (then the load tool's own connects failed); server private memory 1.2 → 36.9 MB = 0.57 KB per connection; system nonpaged pool +627 MB for both ends on one machine ≈ 4.9 KB per socket; 2.2 CPU seconds in total while holding them | same tool, `-mode idle -sources 8`, PowerShell counters, 2026-11-22 |
| `threads` primitives, 16 logical cores | buffered channel 45 ns per value (1 sender, 1 receiver, capacity 1024); unbuffered round trip 650–700 ns; uncontended mutex lock + unlock 7 ns; uncontended `atomic_add` 1.7 ns; thread start + join 90 µs | `build/m/thread_bench.rz`, `QueryPerformanceCounter`, two runs, 2026-11-22 |
| `crypt` throughput, Ryzen 7 5700X, 16 MiB random input | MD5 690 MB/s, SHA-1 857, SHA-256 2,270, SHA-384 730, SHA-512 730, HMAC-SHA256 2,300; AES-256-GCM encrypt 3,000–3,400, CBC 1,200–1,270, CTR 315–327 (key stream and xor in razum); `secure_bytes` 5,200. Pure razum: base64 encode 445 / decode 137 (of text), hex 315 / 170, base32 290 / 245, base85 143 / 77, CRC-32 418, Adler-32 1,130, `xor_bytes` 550 | `crypt_bench.rz` (scratch, not in the repo) with `QueryPerformanceCounter`, two runs, 2026-11-22 |
| `crypt` per-call cost | `hash()` of 64 bytes 205–224 ns, raw `BCryptHash` 194–204 ns; so `hash` keeps its single start/update/finish path instead of a `BCryptHash` shortcut | the same run |
| `examples/worker_pool.rz` | 148,933 primes below 2,000,000 on 16 threads in 0.059 s | `time` |
| `gui_demo.exe` after the library constants became `const` | 45,084 bytes (was 45,756) | `stat -c %s`, 2026-11-22 |
| `gui` frame of `examples/gui_demo.rz`, 880×600 at 96 DPI, off-screen | 4.8 ms layout and drawing, 54 drawing commands | `build/m/gui_bench.rz`, 300 frames, `QueryPerformanceCounter`, 2026-11-21 |
| `gui_demo.exe` idle | 0 ms CPU over 3 s, 18.9 MB working set | PowerShell `TotalProcessorTime`, `WorkingSet64`, 2026-11-21 |
| `gui_demo.exe` | 45,756 bytes | `ls -la` |
| Test cases | 29 programs (28 + modules), 61 errors, a parallel print check, 11 HTTPS checks with curl, 2 multi-error files, 11 editor lookups, a Unicode console self-test, 7 code shapes, 306 module checks, a razum DLL used from razum and C, a mixed C/C++/razum CMake project, installer path-list unit test | `bash tests/run_all.sh` |
| Printing 200,000 lines to a file (`write_string` + `print` per line) | 0.97 s unbuffered, 0.017 s buffered (57× faster); identical output | `build/m/bench.rz`, best of 3 with PowerShell `Measure-Command`, 2026-11-21 |
| Cost of buffered output | +388 bytes per program that prints | 728 → 1,116 bytes for the benchmark |
| Loop speed vs C (benchmark by another tool) | faster than `gcc -O0`, slower than `gcc -O2` | `C:\Users\idom\Desktop\test` |


### Benchmarks against C (`python benchmarks/run.py`)

Best of 3 runs, Ryzen 7 5700X, gcc 16.2 `-O2`, 2026-11-22, rzc without optimization (every local in a stack slot). Output identical in every case.

| Benchmark | What it stresses | razum ms | gcc -O2 ms | razum / C |
|---|---|---|---|---|
| `fib` | recursive calls, `fib(38)` | 172 | 54 | 3.19 |
| `loop` | an integer loop, 400M steps | 444 | 168 | 2.64 |
| `mandel` | `double` arithmetic, 1200² points | 788 | 207 | 3.81 |
| `collatz` | branches and shifts | 683 | 315 | 2.17 |
| `sieve` | byte array, 50M | 337 | 202 | 1.67 |
| `quicksort` | array loads and stores, 5M `long` | 444 | 301 | 1.47 |

Program sizes: razum 1.6–2.4 KB, gcc 38–40 KB.

With `rzc -O` (variables in registers): `loop` 206 ms (1.22x), `quicksort` 342 (1.14x), `sieve` 215 (1.18x), `collatz` 518 (1.65x), `mandel` 480 (2.31x), `fib` 172 (3.26x). Details in [docs/optimizer.md](docs/optimizer.md).
### Sorting

`array_sort_items_long` (`rzc -O`) against `std::sort` and `qsort` (g++ 16.2 `-O2`), random 64-bit `long` keys (every byte differs, so radix sort makes all 8 passes), best of many rounds, 2026-11-22:

| Elements | razum | `std::sort` | `qsort` |
|---|---|---|---|
| 1,000 | 0.028 ms | 0.022 ms | 0.057 ms |
| 100,000 | 3.32 ms | 3.76 ms | 9.35 ms |
| 5,000,000 | 189 ms | 248 ms | 614 ms |

Where razum's radix sort overtakes its own introsort, 64-bit keys: 4,000 elements (0.128 against 0.138 ms); at 2,000 introsort still wins (0.063 against 0.067 ms). Hence the threshold `48 × bits`. Below about 3,000 elements razum is 1.2–1.3x `std::sort`, which is code generation, not the algorithm.

### JSON parser

A local Windows x86-64 run with `rzc -O` repeatedly parsed arrays of 4,096 objects with a name, integer ID, and boolean field. Median of three runs, measured around parsing only. The caller reused the same token buffer.

| Input per parse | Scalar prototype | Current parser |
|---|---:|---:|
| 167,937 bytes, five-byte ASCII names | 407 MB/s | 392 MB/s |
| 1,163,265 bytes, 248-byte ASCII names | 675 MB/s | 2,422 MB/s |

The SSE2 path helps long ASCII strings. Short records are dominated by token handling, and this sample was about 4% slower than the scalar prototype.

### Linux (Fedora 45 under WSL 2, same machine)

`python3 benchmarks/run.py`, best of 3, 2026-11-22. Without `-O`: `loop` 442 ms (2.64x gcc), `mandel` 796 (3.84x), `fib` 145 (3.32x), `collatz` 666 (2.06x), `sieve` 343 (1.70x), `quicksort` 454 (1.44x). With `-O`: `loop` 222 (1.32x), `quicksort` 356 (1.13x), `sieve` 227 (1.15x), `collatz` 548 (1.68x), `mandel` 482 (2.35x), `fib` 158 (3.59x). Sizes: razum 0.9–2.0 KB, gcc 12.6 KB. Hello world: 728 bytes.

### http on Linux

`benchmarks/http_load.c` (epoll load generator, `gcc -O2 -o http_load benchmarks/http_load.c -lpthread`, then `./http_load PORT CONNECTIONS PIPELINE SECONDS THREADS`). Server `rzc -O`, 2 workers pinned with `taskset -c 0,1`, load on `taskset -c 2-15`, `GET /` with a 13-byte body, Fedora 45 in WSL 2, 2026-11-22: 1,109,000 req/s with 16 pipelined requests (64 connections), 102,000 req/s one request per round trip (64 connections), 82,000 req/s over 1,000 connections, 0 failures, 364 KB resident. The Windows numbers above came from a different (Go) load tool, so compare trends, not exact values.

## Building

- GCC (MSYS2 UCRT64, from Git Bash): `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` then `cmake --build build`. Builds with `-Wall -Wextra -Wpedantic -Werror` and no warnings.
- MSVC: in a shell set up by `"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"`, `cmake -S . -B build/msvc -G Ninja -DCMAKE_C_COMPILER=cl -DCMAKE_BUILD_TYPE=Release` then `cmake --build build/msvc`. Builds with `/W4 /WX` and no warnings. Verified 2026-11-21 by compiling `tests/programs/all_types.rz` and `tests/modules/app.rz` with the result and comparing their output.
- MSVC has no variable-length arrays; use `arena_alloc` for arrays whose size is only known at run time.
- MSVC reports code after a `noreturn` call (`compile_error`) as unreachable (C4702) and fails the build; GCC does not. Write error checks as guards: `if (bad) compile_error(...); return value;`.
- CMake globs `libraries/**/*.rz` and `*.asm` with `CONFIGURE_DEPENDS`, so a new library file is embedded by the next `cmake --build build` without re-running configure.
- The installer: `cmake --build build --target razum-setup` (also `build/msvc`). The version is in two places: `project(razum VERSION ...)` in `CMakeLists.txt` and `version` in `editors/vscode/package.json`.
- `OBJECT_DEPENDS` does not expand generator expressions (`$<TARGET_FILE:rzc>` became a literal missing file); `setup.rc` depends on the `.vsix`, which depends on `rzc`.
- MinGW does not declare `ExitProcess` as `noreturn`; a `noreturn` function must end in `exit()`.
- The VS Code package: `editors/vscode/package.ps1` copies `build/rzc.exe`, so rebuild `rzc` first. `rzc.exe` needs only the system UCRT DLLs (`objdump -p build/rzc.exe | grep "DLL Name"`).

## Gotchas

- In WSL, `code` is the Windows VS Code (its path starts with `/mnt/`), so `razum-setup.sh` does not install the Linux extension there; install it from Windows or inside a Remote-WSL window.
- The VS Code extension finds its bundled compiler as `bin/rzc.exe` on Windows and `bin/rzc` elsewhere (`compilerPath` in `extension.js`).
- `crypt` P-256 scalar multiplication branches on key bits (double and add). A constant-time ladder is a separate follow-up.
- Linux `crypt` key expansion and scalar AES still do key-dependent table lookups. The scalar GHASH uses a bit-by-bit loop. Run the existing known-answer tests and `tests/programs/gcm_hardware_linux.rz` when changing [the accelerated or scalar paths](#linux-aes-gcm-phase-timings).
- The Linux SIGINT handler (`libraries/http/server/assembly/linux/signal.asm`) calls `http_console_stop` through the global function value `http_interrupt_handler`: a function or global used only from assembly is not marked as used and would be left out of the program.
- razum cannot dereference a cast directly (`*(uint *)&bytes[0]` is a parse error). Write `uint *at = (uint *)&bytes[0];` then `*at`.
- Linux `http`: an epoll event whose data is below 8 is a listener index; anything else is a `HttpConnection *`. The `epoll_event` structure is 12 bytes and packed on x86-64, so events are read from a byte array.
- After moving or adding library files, reconfigure CMake (`cmake -S . -B build`) if the embedded copy looks stale; the library glob is re-evaluated at configure time.
- `print_line` and `print_error` used to write `\r\n` on Linux too (the test runner strips `\r`, so the suites did not notice); they now have a `linux/` version. Search for a literal backslash-r with `grep -rnF '\r' libraries` (plain `grep '\r'` matches the letter r).
- A Linux program stores its environment pointer only if it links `rz@environment` (`libraries/program/assembly/linux/environment.asm`, used by `process`); `emit_linux_entry_point` adds the three instructions then.
- `process` ignores `SIGPIPE` on Linux before writing a child's input, so a child that exits early cannot kill the parent.
- On Linux, `threads` keeps the Windows lock function names on purpose: the shared channel code then compiles to the same direct kernel32 calls on Windows as before. Futexes work on 32-bit words, so the Linux lock and condition use the low half of the `long` fields. A new thread starts after the `syscall` instruction of `rz@thread_clone` on its own stack; the kernel writes its id into `ThreadControl.id` and clears it at exit (`CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID`), which is what `thread_join` waits for, so that memory must stay valid until then (hence a detached thread's control block and stack are freed later by `thread_reap`, called from `thread_start`, once the id is 0).
- WSL's minimal Fedora had no `awk`; `tests/test_parallel_print.sh` needs it (`sudo dnf install gawk`).
- `import windows;` and `import linux;` skip every folder search and load the built-in library: otherwise a library's own `windows/` platform folder would be loaded in its place (it happened in `filesystem`). A module checked on its own inside a `windows/` or `linux/` folder also loads its parent folder.
- Linux library code calls the kernel through `system_call` from `libraries/linux/`. Arguments are `long`; pass text as `(long)path` and buffers as `(long)&buffer[0]`. A negative result is `-errno`; `EINTR` (`-4`) means try again.
- Signed 64-bit division checks for a divisor of -1 first (`emit_divide`), because `idiv` faults on `LONG_MIN / -1`; the result wraps to `LONG_MIN` with remainder 0, as constant folding does. 32-bit division runs in 64 bits and needs no check.
- Constant folding keeps `float` arithmetic in double precision and rounds only when the value is stored or cast, like the generated code; `&&` and `||` fold their right side only when the left side does not decide (`fold_constant.c`).
- A normal build writes its assembly to `<output>.rzc-<process id>.asm` and deletes it after NASM succeeds, so an existing `name.asm` next to the source is never touched. If NASM fails, that file stays for the bug report. `--asm` and `-S` write `name.asm` on purpose.
- On Linux, `run_process` learns that `execv` failed through a close-on-exec pipe, so a program's own exit code 127 is passed on unchanged (`tests/test_cli.sh`).
- `printf(value)` shares `print`'s type dispatch. Its text path writes each call under the same stdout lock and routes `%n` through the platform newline routine. Use `write_string` for literal percent sequences.

- JSON tokens borrow the source bytes. `JsonToken.next` skips the whole subtree, and failed parses leave partial tokens that callers should discard.
- JSON string scanning uses SSE2 only after a short scalar prefix and reads only complete 16-byte chunks within the caller’s declared length.
- Use `time_monotonic_ns` for elapsed intervals and `time_unix_us` for UTC timestamps. The latter follows system clock adjustments.

- WSL can build this checkout in place, but give it its own build folder: `RZ_BUILD=$HOME/razum-build ./build.sh --test`. Sharing `build/` with Windows mixes two CMake caches. From Git Bash, call WSL as `MSYS_NO_PATHCONV=1 wsl.exe -e bash /mnt/c/...script.sh`, or Git Bash rewrites the `/mnt/...` path.
- Python's `Path.read_text` turns a lone carriage return into a newline. Two test scripts once held a raw CR inside `tr -d '...'` and broke that way; they now write `'\r'`. Edit files that may contain raw control characters as bytes.
- A library file below a folder named `windows` or `linux` exists only for that system (`fits_target` in `driver/load_program.c`). Do not name a folder in `libraries/` like that unless it is meant to be system-specific.
- Linux line ends are `\n` only: `rz@newline` is one byte there and two (`\r\n`) on Windows. Code that writes a line end itself must call `rz@write_newline`, not write `13, 10`.
- Files in `libraries/array/each/` and `libraries/array/integers/` are templates: `rzc` instantiates them (`driver/instantiate_template.c`), so `T` there is not a type of its own. Every name ending in `_T` gets the type appended, and `T` inside a string literal is left alone. The main file, files from `import`, and the siblings of a module checked on its own all go through `parse_source` in `load_program.c`, which is where instantiation happens.
- The output lock is left out (`rz@lock_output` is a bare `ret`) when `analysis/runs_on_one_thread.c` finds no way for a second thread to run razum code. A new way to start threads that does not pass a razum function as a value (for example a new kind of entry point) must be added there, or `print` loses its thread safety.
- In `codegen/`, an operand that is already in `rcx` was once detected by its text starting with `r`. With `-O` operands can be `rbx` or `r12`, so such checks must compare with `"rcx"` exactly (`emit_int.c`). A floating-point operand can be an `xmm` register, so a path that moves it through `rax` needs `movsd` instead (`emit_call.c`, stack arguments).
- After changing the extension's `version` in `editors/vscode/package.json`, CMake must reconfigure to pick up the new `.vsix` name; `installer/CMakeLists.txt` marks `package.json` as a configure dependency for this. Before that fix, a version bump left `razum-setup.exe` holding the previous `rzc`.
- A project folder or file named like a built-in library (`math/`, `text.rz`) hides that library for every file in the project, because the main file's folder and `-I` folders are searched before the built-in libraries. `import math;` then loads your `math/`, not razum's.
- A `program gui;` program has no console: `print` output is lost unless it is redirected, and `rzc --run` cannot show it. Build with `--console` while debugging.
- Windows refuses to load razum's one-section, 4-byte-aligned image when the section's size in memory is larger than its size in the file ("Exec format error" from bash). So there is no zero-filled memory past the end of the file; the output buffer comes from `VirtualAlloc` at the first write.
- File Explorer takes a file's icon from the user's "always open with" choice (`HKCU\...\Explorer\FileExts\.rz\UserChoice`, here `Applications\Code.exe`, which showed VS Code's generic icon), not from the extension's default ProgID. The installer deletes `UserChoice` and `UserChoiceLatest`; if Windows refuses, it tells the user how to choose again.
- .NET Framework's `System.Drawing.Icon` cannot read PNG-compressed `.ico` frames ("Requested range extends past the end of the array"); Windows itself can. Check `rz.ico` by extracting its PNG frames instead.
- Library functions are checked after the program's own, so struct parameter types must be resolved with the signatures (`check_signatures`), not per function body; otherwise a library's `File` and the caller's `File` compare unequal.
- A module checked with `--check` loads only its own folder's `.rz` files; library files therefore `import` other parts of their library (`import text.characters;`), which also documents their dependencies. `tests/test_module_checks.sh` checks every library and module file this way.
- A library file must not `import` its own library (`import gui;` inside `libraries/gui/`): `rzc --check` loads the file from disk and the embedded copy of the same file, and reports every function twice. Import the sibling folders instead (`import gui.state;`).
- Tests that drive `gui` through `gui_window_procedure(0, ...)` must send one click per frame and call `gui_frame()` between builds: the mouse position is shared, and layout continues from the previous build otherwise.
- `PrintWindow(window, dc, 2)` (`PW_RENDERFULLCONTENT`) captures a `gui` window even when it is covered; the capture includes the invisible resize border.
- Python's `json.dumps` writes a `\b` it read from a normal string literal as a backspace escape; edit `razum.tmLanguage.json` with `chr(92)` or by hand.
- A type name in a declaration is parsed before enums are known, so enum names first become struct-name shapes and `resolve_shape` turns them into `int` shapes with `enumeration` set; every cached `type` (globals, parameters, return types, declarations, casts) is re-read from the shape after resolving.
- `(Name)` is read as a cast when the next token starts a value (a name, number, character, string, keyword, `(`, `!`, `~`), so `(Color)(x + 1)` works; `(f)(x)` therefore needs `f(x)`.
- `GetQueuedCompletionStatusEx` can hand one worker several stop packets in one batch; a worker that returned at the first lost the others and the next worker waited forever. Workers now finish the batch, re-post the extra stop packets and then return.
- Literal lengths passed to `http_out` are counted by hand: `"\r\nServer: razum\r\nDate: "` is 23 bytes, not 25. The two extra bytes put a `\0` into the `Date` header, which curl rejects ("Nul byte in header"). `tests/programs/http_server.rz` now checks the exact `Date` layout instead of hiding that line.
- An SChannel server cannot use a `.pfx` key imported with `PKCS12_NO_PERSIST_KEY`: `AcquireCredentialsHandleW` returns `0x8009030E` (`SEC_E_NO_CREDENTIALS`), because the handshake runs in LSASS. `PKCS12_NAMED_NO_PERSIST_KEY` (0x20000) makes the import itself fail here. Importing with `CRYPT_USER_KEYSET | PKCS12_ALWAYS_CNG_KSP`, acquiring the credentials and then `NCryptDeleteKey` works: later handshakes still succeed and `%APPDATA%\Microsoft\Crypto\Keys` is unchanged (`tests/test_https.sh` checks both).
- curl's Schannel backend reports "remote party requests renegotiation" on every TLS 1.3 connection: that is the server's post-handshake session ticket, not an error.
- `process_run` stops a program that exceeds its output limit; if the program already ended on its own, its own exit code is reported instead of 1. Tests check only `truncated` and the output there (an earlier exact exit-code check failed 2 times in 24 parallel runs).
- The website's server entry is `learn.rz`, not `server.rz`: a file named `server.rz` next to a `server/` folder imports itself.
- `website/start.ps1` prefers `build\rzc.exe` over the installed `rzc`, because the installed one may predate libraries the server needs.
- `setjmp` must stand alone in an `if` condition (C17 7.13.1.1); `diagnostic_try` uses `if (setjmp(here) == 0)` and only reads locals that are not changed after it.
- Bash heredocs in this environment turn `\n` inside C string literals into real line breaks; write C files with the Write tool or Python.
- Winsock's exports are plain words (`send`, `recv`, `bind`, `listen`, `connect`, `shutdown`). Declared in `windows`, they broke `tests/programs/gui_offscreen.rz`, which has its own `send`; they live in the separate `winsock` library.
- Listening on anything but loopback makes Windows Firewall ask the user once per program; automated tests use `127.0.0.1` and `::1` only.
- A program file named like a library shadows it for its own imports: `tests/programs/threads.rz` with `import threads;` imported itself (local modules come first), which showed as "unknown type 'Mutex'". Test and example files must not share a library's name.
- The Docs tab fetches `/docs/language.md` and `/docs/libraries.md`. Register `/docs/` before the `/` public route because routes are tried in order. The viewer splits Markdown into topics at `##` headings outside code fences. Search can link to headings and first-column function signatures. A heading or signature rename can change its URL. `tests/test_website.sh` checks the reference against the library folder inventory and tests links and search.
- The learning server also starts from a binary built into `build/` when run from the repository root. A standalone copy needs `public/` beside it and uses a sibling `docs/` for the reference. Missing reference files leave the course and playground available. If `razum-learn.exe` is still running, Windows keeps the executable open and NASM cannot overwrite it. `website/start.ps1` checks for the running copy before compiling.
- `print` from several threads: each call is atomic (`rz@lock_output`), separate calls interleave. `tests/test_parallel_print.sh` checks 8 threads × 5,000 whole lines.
- A struct may point to its own kind (`Node *next`): pointer fields resolve the struct's name without laying it out, so the layout never recurses.
- `(x).field` and `(x)[i]` need postfix parsing after parentheses; `(*link).value` exposed that it was missing.
- NASM's `bin` format allows `&` on a label only after subtracting another label of the same section: relocation entries use `(pe@reloc@N - pe@dos) & 0xFFF`.
- A razum DLL was loaded at `0x7FFF54220000` instead of its preferred `0x180000000` (ASLR) and its string data still worked, so the relocations are exercised by `tests/test_dll.sh`.
- CNG algorithm pseudo-handles (`BCRYPT_SHA256_ALG_HANDLE` = `0x41`, `BCRYPT_HMAC_SHA256_ALG_HANDLE` = `0xb1`, `BCRYPT_AES_GCM_ALG_HANDLE` = `0x1e1`, ...; values from `bcrypt.h` in SDK 10.0.28000) work with `BCryptCreateHash`, `BCryptHash`, `BCryptDeriveKeyPBKDF2` and `BCryptGenerateSymmetricKey` on Windows 10+, with a null object buffer (CNG allocates it). They avoid `BCryptOpenAlgorithmProvider`, which takes UTF-16 names razum cannot write as literals.
- `BCryptEncrypt` and `BCryptDecrypt` update the IV buffer in CBC mode, so `aes_cbc_encrypt` copies the caller's IV first. `BCryptDecrypt` in GCM mode returns `STATUS_AUTH_TAG_MISMATCH` (`0xC000A002`) on a changed tag; `aes_gcm_decrypt` then zeroes `target` in case plaintext was written.
- Every CNG length is a 32-bit `ULONG`: `hash_update` and `secure_bytes` feed 1 GiB at a time, the ciphers return `-1` above 4 GiB.
- A `char` array cannot be cast to `byte *` ("an array cannot be converted to pointer"); it must go through `char *` first, which `as_bytes` does. Outputs in `crypt` are arrays, so a heap buffer from `new byte[n]` cannot be a target; use a global array for large data (a 16 MiB global worked in the benchmark).
- `crypt` naming: `secure_int` and `secure_bytes` instead of `random_*`, because `math.random` already owns `random_int` and both libraries may be imported together.
- `crypt` speed limits: AES-CTR runs at a tenth of GCM because the counter blocks and the xor are built byte by byte in razum; building them 8 bytes at a time or in an `asm` block is the upgrade when CTR is needed above about 300 MB/s. Base85 decoding was 23 MB/s with a linear alphabet search and 77 MB/s with range checks.
- MSVC links a razum DLL through `lib /def:x.def /machine:x64 /out:x.lib`; GCC (MinGW) links the `.dll` directly.
- The PE export directory includes mutable data alongside functions. The generated `.def` marks those exports `DATA`, and the header declares them with `dllimport`. `DllMain` accepts Windows load and thread notifications on x86-64. PE unwind metadata and native static TLS are separate compiler and loader work.
- CMake with MinGW names a shared library `libname.dll`; razum's `extern "name"` looks for `name.dll`, so C DLLs used from razum set `PREFIX ""`.
- An imported target's `INTERFACE_INCLUDE_DIRECTORIES` must exist at configure time; `add_razum_library` creates the output folder before the header exists.
- `&function` must not go through the address-of-a-place path: a function value is already an address (the first callbacks test crashed `rzc` there).
- `link_units` scans only the text it is given: references made by `rz@entry` (`rz@arguments`) are added to the scanned text separately.
- Buffered output is lost when a program ends without `rz@entry`'s flush: an `asm` block calling `ExitProcess` directly, or a crash.
- Windows PowerShell 5.1 adds a UTF-8 byte order mark when piping text into a program, and editors can save files with one; the lexer skips `EF BB BF` at the start of a source.
- `razum-setup.exe` is a GUI-subsystem program: it writes `--quiet` output to an inherited standard output or, failing that, to the parent console (`AttachConsole`), and shells only wait for it when its output is piped.
- `.gitignore` ignores `*.asm` (generated output) with exceptions for `libraries/**/*.asm` and `tests/**/*.asm`. A new folder of hand-written `.asm` elsewhere needs its own exception, or `git add -A` silently skips it.
- Windows PowerShell 5.1 (.NET Framework) `ZipFile.CreateFromDirectory` writes entry names with `\`; VS Code rejects such a `.vsix`. `package.ps1` writes each entry itself with `/`.
- `CreateProcessA` with a NULL application name does not resolve a relative path written with `/`. `run_process` passes the full path from `GetFullPathNameA` as the application name.
- A child process only writes into the parent's redirected stdout (a pipe or file) when handles are inherited: `run_process` sets `STARTF_USESTDHANDLES` and `bInheritHandles = TRUE`.
- NASM accepts `$` before any name to force it to be a label, and `@` inside names; that is the basis of the naming scheme in [docs/codegen.md](docs/codegen.md).
- NASM local labels (`.x`) belong to the last label without a dot. In a function that is `$name`, so locals, compiler labels and labels in `asm` blocks are per function. An `asm` label with a variable's name (`.count:` next to variable `count`) is a duplicate definition.
- Never put a raw CR byte in a test `.sh` file: Git Bash drops CR when reading scripts. Use `${text//$'\r'/}`. Generating such scripts through nested shell and Python quoting mangled the escapes several times; write them with an editor instead.
- Windows console output from the programs ends lines with CR LF; the test scripts strip CR before comparing.
- Text is UTF-8 everywhere. On a console, `rz@write_to` converts it to UTF-16 and calls `WriteConsoleW` (in chunks of at most 16 KiB that end on a character boundary); files and pipes get the exact bytes. It deliberately does not call `SetConsoleOutputCP(65001)`: the code page belongs to the console, so it would stay changed in the user's shell after the program ends, or after a crash. A character split exactly where the 64 KiB output buffer fills can still show as `�` on a console.
- Console input uses `ReadConsoleW` and converts each character to UTF-8 (only whole characters are kept when the array is full); `ReadFile` on a console returns the OEM code page, which garbled every non-ASCII letter.
- `tests/test_unicode.sh` gets a console of its own (`FreeConsole`, `AllocConsole`, hidden at once), prints, reads the screen back with `ReadConsoleOutputCharacterW` and types into it with `WriteConsoleInputW`, because a pipe never takes the console path. A window may flash for a moment. The same check with plain `WriteFile` fails, so the test does catch the old behaviour.
- The `text` library must not import `windows`: `import text;` would then bring every Windows declaration into the program, and programs with their own `extern GetSystemMetrics` failed. `to_utf16` and `from_utf16` are therefore plain razum, not `MultiByteToWideChar`.
- `--stdin` errors name the path given on the command line, with `/` separators. The editor extension compares paths after `path.resolve`, case-insensitively.

- Linux GUI uses a local Wayland socket or X11 display. WSLg provides both on the same Windows desktop. Unset `DISPLAY` to verify native Wayland and unset `WAYLAND_DISPLAY` to exercise the X11 client. The smoke test asserts the backend actually selected.
- Wayland buffers stay busy until the compositor releases them. The Linux GUI draws into its own pixel buffer and copies into one of two available `wl_shm` buffers. Resizing allocates fresh buffers, so the compositor can finish reading old content safely.
- The embed script sorts GUI Windows files by their former paths so moving them into platform folders keeps the same Windows function order and executable bytes.

## Known limits, and when to change them

| Linux GUI limit | Effect | Upgrade when |
|---|---|---|
| Latin-1 bitmap font, single-pixel layout units | Other code points use a fallback glyph, and high-DPI output may be scaled by the compositor | Text shaping, system fonts, or per-output scale are needed |
| Local X11 TrueColor display on the default screen | The X11 client expects 32-bit pixels with standard RGB masks. It connects over a Unix socket | Remote X11, unusual visuals, or multiple X screens are needed |
| Core Wayland pointer and keyboard protocol | Basic keys and modifiers work. Input methods and key repeat need more protocol work | Text entry needs composition or held-key repeat |

| Limit | Why it is fine now | Change when |
|---|---|---|
| No pointer arithmetic except `p[i]` and `&p[i]`; no null or use-after-free checks; no garbage collector | Same as C; mistakes show as access violations | A debug mode that checks pointers is wanted |
| Array indexes are not checked at run time; a wrong index reads or writes other memory | Each access stays one instruction; literal indexes are checked when compiling | A debug mode that checks indexes is wanted |
| Global arrays and structs are stored in the `.exe` (zeros included); the one-section layout cannot have zero-filled memory | Small tables cost little | Large global buffers; allocate them at start-up instead |
| Local arrays live on the 8 MiB stack | A 4 MB local array works (tested) | Larger data; same fix as above |
| Functions cannot return arrays or structs | Passing one in by reference does the same job | Returning small structs by value is wanted |
| `extern` functions cannot take variable argument lists | Most of the Windows API works, callbacks included | `wsprintf`-style functions are needed |
| Paths in `filesystem` are limited to 1,023 UTF-16 units and do not add the `\\?\` long-path prefix | Ordinary paths are far shorter | Paths over 260 characters on systems without long-path support |
| Command-line splitting knows only spaces and double quotes; no `\"` escapes (the text itself is read with `GetCommandLineW` and converted to UTF-8) | Paths and words in any language work | Arguments containing quote characters are needed; add the full Windows quoting rules |
| `read_line` cuts lines longer than its `char` array; the rest of the line is skipped | Callers choose the array size | Arbitrarily long lines are needed; add a growing buffer once there is heap memory |
| Files are text only: no binary reads, seeking or file size | Line-based tools work | Binary formats are needed |
| One function per name; no overloading | Library functions take `long` or `double`, which accept every smaller type | Separate `int` and `long` versions are wanted under one name |
| `(float)` of an integer beyond 2^53 rounds twice (via double) | Such values are rare; error at most one float step | Exact float conversions of huge integers matter |
| `float` expressions keep double precision until stored | Like C with `FLT_EVAL_METHOD` 1; results can differ from strict single precision in the last digit | Bit-exact single precision is required |
| Floating to integer casts of out-of-range values give the hardware's value (`0x8000...`) | Same as C (undefined there) | Checked conversions are wanted |
| Every value lives in memory, no register allocation | Output stays readable line by line | Speed matters more than readability; add an optimizing mode, keep the plain one |
| Symbol and function lookup is linear | Programs have tens to hundreds of names | Thousands of names make compilation noticeably slow |
| Arena memory is never freed | One compilation per process | `rzc` compiles many files in one process |
| After a syntax error, `rzc` skips to the brace that closes the broken item (or its `;`), falling back to the next line that starts in column 1 when braces do not balance, and does not type-check | Unindented code (common for beginners) gives only the real errors | Errors inside a block that also loses a brace may still cascade |
| Hover and Go to Definition treat a local as visible from its declaration to the end of its function, not its block | Shadowing inside one function is rare | Precise block scopes: print block spans in `--symbols` |
| Errors are printed in checking order (structs, globals, signatures, then function bodies), not sorted by line | Editors sort them; each has its position | Long command-line output where order matters |
| Modules share one namespace | Small programs; clashes are reported with both locations | Libraries grow enough that names collide |
| One RWX section, no ASLR, fixed base 0x140000000 | Smallest files; same layout as the asm project | Programs that handle untrusted input; add a paged layout like the asm project's `paged` option |
| Integer overflow wraps; integer division by zero ends the program with 0xC0000094 | Same as the hardware | Checked arithmetic is wanted |
| Default and `-O` use the x86-64 baseline (SSE2, x87); `-march=znver3` permits BMI2, scalar AVX and AVX2 | Portable builds retain their CPU requirements | Add further instruction selection only with a measured gain |
| `gui` has no scrolling, no clipboard or text selection, no keyboard focus moves (Tab), and one window per process | Settings panels and tools fit in one window | Long lists or forms: add a scroll container; text editing beyond one field: add selection and the clipboard |
| `gui` keeps element state by call order, up to 1,024 elements, 8,192 drawing commands and 65,536 UTF-16 characters per frame; past that, animations are shared and the extra commands or text are dropped | A full settings window uses 54 commands | Dynamic lists that insert elements in the middle: key state by an id the program passes |
| While the user drags the window border, `gui` cannot run the program's frame (Windows runs its own sizing loop), so the content stretches only after the drag | The frame is redrawn as soon as the drag ends | Smooth live resizing is wanted: render from `WM_SIZE` with the last frame's commands |
| `http` HTTPS: one certificate per address (no SNI selection), no client certificates, TLS 1.3 key updates from the client are passed back into the handshake but untested | One site per address is the common case | Several domains on one address: add SNI and a certificate table |
| `http`: no chunked request bodies (501), no HTTP/2, no compression, no `Range`, paths are not percent-decoded, at most 64 routes and 8 addresses, static files are read whole into memory per request | Typical API and small-site traffic is covered | Uploads from clients that stream, large downloads (add `TransmitFile`), or HTTP/2 |
| Channels carry `long` only and `channel_select` only receives | Pointers cast to `long` carry any data; sending usually happens in a loop of its own | Typed channels need generics; a select that also sends needs a waiter list per channel |
| Windows only | The output is a Windows PE file | Another OS is needed: new `platform/` folder for `rzc`, new header and system calls for the output |
