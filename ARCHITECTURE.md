# Architecture

## Pipeline

```
file.rz --read_source--> text
        --load_program--> Program + asm units      (tokenize + parse_file for the file and every import)
        --check_program--> types, symbols, call targets
        --mark_used_functions--> which functions reach the .exe
        --emit_program--> NASM text                (functions, then link_units adds rz@ units and iat@ imports)
        --write_file--> file.asm --nasm -f bin--> file.exe
```

`main.c` calls `read_source`, `compile_source` (load, check, mark, emit), writes the `.asm` and runs NASM, in the order the command-line flags ask for. Each stage is one folder under `src/`. A stage only reads what the stage before it produced and never calls a later stage.

## Dependency direction

```
main ──> driver ──> lexing ──> common
     │          ├─> parsing ──> lexing, syntax, common
     │          ├─> checking ──> analysis, syntax, common
     │          ├─> analysis ──> syntax
     │          ├─> codegen ──> linking, analysis, syntax, common
     │          ├─> linking ──> common
     │          ├─> embedded (embedded_files.c, generated from libraries/ at build time)
     │          └─> platform ──> common
     └─> platform
syntax ──> common
```

There are no cycles. Only `driver/`, `main.c` and `platform/` touch files and processes; every other folder is pure logic on memory. Only `platform/windows/` calls the Windows API and only `platform/linux/` calls POSIX; CMake compiles just the one for the host. Which kind of program `rzc` writes is `host_target()` from that folder ([docs/platforms.md](docs/platforms.md)).

## Folders and files

Each `.c` file holds one public function (plus `static` helpers it alone uses), or the few accessors of one small type. Each folder has one header that declares its public functions.

### `src/common/`: shared building blocks

| File | Job |
|---|---|
| `arena.h`, `arena_alloc.c` | Bump allocator: zeroed memory from 64 KiB chunks, never freed |
| `arena_copy_text.c` | Copy text into the arena with a closing 0 |
| `diagnostics.h`, `compile_error.c`, `diagnostic_try.c` | Report `file:line:column: error: ...`, count it, and jump back to the nearest recovery point (`diagnostic_try`); exit 1 when there is none or after 100 errors; `diagnostic_source_path` names the file being checked |
| `fatal_error.c` | Report a usage or system problem and exit 2 |
| `text_buffer.h`, `text_append_bytes.c` | Growable text buffer: append raw bytes |
| `text_append.c` | Append a C string |
| `text_append_formatted.c` | Append `printf`-style output |
| `text_line.c` | Append a formatted line |
| `list.h`, `list_push.c` | Growable list of pointers and `LIST_AT` to read it |
| `files.h`, `read_file.c`, `write_file.c` | Whole-file read and write |

### `src/lexing/`: text to tokens

| File | Job |
|---|---|
| `token.h` | `Token` and `TokenArray` |
| `source_cursor.h`, `source_cursor.c` | Position, line and column while reading |
| `vocabulary.h`, `vocabulary.c` | Keywords, type names, operators, escapes, character classes |
| `lexing.h` | Lexer functions |
| `tokenize.c` | Read a whole file into tokens |
| `skip_trivia.c` | Skip whitespace and comments |
| `scan_word.c` | Names and keywords; hands `asm` to `scan_asm_block` |
| `scan_asm_block.c` | The raw text of `asm { ... }` up to the matching `}`, ignoring braces in `;` comments and quotes |
| `scan_number.c` | Decimal, hexadecimal and floating literals, up to 64 bits |
| `scan_string.c`, `scan_char.c` | `"text"` and `'c'` with escapes |
| `scan_operator.c` | Operators and punctuation, longest match first |
| `token_at.c` | Make a token from the text just read |

### `src/syntax/`: the syntax tree and the type system

| File | Job |
|---|---|
| `syntax.h` | Every node type (`Expression`, `Statement`, `Function`, `GlobalVariable`, `Import`, `Program`, `Symbol`), `ValueType`, `TypeInfo` and `Operator` |
| `new_expression.c`, `new_statement.c` | Allocate a node |
| `type_info.c` | One table with each type's name, size, kind and signedness; lookup by name |
| `widens.c` | Integer/floating tests and which type converts automatically to which |
| `shape.c` | `Shape`: a scalar type (an `int` may carry its enum), an array (element and count) or a struct; size, alignment, equality, and `int[3]`-style names for messages |
| `operator_text.c` | Operator spelling for messages |

### `src/parsing/`: tokens to syntax tree (recursive descent)

| File | Job |
|---|---|
| `token_stream.h`, `token_stream.c` | Look at, accept and expect tokens |
| `describe_token.c` | `'x'` or "the end of the file" for messages |
| `parsing.h` | Parser functions |
| `parse_file.c` | Imports (dotted names), structs, enums, `const` and plain globals, `extern` declarations and functions of one file; marks library files |
| `parse_block.c`, `parse_statement.c` | `{ }` blocks; choose the statement kind, including `asm` |
| `parse_declaration.c` | `type name = value` |
| `parse_simple_statement.c` | Assignments, `++`/`--`, call statements |
| `parse_control_statement.c` | `if`, `while`, `for`, `return`, `break`, `continue` |
| `parse_expression.c` | Precedence table, unary operators, casts, literals, names, calls, and `[i]` / `.field` after a name |
| `parse_arguments.c` | `( a, b, ... )` |
| `parse_type.c` | A built-in type name (casts, return types) |
| `parse_shape.c` | Is this a declaration; a type that may be a struct or enum name; `[N]` array suffixes, where `N` may be a constant expression |
| `parse_initializer.c` | An initial value: an expression or a nested `{ }` list |
| `parse_struct.c` | `struct Name { fields }` |
| `parse_enum.c` | `enum Name { A, B = 5, C }` |

### `src/checking/`: types, names and rules

| File | Job |
|---|---|
| `checking.h` | `Checker`, `Scopes` and checker functions |
| `check_statement_safely.c` | Checks one statement inside a recovery point and restores scopes and loop depth if it fails |
| `check_program.c` | Enums, global symbols, structs, globals, signatures, every function; then drops `const` globals, which need no storage |
| `check_enums.c`, `find_enum.c` | Duplicate enums and members, member values that fit an `int`, the `int` shape that marks each enum |
| `declare_globals.c` | A symbol for every global before anything is resolved, so constants can be found in any order |
| `fold_constant.c` | Compute a checked expression while compiling, with the run-time rules (wrapping, signedness, shift counts); `NULL` if it is not constant |
| `constant_value.c` | Check and fold a `const`'s value into the literal that replaces every use |
| `resolve_global_constant.c` | Fold a global `const` on first use, reporting constants that depend on themselves |
| `resolve_count.c` | An array size given as a constant expression |
| `check_structs.c` | Duplicate struct names; lay out every struct |
| `resolve_shape.c` | Find the struct or enum behind a name; constant array sizes; field offsets, size and alignment; a struct that contains itself |
| `check_access.c` | `a[i]` (integer index, constant range check) and `p.field` / `a.length` |
| `check_pointer.c` | `&x`, `*p`, `new`, `sizeof`, and which pointers compare |
| `check_switch.c` | `switch`: value type, constant cases (folded), enum members for an enum switch, duplicates, one `default`, a scope per case |
| `check_initial_value.c` | `{ }` lists, text for `char` arrays, and whole-array or whole-struct copies |
| `check_globals.c` | Global names, constants, and initial values known when compiling |
| `check_signatures.c` | Duplicate functions (including clashes with library functions), parameters, `main` |
| `check_dll_main.c` | Check the optional Windows DLL lifecycle hook against the loader's argument types |
| `check_array_method.c` | `values.sort()` and the other array methods become calls to `array_<method>_<type>(values, ...)` |
| `rewrite_end_element.c` | `values.first` and `values.last` become `values[0]` and `values[values.length - 1]` |
| `text_as_char.c` | Turns a one-byte text literal into a `char` literal where a `char` is expected (stores, arguments, returns, `case` values, operations with an integer) |
| `check_prototypes.c` | Moves C-style declarations without a body out of the function list, then checks each against its definition |
| `check_function.c` | Parameters, body, "every path returns" (or ends in `asm`) |
| `check_statement.c` | Each statement kind, loops, compound assignments, `++`/`--`, `return` |
| `check_store.c` | A value stored into a typed place, with range messages for literals |
| `can_store.c` | Widening or a literal that fits the target |
| `operation_type.c` | The common type of a binary operation (promotion, literals, signedness) |
| `check_expression.c` | Each expression kind and its type, conditions, casts |
| `check_call.c`, `find_function.c` | Calls and arguments (arrays and structs by reference); `print` picks `print_long`, `print_float`, ... by type |
| `scopes_push_pop.c`, `scopes_declare.c`, `scopes_lookup.c` | Block scopes and the `.name@N` label for each variable |

### `src/analysis/`

| File | Job |
|---|---|
| `analysis.h` | Analysis functions |
| `always_returns.c` | Does a statement end in `return` on every path |
| `mark_used_functions.c` | Mark reachable functions, inline eligible value calls and count local references weighted by loop depth |
| `inline_small_call.c` | Substitute safe scalar arguments into a short checked return expression while retaining the original function's reachability |

### `src/codegen/`: syntax tree to NASM text

| File | Job |
|---|---|
| `codegen.h` | `ProgramContext`, `FunctionContext`, `Operand` and emitter functions |
| `emit_program.c` | Whole file: header, entry, used functions, linked units, data, imports |
| `emit_pe_header.c`, `emit_entry_point.c`, `emit_data_section.c` | One file part each |
| `emit_dll_entry_point.c` | Forward loader notifications to the optional `DllMain` and flush output on process detach |
| `emit_export_section.c` | The export directory of a DLL |
| `emit_relocations.c` | `pe@reloc@N` labels for absolute addresses and the relocation directory of a DLL |
| `emit_def_file.c`, `emit_c_header.c` | The `.def` export list and the C/C++ header for the DLL's exported functions and variables |
| `emit_coff_header.c`, `emit_elf_object_header.c` | Emit native object sections and exports. ELF exports adapt System V arguments to the internal calling convention. |
| `function_label_prefix.c` | Keep internal ELF function labels distinct from public native ABI entry points. |
| `emit_import_section.c` | Import tables grouped by DLL from `iat@Name` / `iat@dll@Name` references |
| `emit_function.c` | Frame layout (sized locals, argument positions, save slots for registers used with `-O`), parameter spills, prologue with stack probe, register saves and parameter loads |
| `emit_statement.c` | Statement dispatch, `return`, `break`, `continue`, epilogue |
| `emit_assignment.c` | Declarations, assignments, compound assignments, `++`/`--` for every type |
| `emit_control.c` | `if`, `while`, `do`, `for`, `switch`, and the integration of loop optimization with the scalar code |
| `loop_invariant_declaration.c` | Check whether the first declaration of a loop can run once, after the initial loop test |
| `emit_int.c`, `emit_double.c` | Expressions into `rax` / `xmm0` |
| `emit_wide_tree.c`, `emit_wide_offset.c`, `emit_double_tree.c` | Call-free scalar expressions in scratch registers and scaled arithmetic in `lea`; optional Zen 3 three-operand scalar operations |
| `plan_loop_products.c`, `cached_double_product.c` | Check when a floating-point product from the condition is still valid in the loop body and keep it in a reserved register |
| `vector_loop.h`, `plan_vector_loop.c`, `emit_vector_loop_prefix.c`, `emit_vector_expression.c` | Validate independent integer loop iterations and emit SIMD arithmetic, stores and reductions ahead of the scalar tail |
| `packed_double_loop.h`, `plan_packed_double_loop.c` | Recognize increasing outer loops that initialize independent double calculations and count lanes that reach a common step limit |
| `plan_packed_double_step.c` | Check the inner comparison and straight-line updates, reuse identical expressions, and fit all live values into a bounded vector register plan |
| `emit_packed_double_loop.c`, `emit_packed_double_values.c` | Emit SSE2 or AVX2 groups, preserve affected registers, mask out finished lanes, and resume scalar code for the remainder or on a floating-point fallback |
| `emit_mask_select.c`, `mask_test_operands.c` | Match register bit-mask tests and lower short pure assignments to a conditional move |
| `tail_recursion.c`, `emit_fast_return.c`, `small_recurrence.c` | Reuse a frame for eligible self-recursion, return simple base cases at entry, and precompute bounded pure recurrences |
| `emit_conversion.c` | Extending narrow integers, integer/floating conversions (with the `ulong` cases), division, float loads and stores |
| `emit_condition.c` | Conditions as jumps, comparisons as `bool` values, signed and unsigned codes |
| `emit_call.c` | Arguments and calls |
| `emit_asm_block.c` | Copy an `asm { }` block, re-indented |
| `emit_address.c` | The address of an element or field: a direct operand when known at compile time, else computed into `rax`; `.length` |
| `emit_memory.c` | Loads and stores at an element's real size (`movzx`, `movsxd`, `mov byte [...]`, ...) |
| `emit_place_assignment.c` | `=`, `op=`, `++`/`--` on elements and fields |
| `emit_new.c` | `new T` / `new T[n]` through `rz@allocate` |
| `emit_conditional.c` | `c ? a : b` as a branch into `rax` or `xmm0` |
| `emit_aggregate.c` | Zeroing, `{ }` lists and text for declared arrays and structs, and copies |
| `emit_global_data.c` | Global arrays and structs as `db`/`dw`/`dd`/`dq` data with padding |
| `function_context.c` | Emit a line or label, source comments, temporaries, new labels |
| `source_line.c` | The text of a source line, for the comment above its instructions |
| `operands.c` | Memory, immediate, constant and string operands; a variable in a register (with `-O`) gives its register name instead of a stack slot |
| `contains_asm.c` (analysis) | Whether a statement tree holds an `asm` block |
| `runs_on_one_thread.c` (analysis) | Whether only the main thread can run razum code: not a DLL, no function used as a value, no `asm` block in your own code. Then the output lock is replaced by a `ret` |
| `emit_elf_header.c` | The Linux ELF header and its two program headers (one loaded segment, a non-executable stack) |
| `emit_linux_entry_point.c` | `rz@entry` on Linux: align the stack, pass `argc`/`argv`, call `main`, flush, `exit_group` |
| `assign_registers.c` | Assigns non-volatile registers to eligible scalars under `-O`. When integer registers fill, loop-weighted uses can displace a colder value. Keeps the parameter needed for single-argument tail calls |
| `constants.c` | Deduplicated string and floating constants |
| `append_nasm_string.c`, `double_bits.c` | NASM string literals; the bits of a double |

### `src/linking/`: choosing library code

| File | Job |
|---|---|
| `linking.h` | `AsmUnit` (origin and text) and `LinkResult` |
| `link_units.c` | Follow `rz@` references to the units that define them, transitively; collect `iat@` imports |
| `collect_references.c` | Every distinct `prefix@name` in a text |
| `unit_defines.c` | Does a unit define a label at the start of a line |

### `src/driver/`, `src/platform/`, `src/embedded/`, `src/main.c`

| File | Job |
|---|---|
| `driver/driver.h` | `Options`, `LoadedProgram` and driver functions |
| `driver/parse_options.c` | Command line and usage text |
| `driver/analyze_source.c`, `driver/print_symbols.c`, `driver/print_embedded_source.c` | `--symbols`: load and check while tolerating errors, then print every declaration as tab-separated fields; `--source`: print an embedded library file |
| `driver/read_source.c` | The source from a file or from standard input |
| `driver/instantiate_template.c` | Copies a file from `libraries/array/each/` or `libraries/array/integers/` once per number type, replacing `T`, `T_BITS`, `T_FLIP`, `T_SUM` and names ending in `_T` |
| `driver/load_program.c` | Parse the main file and resolve every `import`: a file or folder next to the importing file first, then the embedded libraries; gather `.asm` units. With `--check`, a file without `main` is a module: its folder's other `.rz` files (without their own `main`) are loaded too. Library files below a `windows/` folder are never loaded on Linux, and those below `linux/` never on Windows; importing a Windows-only library on Linux is an error |
| `driver/compile_source.c` | Load, check, mark, emit; for `--dll` also the `.def` and the header |
| `driver/assemble_file.c` | Find and run NASM |
| `driver/link_shared_object.c` | Invoke the C linker to produce a Linux shared object from relocatable ELF. |
| `driver/replace_extension.c` | `x.rz` to `x.exe` / `x.asm` |
| `platform/process.h` | Run a program; find a program on `PATH` |
| `platform/filesystem.h` | Is a path a file or folder; list files by extension |
| `platform/windows/run_process.c`, `find_program.c` | `CreateProcessA` (full path, inherited handles), `SearchPathA` |
| `platform/windows/rzc.rc`, `rzc.manifest` | Set the compiler process code page to UTF-8 |
| `platform/windows/run_assembler.c` | Stage Unicode input and output paths under ASCII temporary basenames for NASM |
| `platform/linux/run_assembler.c` | Feed ELF assembly through standard input to keep the object file symbol identical across builds. |
| `platform/{windows,linux}/same_file.c` | Compare file identity so output aliases cannot clobber inputs or other outputs |
| `platform/windows/path_kind.c`, `list_files.c` | `GetFileAttributesA`; sorted `FindFirstFileA`, recursive or one folder |
| `platform/windows/make_executable.c`, `host_target.c` | Nothing to do (an `.exe` runs as it is); `TARGET_WINDOWS` |
| `platform/linux/run_process.c`, `find_program.c` | `fork` and `execv`, optional stdout capture, exit status and a search of `PATH` with `access(X_OK)` |
| `platform/linux/path_kind.c`, `list_files.c` | `stat`; sorted `opendir`/`readdir`, recursive or one folder |
| `platform/linux/make_executable.c`, `host_target.c` | `chmod +x` on the new program; `TARGET_LINUX` |
| `common/target.h` | `Target` (Windows or Linux) and `host_target()` |
| `embedded/embedded_files.h` | `EMBEDDED_FILES`: path and text of every library file |
| `main.c` | Wire options, compile, write, assemble, run |

### `installer/`: razum-setup.exe

A Windows GUI program in C (wide-character Win32 API), built only on request (`--target razum-setup`). `rzc.exe`, `nasm.exe`, NASM's licence, the `.vsix` and `rz.ico` are embedded as `RCDATA` resources by the generated `setup.rc`.

| File | Job |
|---|---|
| `CMakeLists.txt` | Finds NASM and its licence, builds the `.vsix` with `package.ps1`, generates `setup.rc`, builds `razum-setup` and `test_path_list` |
| `make_icon.ps1`, `rz.ico`, `rz-icon-LICENSE.txt` | Takes VS Code's `cpp.ico` (the template of the other file icons), erases the C++ logo at every size and draws an orange "rz" in its box; writes a PNG-compressed `.ico`. The MIT notice of the template ships next to the icon |
| `src/setup.h`, `src/resource_ids.h` | Declarations; resource numbers shared with `setup.rc` |
| `src/main.c` | `--uninstall` runs `uninstall`, anything else `install`; `--quiet` and `--no-vscode` set the mode |
| `src/install.c`, `src/uninstall.c` | The two flows, with their questions and final messages |
| `src/install_paths.c` | `%LOCALAPPDATA%\Programs\razum` and its `bin` |
| `src/extract_resource.c` | Write one embedded file to disk |
| `src/path_list.c` | Pure: find, add and remove a folder in a `;`-separated list (tested) |
| `src/user_path.c` | Read and write `HKCU\Environment\Path`, then broadcast `WM_SETTINGCHANGE` |
| `src/set_user_string.c` | Write one `REG_SZ` under `HKEY_CURRENT_USER` |
| `src/file_type.c` | `.rz` → `razum.source` with icon and open command; drop the old "open with" choice; undo |
| `src/cmake_package.c` | The CMake user package registry entry for `find_package(Razum)` |
| `src/uninstall_entry.c` | The Installed apps entry |
| `src/find_vscode.c`, `src/run_hidden.c` | Locate `code.cmd` and `Code.exe`; run a command without a window and wait |
| `src/dialogs.c` | Yes/no questions, messages, fatal errors; in `--quiet` mode every question is answered yes and messages go to the parent console |

### Other folders

| Path | Job |
|---|---|
| `rz/cli/`, `rz/manifest/` | Select project commands, translate validated TOML into typed targets and compiler profiles, and map parser positions back to manifest text for package edits. |
| `rz/targets/` | Build targets in dependency order, reuse current Razum artifacts, stage results before publication and coordinate local CMake targets. |
| `rz/packages/` | Select stable or exact Git tags, resolve them to locked commits, materialize them in a shared source cache and stage manifest edits with the lockfile. |
| `rz/support/`, `rz/vendor/tomlc17/` | Thin program and filesystem helpers and the pinned MIT-licensed TOML parser. The parser records string coordinates at the opening delimiter. |
| `examples/packages/discord/` | Installable Razum package and slash-command example. The package maps relative REST routes, processes Gateway JSON, and verifies signed HTTP deliveries. |
| `examples/project_build/`, `examples/packages/csv/`, `examples/mixed_static/` | Portable manifest example, publishable CSV example and a Windows native link with Go, Rust, C, C++, Razum, NASM and Lua. |
| `libraries/array/` | Array methods: `each/` (templates for every number type: reverse, fill, contains, index_of, min, max, sum, sort_descending), `integers/` (templates for integer types: sort, introsort, insertion sort, heapsort, radix sort), `sort_double.rz` and `sort_float.rz` (ordered integer keys) |
| `libraries/stdio/assembly/windows/`, `linux/` | The runtime routines that talk to the system (`rz@write`, `rz@write_to`, `rz@write_newline`, `rz@write_error`, `rz@read_line`, the output lock); the same names on both |
| `libraries/program/assembly/windows/`, `linux/` | `rz@allocate`, `rz@release`, `rz@arguments`: `HeapAlloc` and the command line on Windows, an `mmap` allocator with size classes and `argv` on Linux |
| `libraries/threads/system/` | The lock layer of `threads`. Windows imports kernel32 locks. Linux implements the same lock and condition names using futexes, with the waiter count stored above the condition sequence |
| `libraries/threads/system/linux/futex_{begin_wait,end_wait,advance_if_waiting}.rz` | Register and release condition waiters, and advance the sequence only when a waiter may need waking |
| `libraries/threads/thread/windows/`, `linux/`, `assembly/linux/thread_clone.asm` | Starting, joining, sleeping, yielding, counting cores: `CreateThread` and friends on Windows; `clone` on a stack from `new`, a futex wait on the cleared thread id, `nanosleep`, `sched_yield`, `sched_getaffinity` on Linux |
| `libraries/process/windows/`, `linux/` | Running a program: job objects and named pipes on Windows; `fork`, `pipe2`, `poll`, `wait4`, `setrlimit` on Linux (`process_arguments.rz` splits the command line with the Windows rules, `process_child.rz` is what the child does before `execve`, `process_task_count.rz` counts the user's tasks in `/proc` for the process limit). `process_result.rz` and `process_free.rz` are shared |
| `libraries/stdio/output/windows/`, `linux/`, `errors/windows/`, `linux/` | `print_line` and `print_error`: the line end of each system (`\r\n`, `\n`) |
| `libraries/http/*/windows/`, `*/linux/` | The system part of `http`: I/O completion ports, `AcceptEx`, overlapped sends and SChannel TLS on Windows; an epoll loop per worker, `accept4`, non-blocking reads and writes on Linux. `state/http_types.rz` holds the shared request, response and route types; `state/<system>/http_system_types.rz` the server, worker, listener and connection |
| `libraries/crypt/certificate/` | Reading `.pfx` files on both systems: `der_read.rz` (DER elements, object identifiers, small integers), `pkcs12_derive.rz` (the PKCS#12 key derivation, RFC 7292 appendix B), `pfx_decrypt.rz` (Triple-DES and PBES2/AES), `pfx_bags.rz` (key and certificate bags), `pfx_private_key.rz` (PKCS#8 P-256 keys), `pfx_parse.rz` (structure, MAC check, leaf certificate first) |
| `libraries/crypt/cipher/des.rz`, `des3_cbc_decrypt.rz` | DES with tables checked against the FIPS known answer, and Triple-DES CBC decryption, for legacy `.pfx` files |
| `libraries/crypt/sign/` | P-256 and ECDSA on both systems: `modular.rz` (256-bit Montgomery arithmetic with sixteen 16-bit limbs for any odd modulus), `p256.rz` (the curve, Jacobian points, doubling, addition, scalar multiplication), `p256_public.rz`, `ecdsa_p256_sign.rz` (RFC 6979 nonces), `ecdsa_der.rz` (DER signature encoding) |
| `libraries/crypt/exchange/` | X25519 on both systems: `x25519_field.rz` (arithmetic modulo 2^255 - 19 with sixteen 16-bit limbs, as in TweetNaCl), `x25519.rz` (the Montgomery ladder), `x25519_public.rz` |
| `libraries/crypt/*/windows/`, `*/linux/` | The algorithm part of `crypt`: CNG calls on Windows; MD5, SHA-1, SHA-2, HMAC, PBKDF2, AES (CBC, CTR, GCM) and `getrandom` in razum on Linux, behind the same `Hasher` and function signatures |
| `libraries/crypt/cipher/linux/aes_cpu_features.rz` | Reads AES-NI, PCLMULQDQ and SSSE3 support via CPUID during key expansion |
| `libraries/crypt/cipher/linux/aes_encrypt_hardware.rz`, `gcm_multiply_hardware.rz` | AES block encryption and GHASH field multiplication selected by CPU flags, with separate scalar fallbacks |
| `libraries/http/tls/linux/` | The Linux TLS server: `http_tls12_hello.rz`, `http_tls12_step.rz`, `http_tls12_prf.rz` (TLS 1.2 handshake, key exchange, Finished and PRF), `http_tls_hello.rz` (ClientHello, ServerHello, key schedule, Certificate, CertificateVerify, Finished), `http_tls_seal.rz` (record encryption and decryption with AES-GCM), `http_tls_label.rz` (HKDF-Expand-Label, transcript snapshots, traffic keys), `http_tls_put.rz` (big-endian fields), `http_sealed_append.rz`; `connection/linux/http_serve_tls.rz` reads records, checks the client Finished and feeds decrypted data to `http_handle` |
| `libraries/linux/` | Linux only: `system_call` and `system_call6` (the `syscall` instruction in an `asm` block), `environment_value` (an environment variable, from the pointer the entry code keeps), and the system call numbers and flags as constants |
| `libraries/filesystem/*/windows/`, `*/linux/` | The parts of `filesystem` that call the system (open, read, write, seek, directories, paths); the `File` struct and the buffered reading and writing above them are shared |
| `libraries/filesystem/file/{read_bytes,write_bytes,file_copy_bytes}.rz` | Copy binary data in contiguous chunks through the shared File buffer, using the platform refill and flush functions |
| `libraries/filesystem/paths/linux/{copy_regular_file,open_copy_target,copy_file_contents}.rz` | Check file identity, create a sibling staging file, then copy and publish the replacement |
| `libraries/stdio/output/`, `input/` | Printing and reading functions, one razum file per function. `write_bool.rz` and `write_formatted_text.rz` support type-directed `printf`. |
| `tests/programs/print_and_printf.rz`, `custom_printf.rz` | Output and compatibility fixtures for `print`, `printf`, and explicit writes. The error fixtures cover arity and the stdio import. |
| `libraries/stdio/assembly/` | `rz@` routines they share: writing, number formatting and parsing, reading lines |
| `libraries/math/integer/`, `real/`, `random/` | Integer and floating math (x87 `asm` for trigonometry and logarithms), random numbers; `assembly/round_with.asm` |
| `libraries/process/` | Running a program: pipes for input and output, `CreateProcessW` suspended inside a job object (memory, process count, kill on close), reading with `PeekNamedPipe` until exit, timeout or output limit |
| `libraries/text/unicode/` | UTF-8 decoding, UTF-8 ⇄ UTF-16 conversion and validation in plain razum, so `text` does not depend on `windows` |
| `libraries/text/` | Text functions; `characters/` the character tests; `assembly/` the null-safe loops and number formatting |
| `libraries/json/types/` | `JsonToken`, `JsonReader`, `JsonOutput`, and constants used by the JSON reader and writer |
| `libraries/json/scan/`, `strings/` | JSON grammar, whitespace and number scanning, SSE2 ASCII string scan, UTF-8 and escape validation, code-point encoding, escaped key comparison |
| `libraries/json/read/`, `write/` | Parse, lookup and extract tokens, plus the bounded writer and its integer convenience function |
| `libraries/time/windows/`, `linux/`, `utc/` | Wall and monotonic clocks backed by host APIs, plus shared Gregorian UTC formatting |
| `tests/programs/json_library.rz`, `time_library.rz` | Portable tests and their `.expected` fixtures for JSON round trips, parser errors, clocks, and UTC dates |
| `libraries/filesystem/` | `file/` (buffered `File`, binary, seek), `paths/` (exists, delete, rename, copy, folders, whole-file text), `directory/` (listing); plain razum on `windows` |
| `libraries/windows/` | `extern` declarations per DLL (`kernel32.rz`, `user32.rz`, `gdi32.rz`, `gdiplus.rz`, `dwmapi.rz`, `shell32.rz`, `advapi32.rz`, `bcrypt.rz` with its CNG struct and constants, `secur32.rz` and `crypt32.rz` for TLS), `structs.rz`, `constants.rz`, `rgb.rz` |
| `libraries/net/types.rz`, `address/` | Socket/address constants, native address storage and portable IPv4/IPv6 parsing. Linux HTTP shares the numeric parser. |
| `libraries/net/socket/`, `transfer/`, `transport/` | Winsock or direct Linux syscalls under portable socket operations. Connect/listen/bind and exact byte transfers use these primitives. |
| `libraries/net/stream/` | Buffered line reads and nonblocking writes against a monotonic deadline, with unread bytes retained across timeouts. |
| `libraries/net/socket/*/net_poll_*.rz`, `net_wait_many.rz` | Portable event flags on a caller-owned native poll array, with one wait syscall or Winsock call. |
| `libraries/net/config/`, `resolve/`, `dns/` | Hostname lookup with the Windows resolver or Linux configuration files. The explicit DNS client uses UDP plus TCP fallback and bounded wire decoding. |
| `libraries/net/raw/`, `icmp/`, `ethernet/` | Packet byte helpers, native raw IP, system echo, and Linux `AF_PACKET` with IPv4 neighbor lookup on either host. |
| `libraries/winsock/` | `ws2_32.rz`: Winsock declarations, kept out of `windows` so their short names (`send`, `recv`, `bind`) stay free for programs that import `windows` |
| `libraries/http/state/` | `http_types.rz`: server, worker, connection, accept, request, response, route and listener structs; limits; the two `Date` buffers |
| `libraries/http/memory/` | `http_copy` (`rep movsb`) and `http_find` (SSE2 byte search), the two hot loops, in `asm` blocks |
| `libraries/http/output/` | Appending bytes, text and numbers to a worker's output buffer |
| `libraries/http/parse/` | End of the header block, the request parser (on a copy of the header block, so a request whose body is still missing can be parsed again), header lookup, query values with percent-decoding |
| `libraries/http/respond/` | The response API for handlers, status texts, serializing a response, error responses |
| `libraries/http/routing/` | Route registration and matching, static files, content types by extension |
| `libraries/http/connection/` | Per-connection state machine: accept, read and answer (`http_serve`), send with overlapped fallback, the zero-byte wait, close, the connection table |
| `libraries/http/client/` | Bounded HTTPS GET API. Windows uses asynchronous WinHTTP with a cancellation event and platform trust. Linux writes the embedded backend to sealed memory and launches the descriptor directly. |
| `http-backend/linux/main.c`, `src/codegen/emit_elf_https_backend.c` | Native libcurl client and compiler emission of its image when an ELF program calls the client. |
| `http-backend/linux/http_backend_call.c`, `http_backend_websocket.c`, `transport.c` | Verified HTTPS and WSS over a bounded local binary protocol. Linux opens the embedded backend from sealed memory. |
| `libraries/http/websocket/` | Portable message API with WinHTTP on Windows and the embedded transport on Linux. |
| `libraries/crypt/signature/` | Ed25519 verification using the existing Curve25519 field routines and streamed SHA-512. |
| `libraries/http/tls/` | SChannel: loading a `.pfx` and acquiring credentials, the handshake loop, decrypting records into the plaintext buffer, sealing responses into records, closing the context |
| `libraries/http/server/` | Creating, listening (address parsing, `AcceptEx` loading), starting workers and the timeout thread, stopping, waiting, Ctrl+C |
| `libraries/threads/thread/` | Starting (a trampoline frees the start block and calls `fn(long)`), joining, detaching, sleeping, `cpu_count` |
| `libraries/threads/channel/` | `Channel` (ring buffer of `long` under a slim reader/writer lock, two condition variables), send/receive/timeout/try, close, and `channel_select`, which waits on one shared signal that senders bump only while a select waits |
| `libraries/threads/mutex/`, `wait_group/`, `atomic/` | Slim reader/writer locks, a counter with a condition variable, and `lock xadd` / `xchg` / `lock cmpxchg` in `asm` blocks |
| `libraries/crypt/hash/` | `Hash` enum, `Hasher` (a CNG hash handle), start/update/finish, one-shot and hex helpers, the five named hashes; algorithms are CNG pseudo-handles (`0x21 + 16 * kind`) |
| `libraries/crypt/hmac/` | HMAC through the same `Hasher` with the HMAC pseudo-handles (`0x91 + 16 * kind`) and the key as the hash secret |
| `libraries/crypt/kdf/` | `pbkdf2` over `BCryptDeriveKeyPBKDF2` |
| `libraries/crypt/cipher/` | AES keys from `BCryptGenerateSymmetricKey`; GCM (ciphertext then tag), CBC with PKCS#7, and CTR built on ECB key-stream blocks, 4 KiB at a time |
| `libraries/crypt/random/` | `BCryptGenRandom` with the system-preferred generator; unbiased ranges by rejection; hex and URL-safe tokens; UUID version 4 |
| `libraries/crypt/encoding/` | Pure razum base64 (both alphabets, padding optional), base32, base16/hex and base85 (RFC 1924), each with a strict decoder |
| `libraries/crypt/bytes/` | `as_bytes`, xor, table-driven CRC-32 (`crc32_table.rz`, the 256 reflected `0xEDB88320` entries), Adler-32, constant-time compare |
| `libraries/stdio/assembly/write_to.asm`, `read_console_line.asm` | Writing to stdout or stderr: `WriteConsoleW` after UTF-8 → UTF-16 conversion on a console, `WriteFile` otherwise; reading a line from a console with `ReadConsoleW` |
| `libraries/stdio/assembly/output_lock.asm` | The spin lock (yielding after 64 tries) that every print and write function holds while it touches the output buffer |
| `libraries/gui/state/` | `gui_state.rz`: the `GuiCommand`, `GuiBox`, `GuiTheme` and `GuiState` structs, the one global `gui`, command kinds and font slots |
| `libraries/gui/theme/` | Light and dark colours, accent, the theme fade, colour mixing (`gui_mix`, `gui_fade`), reading the Windows app mode |
| `libraries/gui/typography/` | UTF-8 to UTF-16 into the per-frame text arena (`gui_wide`), measuring text, creating the four fonts |
| `libraries/gui/input/` | Element ids, hover and click tests, eased animation values, the queue of typed bytes and keys |
| `libraries/gui/editing/` | Editing a `char` array as UTF-8: caret steps, insert and delete, caret from a click, horizontal scroll |
| `libraries/gui/draw/` | Appending drawing commands (fill, stroke, shadow, circle, line, text) to the frame's command list |
| `libraries/gui/render/` | Replaying the command list with GDI+ (rounded paths, layered shadows, clipping) |
| `libraries/gui/layout/` | The container stack: panels, rows, columns, placing each element |
| `libraries/gui/window/` | Opening (window or off-screen), the frame loop, waiting, the window procedure, the back buffer, presenting, closing |
| `libraries/gui/input/windows/`, `input/linux/` | Platform UTF-16 unit input in `gui_type.rz`, converted to the shared UTF-8 editing events. `state/gui_keys.rz` gives the shared editing keys and cursor IDs. |
| `libraries/gui/window/windows/`, `render/windows/`, `theme/windows/`, `typography/windows/` | The Windows window procedures, GDI+ drawing, system theme, and font functions. Embedding preserves the original GUI function order for generated Windows binaries. |
| `libraries/gui/window/linux/` | Start, attach to Wayland or X11, manage software buffers, run the frame loop, poll display events, and clean up. `state/linux/gui_desktop.rz` holds the Linux backend state. |
| `libraries/gui/render/linux/`, `typography/linux/`, `theme/linux/` | Software spans, shapes, text layout and bitmap glyphs, UTF-8 conversion, measurement, and the shared colour palette. [Font and keyboard data notices](docs/linux-gui-notices.md) |
| `libraries/gui/wire/linux/` | Unix socket connection, polling, complete sends and receives, ancillary file descriptors, and key symbol conversion. |
| `libraries/gui/x11/linux/` | X11 setup, Xauthority cookie, supported visuals, keyboard map, atoms, window properties, events, and bounded PutImage requests. |
| `libraries/gui/wayland/linux/` | Registry binding, XDG toplevel, pointer and keyboard input, shared-memory buffers and cursor, configure and release events, and XKB keymap data. |
| `tests/test_gui.sh`, `tests/gui/backend.rz`, `tests/programs/gui_linux_offscreen.rz` | Linux protocol smoke tests, display-free failure handling, and offscreen drawing, input, and reopen checks. |
| `libraries/gui/widgets/` | The public elements: text, headings, buttons, checkbox, switch, slider, text field, progress bar, separator |
| `libraries/program/assembly/` | `arguments.asm` splits the command line for `main(string args[])`; `allocate.asm` and `release.asm` back `new` and `free`; not importable |
| `cmake/package/RazumConfig.cmake` | The CMake package for users: `add_razum_library`, `add_razum_executable`; installed and registered by the installer |
| `examples/mixed_project/` | C, C++ and razum in one CMake project; built by `tests/test_cmake.sh` |
| `cmake/embed_libraries.cmake` | Turns every `libraries/**/*.rz` and `*.asm` into a byte array in `build/embedded_files.c` |
| `CMakeLists.txt` | Builds `rzc.exe` from `src/` plus the generated file |
| `editors/vscode/` | VS Code extension: `package.json` (contributions), `extension.js` (live errors, hover and Go to Definition from `rzc --symbols`, library files through `rzc --source`, Run, Show Assembly), `syntaxes/` (TextMate grammar), `icons/`, `language-configuration.json`, `package.ps1` (builds the `.vsix` with `rzc.exe` inside) |
| `tests/` | `run_all.sh`, three script suites with their `programs/`, `errors/`, `shapes/` and `modules/` cases, and `installer/test_path_list.c` |
| `tests/website/test_docs.js` | Node checks for Docs coverage, links, tables, syntax colors and signature search |
| `tests/website/test_benchmarks.js` | Node checks for comparable chart data, workload switching, report anchors, refresh and load errors |
| `tests/errors_multiple/`, `tests/editor/` | Files with several errors and their whole expected output; the VS Code lookup test (`test_extension.js` with a `vscode` stub in `stubs/`), run by `tests/test_editor.sh` when Node is installed |
| `examples/` | Example programs |
| `PLAN.md` | The roadmap and work in progress, for whoever continues |
| `RULES.md`, `PLAN.md`, `PROGRESS.md`, `KNOWLEDGE.md`, `LICENSE` | Contributor rules, roadmap, dated log, benchmark notes, and the MIT License |
| `installer/linux/` | The Linux installer: `setup.sh` (the installing script, also `--uninstall`), `make_setup.sh` (appends the payload: `rzc`, NASM and its licence, `RazumConfig.cmake`, the `.vsix`), `CMakeLists.txt` (target `razum-setup`, output `build/razum-setup.sh`) |
| `editors/vscode/package.sh` | Builds the `.vsix` on Linux with `zip`, carrying the Linux `rzc` (`package.ps1` does it on Windows) |
| `build.sh`, `build.ps1` | Build scripts for Linux and Windows: check the tools, configure and build with CMake, optionally test and install |
| `.github/workflows/ci.yml` | GitHub Actions: `./build.sh --test` on Ubuntu, `tests/run_all.sh` in MSYS2 on Windows |
| `tests/test_cli.sh` | Temporary assembly and output collisions, exit codes, module lookup, Unicode Windows paths |
| `tests/test_filesystem.sh`, `tests/filesystem/replace.rz` | Linux copy and move behaviour for aliased paths, failed writes, and cross-filesystem symlinks |
| `tests/test_installer_linux.sh` | Install from a relative prefix, run the installed compiler from PATH, and uninstall |
| `platform/<system>/process_id.c` | The process id, for the temporary assembly file name |
| `tests/environment.sh` | Shared by the test scripts: build folder (`RZ_BUILD`), `.exe` or not, `fits_host` for `// platform:` lines |
| `benchmarks/http_load.c` | Linux load generator for `http` (epoll, pthreads; connections, pipeline depth, seconds, threads) |
| `benchmarks/` | Six matched workloads in Razum, C/C++, and Rust, plus optional Goose Fibonacci. `run.py` builds programs, validates output on every run, and measures median execution time |
| `benchmarks/run_libraries.py` | Builds and checks all supported library workloads, optionally interleaving a baseline compiler and a candidate, with a local pipeline driver for the HTTP server |
| `benchmarks/gcm_phases.rz`, `tests/programs/gcm_hardware_linux.rz` | Linux AES-GCM phase timing probe and scalar-versus-default differential coverage |
| `benchmarks/libraries/` | One standalone Razum probe for each bundled library, including the `program` runtime and native API wrappers |
| `website/` | The Razum language site. `learn.rz` serves `public/`, `docs/` and the local API. `server/` handles request checks, JSON and running code through `process`. `start.ps1` builds and opens the site, and `check_lessons.py` checks lesson solutions |
| `website/server/learn_benchmarks.rz` | Streams the selected compiler and library measurement sections of `KNOWLEDGE.md` at `/api/benchmarks` |
| `website/public/` | `index.html`, `css/style.css`, `lessons.txt` (lesson descriptions, starter code, tasks, test cases, hints and solutions), `js/app.js` (routing, home, course and playground), `js/docs.js` (reference navigation and signature search), `js/editor.js` (code input, highlighting and diagnostics), `js/highlight.js`, `js/markdown.js` (reference, lesson and benchmark rendering), `js/api.js`, `js/store.js` (progress in `localStorage`), `js/lessons.js` (lesson parsing) |
| `website/public/js/benchmarks.js` | Renders elapsed-time charts and the expandable report, including commands to reproduce the results |
| `website/public/js/benchmark_data.js` | Reads the measurement tables. Complete rows in `Current compiler benchmarks` take priority over earlier comparisons. Each comparison uses timings from one run, and library charts omit platforms without timings |
| `docs/` | Beginner tutorial, language, library, CMake, code generation and optimizer (`optimizer.md`) reference |

## Key decisions

| Decision | Reason | Rejected alternative |
|---|---|---|
| `gui` is immediate mode over a per-frame command list | Programs keep state in their own variables and need no callbacks or object trees; a panel's background command is reserved first and sized at `gui_end`, so one pass lays out and draws | Retained widget objects: need ownership and events, which razum (no methods, no closures) would make verbose |
| `gui` renders with GDI+ into a DIB section, copied with `BitBlt` | Anti-aliasing, ClearType text and alpha come with Windows, no extra DLL; no flicker; the same buffer serves off-screen tests | Direct2D/DirectWrite: COM vtables from razum; plain GDI: no anti-aliasing; a GPU renderer: far more code |
| `gui` folders import each other one way: `state` ← `theme`, `typography`, `input`, `draw` ← `render`, `editing`, `layout` ← `window`, `widgets` | No cycles; a library file must not import its own library (the embedded copy would clash with the file being checked) | |
| `const` values are folded while checking and replace each use; the globals are dropped before code generation | A constant is an immediate operand: no memory, no load, and it can size arrays and label cases | Read-only globals: still a memory load per use and no array sizes |
| Enums are `int` shapes that carry their enum, with scoped members (`Color.RED`) | All `int` code paths (arithmetic, `print`, `switch`, codegen) work unchanged; only storing into an enum is checked; scoped names avoid clashes in razum's single namespace | A separate value type: every conversion, operator and codegen path would need a new case; C-style unscoped members: clash across libraries |
| `http` uses I/O completion ports, not Registered I/O | The only published RIO gain is about 25% (a 2012 UDP test), and RIO needs a registered buffer per connection, which costs memory on every idle connection | RIO: revisit for a CPU-bound, few-connection workload |
| An idle `http` connection keeps only a zero-byte receive posted | No buffer per idle connection: under 1 KB of process memory each, so memory for a million connections is the kernel's | A real receive per connection: one system call fewer per request, but 4–64 KB per idle connection |
| Workers share one completion port and send with a non-blocking `send` first | Any worker serves any connection; most responses leave in one call without an overlapped buffer; only the rest is copied for an overlapped `WSASend` | One port per core: needs connection placement and stealing |
| A timeout thread sweeps the connection table every 250 ms (a quarter per sweep) and cancels expired I/O with `CancelIoEx` | Idle and slow-loris timeouts without a timer per connection; the worker that receives the cancelled operation closes the connection, so only one thread ever frees it | A timer wheel: O(1) per connection but needs locking with the workers |
| HTTPS through SChannel (`SCH_CREDENTIALS`, version 5) | TLS 1.3 and 1.2 from Windows itself: no OpenSSL to build or ship, updated by Windows Update, hardware AES | Bundling a TLS library: megabytes of C to compile and patch |
| TLS reuses the plain read path: raw records go into a per-connection buffer only while incomplete, plaintext into the same worker buffer `http_handle` parses | An idle HTTPS connection also holds no buffers; one request parser for both | A separate TLS connection type: duplicated parsing and buffering |
| The `.pfx` key is persisted, handed to SChannel, then deleted from the user key store | SChannel (in LSASS) cannot use a key that lives only in the server's memory (`PKCS12_NO_PERSIST_KEY` gives `SEC_E_NO_CREDENTIALS`), and leaving keys behind on every start would litter the store | Requiring the certificate in the Windows store: an extra install step for every user |
| Threads are OS threads; channels carry `long` | Real parallelism on every core with no scheduler to write; one value type works for numbers and pointers without generics, which razum lacks | Green threads (M:N): need a scheduler and stack switching for little gain at server thread counts; typed channels: need generics |
| `channel_select` uses one process-wide signal instead of registering on each channel | Senders pay one locked add when nobody selects; a select over any channels needs no per-channel waiter lists | Per-channel waiter lists (Go's design): faster for many concurrent selects, much more code |
| `crypt` uses Windows CNG through pseudo-handles; only encodings and checksums are razum | Hashes, AES and the random generator are maintained and reviewed with Windows, and fast (measured: SHA-256 2.3 GB/s, AES-256-GCM 3.4 GB/s); pseudo-handles need no opening, closing or locking, so the library has no global state and is thread-safe | Own implementations in razum: slower and unreviewed; `BCryptOpenAlgorithmProvider` once per algorithm: needs wide-string names and thread-safe lazy setup for no gain on Windows 10+ |
| `crypt` parts import one way: `bytes`, `encoding` ← `hash` ← `hmac` ← `kdf`; `random` → `encoding`; `cipher` → only `windows` | No cycles, and each part can be imported alone (`import crypt.encoding;` pulls in no CNG code) | |
| `crypt` writes results into arrays and reads inputs as pointer and length | The array's `.length` lets the library refuse to overflow a target, as `text` and `filesystem` do; a pointer input takes arrays, heap memory and text (`as_bytes`) alike | Pointer-and-capacity targets: every call gets one more argument and a size the caller can get wrong |
| Unicode on the console through `WriteConsoleW` / `ReadConsoleW`, not `SetConsoleOutputCP(65001)` | Correct output in any console without changing the user's console code page, which outlives the program | Switching the code page: shorter, but leaves the shell changed |
| `windows` and `linux` are reserved import names | A library's `windows/` platform folder would otherwise shadow the `windows` library for every file next to it | Renaming platform folders (`os_windows/`): longer paths everywhere for a rule that is easy to state |
| One source tree for Windows and Linux; `rzc` writes programs for the system it was built on; system-specific library code sits in `windows/` and `linux/` folders with the same routine names | Each build contains only its own system's code, so Windows output stays byte for byte what it was; everything above the runtime routines is shared | A separate Linux port (two code bases); a `--target` flag for cross compiling (not needed yet, can be added on top) |
| Linux programs call the kernel directly and keep razum's Windows-style calling convention | No libc, no dynamic linker, tiny static files; one convention for `asm` blocks on both systems | Linking against libc (needs the System V convention and a dynamic loader) |
| Array methods are written once with `T` for the element type, and `rzc` makes a copy per type when `array` is imported | One readable file per function instead of eleven; razum has no generics yet, and a C-style `void *` sort with a compare function is several times slower | Hand-written copies per type (110 files to keep in step), or a generator script whose output is checked in |
| The Linux installer is a self-extracting shell script with a tar payload | One file to download and run, like `razum-setup.exe`; needs only `sh`, `tar`, `gzip` and `awk`, which every distribution has | `.deb` and `.rpm` packages (one per distribution, need root), AppImage (for applications, not command-line tools) |
| `-O` keeps variables in the Windows non-volatile registers and saves them once per function | They survive every call, so no save and restore around calls and no liveness analysis are needed; the plain output stays readable for learning | A full register allocator with live ranges: much more code for the first step; it can come later behind the same flag |
| The website is plain HTML, CSS and JavaScript modules, served by a razum program | No build step, no downloads, works offline; the server shows off the `http` and `process` libraries | A framework or bundler: a toolchain to install for a handful of pages |
| The Docs tab loads `docs/language.md` and `docs/libraries.md` through a `/docs/` route instead of copying them into `website/public/` | One source for the reference: an edit in `docs/` is on the site on the next reload, and the Markdown stays exactly what GitHub shows | Copies under `public/docs/`: a sync step whose output goes stale, or a generator that must run before the site starts |
| Lessons live in a plain text file with `===`/`---` markers, not in JavaScript | Lesson text is full of backticks and backslashes that string literals would mangle; the same file feeds the browser and `check_lessons.py` | Lessons in JSON or JS: escaping in every code sample |
| Compiler in C17 | One native `rzc.exe`, no runtime dependency, builds with GCC and MSVC | Python (needs an interpreter; started and dropped on request), Rust, Go |
| Output is one plain NASM file, no macros | What is in the `.asm` is exactly what runs; any NASM assembles it | Emitting the pe64 macro library: shorter output, but readers must know the macros |
| PE headers and imports written by the compiler | No linker; the file is self-contained | `nasm -f win64` plus a linker: adds tools and padding |
| One RWX section, 4-byte alignment | Smallest files and file offset = memory offset, as proven in the asm project | Page-aligned sections: at least 2 KiB per program |
| Libraries written in razum with `asm { }` blocks, plus `.asm` units | Adding a library is adding files; the language proves itself on its own library; machine-level code stays possible | A fixed table of runtime routines in the compiler (the first version): every new routine needed C changes |
| Libraries embedded into `rzc` at build time | `rzc.exe` is one file that works from any folder | Reading `libraries/` at run time: `rzc` would depend on its install folder |
| Only reachable functions emitted; units linked by `rz@` reference | Importing a big library costs nothing; programs stay minimal | Emitting everything imported |
| Local modules before built-in libraries | A project can override or extend a library without rebuilding `rzc` | Built-ins first: a local file could never shadow one |
| `int` is 32-bit, `long` 64-bit, sizes as in C# | The types the user expects from C# and Java; `char`/`byte`/`short` complete the set | 64-bit `int` (the first version): unusual and leaves no name for 32 bits |
| Arrays and structs packed at real sizes, like C | Text in `char` arrays is a C string; `asm` and Windows calls see the expected layout; memory operands work directly | 8-byte elements like scalar slots: wastes memory and breaks text |
| Array and struct parameters by reference, arrays with a hidden length | No copying; `values.length` works in every function | Copying structs into the callee; C's bare pointers without a length |
| CMake support as a package file, not a CMake language plugin | Plain `add_custom_command` and imported targets work with every CMake version from 3.20 and every generator that runs commands | `enable_language(RAZUM)` with a CMake language module: compiles per file, which razum's whole-program compile does not fit |
| DLLs relocatable through one relocation block per string pointer | Code is RIP-relative, so string pointers in data are the only absolute addresses; one block per entry needs no page grouping in NASM | Fixed-base DLLs: they fail to load when the address is taken |
| Narrow variables whose address is taken use sized access | Keeps the 64-bit slot rule for every other variable, so most code stays one instruction per access | Sized slots for all variables |
| Function values are plain addresses with the Windows x64 convention | Any razum function is a valid Windows or C callback without wrappers | Closures: need captured state, which C callers cannot pass |
| `.` reaches through pointers, `->` accepted too | One spelling for fields whether the struct is local or on the heap; C habits still work | C's mandatory `->` |
| `extern` declarations instead of a C header parser | One line per function, checked like razum code; the `windows` library is plain razum | Reading Windows SDK headers: a C preprocessor and parser |
| Modules checked with their folder in `--check` | The editor can check any file; modules need no header files, `import` states dependencies | Header files: a second file to keep in sync per module |
| `switch` cases do not fall through | C's fallthrough is a common bug; C# also forbids it | C's fallthrough with required `break` |
| `File` as a razum struct with its own buffer | Buffered I/O with no hidden state in the library; written mostly in razum | One `ReadFile` per character: thousands of times slower |
| No runtime index checks | Element access stays one instruction | Checks on every access; constant indexes are still checked when compiling |
| Every variable in an 8-byte slot, integers kept extended to 64 bits | Arithmetic and comparisons stay single 64-bit instructions for every type; only `ulong` needs unsigned forms; one extend instruction restores a narrow result | Sized slots: more load forms (`movsx`/`movzx`) and fewer direct memory operands |
| `float` computed in double precision, rounded when stored | One floating code path; correctly rounded for single operations | Separate `ss` instruction paths for every operator |
| Implicit conversions only widen; literals fit by value | No silent data loss; `byte b = 200;` still works | C's implicit narrowing |
| Comparisons give `bool`, but conditions also take integers | `print(a < b)` shows `true`; `while (1)` keeps working | Requiring `bool` everywhere as C# does |
| Every variable in a stack slot, no register allocation | Each statement compiles to a short, predictable pattern that can be read next to its source line | Register allocation: faster code, far harder to follow |
| Arguments copied to home slots on entry | Parameters and locals are read the same way, also from `asm` blocks | Keeping arguments in registers: needs liveness tracking across calls |
| `$name`, `.name`, `@` in compiler names | User names map to themselves, never clash with registers, instructions or compiler labels | Prefixes such as `f_add`: the output no longer matches the source |
| Report every error: `compile_error` jumps back to the nearest recovery point (`diagnostic_try`, a `setjmp`) around each top-level item, struct, global, signature, function and statement | All mistakes at once, each still at one exact place. Scopes and loop depth are restored at each point; a declaration whose initial value fails is still declared and a failed `const` gets a zero stand-in, so neither causes follow-on errors. Syntax errors stop before checking, which would only report holes | Stop at the first error: simpler, but slow to fix files with several mistakes |
| Editor features from `rzc --symbols` (tab-separated declarations) instead of a language server | Hover and Go to Definition reuse the real parser and checker; one process per change, measured in milliseconds; no protocol code | A language server: incremental, but a second front end to keep in step |
| Arena memory, never freed | The compiler runs once and exits; no ownership rules to get wrong | `malloc`/`free` per node |
| `x++` only as a statement | Expressions stay free of side effects except calls, so evaluation order never matters | C's `x++` in expressions |
| Linear symbol and function lookup | Programs are small; simplest correct code | Hash tables: worth it only for very large files (see KNOWLEDGE.md) |
| Tests as bash scripts with data files | Adding a case is adding a `.rz` file; no test framework needed | A C test harness |
| Installer as a C program with embedded resources, per user | Builds with the existing toolchain, no admin rights, one file to hand out; uninstalls cleanly | An MSI: needs the WiX toolset, a download; Inno Setup or NSIS: another tool and script language |
| NASM bundled next to `rzc.exe` | `SearchPathA` looks in `rzc`'s own folder first, so an install works without a separate NASM | Requiring NASM on PATH: one more manual step |
| Remove the `.rz` UserChoice at install | Windows shows the icon of the user's "always open with" app, which hid the razum icon; `razum.source` opens VS Code anyway | Asking the user to change it by hand |
| VS Code extension in plain JavaScript, packaged by a PowerShell script | No npm dependencies or build step; the `.vsix` is a zip with two XML files | `yo code` + `vsce`: downloads hundreds of packages for a 150-line extension |
