# How razum becomes assembly

`rzc` writes one NASM file that `nasm -f bin` turns into the final `.exe`, with nothing else involved. This page explains every part of that file so it can be read line by line. To see it for your program:

```powershell
.\build\rzc.exe app.rz -S        # writes app.asm, no .exe
.\build\rzc.exe app.rz --asm     # writes app.exe and keeps app.asm
```

## File layout

| Part | Contents |
|---|---|
| Header | `bits 64`, `default rel`, `org 0`, then the DOS header, PE header, optional header and one section header, each field commented |
| `rz@entry` | Program entry: reserve shadow space, `call $main`, `call rz@flush` if the program links buffered output, pass `main`'s result to `ExitProcess` |
| Functions | Every razum function the program calls (directly or through other functions), including library functions, in load order. Unused ones are left out |
| Linked units | `; from <file>` and the text of each `.asm` unit whose `rz@` labels are referenced |
| Data | Floating constants (`num@N`), globals (`$name`), strings (`str@N`) |
| Imports | One import address table per DLL (`pe@iat@dll`, entries `iat@Name` or `iat@dll@Name`), the import directory with one entry per DLL, function names, DLL names |
| `pe@image_end` | End of the file |

There is one section that is readable, writable and executable, aligned to 4 bytes, so every file offset equals its address offset in memory. `default rel` makes memory accesses relative to the instruction (RIP-relative).

## Names in the output

| Name | Means |
|---|---|
| `$name` | Your function or global `name`. `$` lets NASM accept any name, including `add` or `rcx` |
| `.name` | Your local variable or parameter, as an offset from `rsp` |
| `.name@1`, `.name@2` | A later variable with the same name in another block |
| `.@frame` | Size of the current function's stack frame |
| `.@t0`, `.@t1` | Temporary slots for intermediate results |
| `.@else3`, `.@while5`, `.@large7`, ... | Jump targets made by the compiler |
| `rz@name` | Assembly routines from libraries or your modules |
| `iat@Name`, `iat@dll@Name` | The address of the imported function `Name` from `kernel32.dll` or from `dll.dll`; a call to an `extern` function is `call [iat@dll@Name]` |
| `str@N`, `num@N`, `pe@...` | Strings, floating constants, PE structure |

razum names cannot contain `@`, so compiler names never collide with yours.

## Functions and the stack frame

Every function follows the Windows x64 calling convention:

- Arguments 1-4 arrive in `rcx`, `rdx`, `r8`, `r9`, or `xmm0`-`xmm3` for `float` and `double`; further arguments are on the stack.
- The result is returned in `rax` (integers, `bool`, `char`, `string`) or `xmm0` (`double`, and `float` in single precision).
- The caller reserves 32 bytes of "shadow space" above its stack arguments, and `rsp` must be 16-byte aligned at every `call`.

```nasm
; int square(int value)
$square:
.@frame equ 40
.value equ 48
    mov qword [rsp + 8], rcx      ; store argument 1 in its home slot
    sub rsp, .@frame
    ; 8: return value * value;
    mov rax, qword [rsp + .value]
    imul rax, qword [rsp + .value]
    movsxd rax, eax               ; int result: keep the low 32 bits, sign-extended
    add rsp, .@frame
    ret
```

Frame layout after `sub rsp, .@frame`, from `rsp` upwards:

| Offset | Contents |
|---|---|
| 0 | 32 bytes shadow space for the calls this function makes |
| 32 | Stack arguments for the widest call this function makes (arguments 5 and up) |
| then | One 8-byte slot per local variable (`.name equ offset`) |
| then | One 8-byte slot per temporary (`.@tN equ offset`) |
| `.@frame` | The return address; `.@frame` is chosen so `rsp` stays 16-byte aligned |
| `.@frame + 8` | Home slots of arguments 1-4, then the caller's stack arguments 5+ |

Each argument is copied from its register to its home slot at entry, so parameters are read exactly like locals. Generated code uses only `rax`, `rcx`, `rdx`, `r8`, `r9` and `xmm0`-`xmm3`, which every call may change anyway, so no register needs saving.

## Types in memory and registers

Every variable has one 8-byte slot, whatever its type.

| Type | Slot holds | Computed in |
|---|---|---|
| `bool`, `char`, `byte`, `ushort`, `uint` | the value zero-extended to 64 bits | `rax` |
| `sbyte`, `short`, `int` | the value sign-extended to 64 bits | `rax` |
| `long`, `ulong` | the value | `rax` |
| `string` | the address of the text | `rax` |
| `float` | the float in the low 4 bytes | `xmm0`, as a double |
| `double` | the value | `xmm0` |

Because every integer slot holds the exact value in 64 bits, ordinary 64-bit instructions give the right answer for every type up to 32 bits, signed or unsigned. One extend instruction after an operation brings a result back to its type:

| Result type | Instruction |
|---|---|
| `int` | `movsxd rax, eax` |
| `uint` | `mov eax, eax` |
| `short` / `ushort` | `movsx rax, ax` / `movzx eax, ax` |
| `sbyte` / `byte`, `char`, `bool` | `movsx rax, al` / `movzx eax, al` |

It appears after `+ - * / %`, unary `-`, narrowing casts, compound assignments and `++`/`--` on narrow variables, and after calls that return narrow types. `long` and `ulong` need none. Only `ulong` needs unsigned instructions: `div` instead of `idiv`, and `jb`/`ja`-style jumps instead of `jl`/`jg`.

## Arrays and structs in memory

Arrays and structs are stored packed, at their real sizes: an `int` element takes 4 bytes, a `byte` 1, as in C. A local array or struct takes its size rounded up to 8 bytes in the frame; a global one is written into the data with `times N db 0` for zeros, so a large global array makes the `.exe` larger by its size.

| razum | Assembly |
|---|---|
| `numbers[1] = 7;` | `mov dword [rsp + .numbers + 4], 7` |
| `numbers[1] += 3;` | `add dword [rsp + .numbers + 4], 3` |
| `p.flag++;` (byte field at offset 8) | `inc byte [rsp + .p + 8]` |
| `x = p.second;` | `movsxd rax, dword [rsp + .p + 4]` ... |
| `numbers[i] = v;` | value into a temporary, `mov rcx, i` / `lea rax, [rsp + .numbers]` / `lea rax, [rax + rcx*4]`, `mov dword [rax], ecx` |
| `a = b;` (24 bytes) | `lea rcx, [rsp + .b]` and three `mov rdx, qword [rcx + k]` / `mov qword [rax + k], rdx` pairs; above 128 bytes a loop |

- A place whose address is known when compiling (a local or global, with constant indexes and fields) is used directly as the memory operand. Otherwise its address is computed into `rax`.
- Because elements are stored at their real size, `+=`, `-=`, `++` and `--` act on memory directly (`add dword [...]`) and wrap at the element's size. `*=`, `/=` and `%=` load, compute and store.
- A declared array or struct is zeroed first (`mov qword [...], 0` for up to 128 bytes, a loop above), then its `{ }` list or text is stored.
- An array or struct argument passes its address. An array parameter also takes the next argument position for its length, so `long sum(int values[])` receives the address in `rcx` and the length in `rdx`.
- A frame larger than 4096 bytes is reserved one page at a time (`.@probe:` loop), because Windows grows the stack only through the guard page just below it. The stack is 8 MiB.

## Pointers and the heap

A pointer is an 8-byte value in `rax` like a `long`. `*p`, `p[i]` and `p.field` load the pointer and then use the same sized loads and stores as array elements (`movsxd rax, dword [rax]`, `mov dword [rax], ecx`). `&x` is `lea rax, [rsp + .x]`.

A narrow integer variable (`int`, `short`, `byte`, ...) whose address is taken anywhere is accessed with sized instructions too (`movsxd rax, dword [rsp + .x]`, `add dword [rsp + .x], 1`), because a write through the pointer changes only its low bytes and the 64-bit slot would no longer hold the extended value.

`new` calls `rz@allocate` (`HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)`), `free` calls `rz@release` (`HeapFree`); both come from `libraries/program/assembly/` and are linked only when used.

## Statements

| razum | Assembly |
|---|---|
| `long x;` | `mov qword [rsp + .x], 0` |
| `x = 5;` | `mov qword [rsp + .x], 5` |
| `x = a + b;` (longs) | `mov rax, qword [rsp + .a]` / `add rax, qword [rsp + .b]` / `mov qword [rsp + .x], rax` |
| `x++;` (long) | `inc qword [rsp + .x]` |
| `x += 5;` (long) | `add qword [rsp + .x], 5` |
| `b += 10;` (byte) | `mov rax, qword [rsp + .b]` / `add rax, 10` / `movzx eax, al` / `mov qword [rsp + .b], rax` |
| `f = 0.5;` (float) | `movsd xmm0, qword [num@1]` / `cvtsd2ss xmm0, xmm0` / `movss dword [rsp + .f], xmm0` |
| `return e;` | value into `rax`/`xmm0`, `add rsp, .@frame`, `ret` |

`if (a < 10) A else B`:

```nasm
    mov rax, qword [rsp + .a]
    cmp rax, 10
    jge .@else2          ; jump when the condition is false
    ...A...
    jmp .@endif1
.@else2:
    ...B...
.@endif1:
```

`while (c) S` tests at the top and jumps back from the bottom. `for (init; c; step) S` is `init`, then the same loop with `step` at its own label, so `continue` jumps to the step and `break` jumps past the loop.

## Expressions

The result of an integer, `bool` or `string` expression ends up in `rax`; of a `float` or `double` expression, in `xmm0` as a double.

- If the right side of an operator is a variable or a literal that fits in 32 bits, it is used directly as the instruction's operand: `add rax, qword [rsp + .b]`, `cmp rax, 10`.
- Otherwise the left side is saved in a temporary, the right side is computed, and both are brought back together: `mov .@t0, rax` ... `mov rcx, rax` / `mov rax, .@t0` / `add rax, rcx`.
- `/` and `%` use `cqo` and `idiv rcx` (`xor edx, edx` and `div rcx` for `ulong`); `%` takes the remainder from `rdx`.
- Comparisons used as values become `cmp`, `setCC al`, `movzx eax, al`. Inside conditions they become `cmp` and one conditional jump, and `&&`/`||` become chains of jumps that skip the right side.
- Floating comparisons use `ucomisd`, with an extra `jp` for `==` and `!=` so NaN compares correctly.
- Floating negation flips the sign bit with `btc`.

| Conversion | Instructions |
|---|---|
| integer to double | `cvtsi2sd xmm0, rax` |
| `ulong` to double | the same when the top bit is clear; otherwise halve (keeping the low bit), convert and double |
| double to integer | `cvttsd2si rax, xmm0`, then the extend instruction of the target |
| double to `ulong` | the same below 2^63; otherwise subtract 2^63, convert and set the top bit |
| `float` variable to double | `cvtss2sd xmm0, dword [rsp + .f]` |
| double to `float` | `cvtsd2ss xmm0, xmm0` |
| integer to `bool` | `test rax, rax` / `setne al` / `movzx eax, al` |

## Calls

All arguments are computed first, the complicated ones into temporaries. Then stack arguments are stored at `[rsp + 32]`, `[rsp + 40]`, ..., registers are loaded, and `call $name` follows. Loading registers last means computing one argument can never overwrite another. A `float` argument is passed in single precision, as the convention requires.

## asm blocks

The text of an `asm { }` block is copied into the function unchanged, with the common indentation removed. Lines that are labels (`.done:`) start at column 0; every other line is indented by four spaces. The source comment `; N: asm {` marks where it starts.

## DLLs

With `--dll` the same file layout is used with these differences:

| Part | Executable | DLL |
|---|---|---|
| Preferred base (`pe@base`) | `0x140000000` | `0x180000000` |
| File characteristics | `0x0022` | `0x2022` (adds "DLL") |
| DLL characteristics | `0` | `0x0060`: relocatable with high-entropy addresses, so Windows usually loads it elsewhere |
| Entry `rz@entry` | calls `main` and `ExitProcess` | `DllMain`: returns 1; on unload sends buffered output |
| Export directory | empty | `pe@exports`: sorted names, ordinals and the `$name` addresses of every `export` function |
| Relocation directory | empty | one block per absolute address: each string pointer in data is `pe@reloc@N: dq pe@base + str@M`, and Windows adds the load offset to it |

An exported function stores its narrow integer and `bool` parameters again after the usual home-slot spill (`movsxd rax, dword [rsp + 8]` / `mov qword [rsp + 8], rax`), because C callers leave the upper register bits undefined while razum code expects every slot extended to 64 bits.

## Linking library code

There is no linker. `rzc` builds the program text, then:

1. Collects every `rz@name` the text references.
2. Finds the `.asm` unit that defines each label (`rz@name:` or `rz@name equ` at the start of a line), among the library units embedded in `rzc` and the `.asm` files of imported folders.
3. Adds that unit and repeats with the labels the unit references, so dependencies come along.
4. Collects every `iat@Name` and `iat@dll@Name` in the program and the added units, groups them by DLL, and builds one import descriptor per DLL. `ExitProcess` is always imported.

Globals declared in library files (constants such as `MB_OK` or `PI`) are emitted only when the program uses them.

A program that prints a string therefore contains `rz@write_string`, `rz@write_newline`, `rz@write`, `rz@flush`, `rz@write_handle` and the buffer variables, and imports `ExitProcess`, `GetStdHandle`, `GetConsoleMode`, `VirtualAlloc` and `WriteFile`. A program that prints nothing imports only `ExitProcess`.
