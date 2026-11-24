# Libraries

Add an `import` for the library you want to use. The compiler ships with these libraries and links only the code and data your executable needs. Most libraries work on both Windows and Linux. Use [windows](#windows) or [winsock](#winsock) for Windows APIs and [linux](#linux) for Linux kernel APIs.

Built in: [stdio](#stdio) · [array](#array) · [text](#text) · [json](#json) · [time](#time) · [math](#math) · [filesystem](#filesystem) · [windows](#windows) · [winsock](#winsock) · [linux](#linux) · [gui](#gui) · [threads](#threads) · [http](#http) · [crypt](#crypt) · [process](#process) · [program](#program).

`import math;` makes all of `math` available. `import math.real;` brings in only the floating-point part. Imported functions share the same namespace as your own functions. If names clash, `rzc` reports both declarations.

## stdio

Use `import stdio;` to read from the console or standard input and write to standard output. The library also exposes `stdio.output`, `stdio.input`, `stdio.errors` and `stdio.console` as separate imports.

| Call | Use it when |
|---|---|
| `print(value)` | You want to display a value and end the line. |
| `printf(value)` | You want several values on one line, with optional `%n` for a line break and `%%` for a literal percent sign in text. |
| `void write_string(string text)` | You want to write text exactly as supplied, including percent signs, without adding a line break. |

### print

`print(value)` chooses a formatter for the supported types below and appends the platform’s line ending.

| Value type | Calls | Output |
|---|---|---|
| `bool` | `print_bool` | `true`, `false` |
| `char` | `print_char` | the character |
| `byte` `sbyte` `short` `ushort` `int` `uint` `long` | `print_long` | `-42` |
| `ulong` | `print_ulong` | `18446744073709551615` |
| `float` | `print_float` | up to 7 significant digits: `0.1`, `0.3333333` |
| `double` | `print_double` | up to 15 significant digits, always with a `.`: `3.0`, `1.0e20`, `inf`, `nan` |
| `string`, `char` array | `print_string` | the text up to its `\0` |

### Output

| Function | Writes |
|---|---|
| `print_bool(bool value)`, `print_char(char value)`, `print_long(long value)`, `print_ulong(ulong value)`, `print_float(float value)`, `print_double(double value)`, `print_string(string text)` | the value, then a line ending |
| `write_bool(bool value)`, `write_char(char value)`, `write_long(long value)`, `write_ulong(ulong value)`, `write_float(float value)`, `write_double(double value)`, `write_string(string text)` | the value, no line end |
| `void print_line()` | only a line ending |
| `void write_hex(ulong value)` / `void print_hex(ulong value)` | lowercase hexadecimal without `0x`: `ff` |
| `void flush_output()` | sends buffered output now |

`printf` uses the same types as `print` and accepts one value per call. Sequences such as `%d` go straight to output as text. To mix text and numbers, make successive calls. The following prints `2 h 3 min` followed by a line ending.

```c
import stdio;

int main() {
    int minutes = 7384 / 60;
    printf(minutes / 60);
    printf(" h ");
    printf(minutes % 60);
    printf(" min%n");
    return 0;
}
```

A declared function named `printf` takes precedence, so existing code using its own function keeps working.

Output is buffered in 64 KiB. It is sent when the buffer fills, when the program exits, before reading input, and at line endings when writing to the console. Use `flush_output()` to send it immediately. An `asm` block that ends a Windows program with `ExitProcess` must `call rz@flush` first.

### Input

| Function | Reads |
|---|---|
| `bool read_input(char line[])` | one line of text into `line` (cut to fit, without its line end); `false` at the end of input |
| `int read_int()`, `long read_long()` | one line as an integer: leading spaces, an optional sign, digits; 0 if there is no number or input ended |
| `double read_double()` | one line as a floating number (`1.5`, `-2e3`, `.5`); 0.0 likewise |

Each call reads one whole line of up to 255 characters (`read_input`: up to the array's size); the rest of a longer line is skipped.

### Errors and console

| Function | Does |
|---|---|
| `void write_error(string text)` / `void print_error(string text)` | writes to standard error (unbuffered), after sending buffered standard output so the order stays right |
| `void set_color(int color)` | console text colour: `COLOR_BLACK`, `COLOR_BLUE`, `COLOR_GREEN`, `COLOR_CYAN`, `COLOR_RED`, `COLOR_MAGENTA`, `COLOR_YELLOW`, `COLOR_WHITE`, each `+ COLOR_BRIGHT`, and background colours as `16 *` a colour |
| `void reset_color()` | back to grey on black |

```c
set_color(COLOR_RED + COLOR_BRIGHT);
print("error");
reset_color();
```

## array

`import array;` gives every array of numbers or `char`s these methods. They work on fixed arrays, array parameters and fields alike (`values.sort()`), for `char`, `byte`, `sbyte`, `short`, `ushort`, `int`, `uint`, `long`, `ulong`, `float` and `double`.

| Method | Does |
|---|---|
| `values.sort()` | Smallest first |
| `values.sort_descending()` | Largest first |
| `values.reverse()` | Last element first |
| `values.fill(x)` | Every element becomes `x` |
| `values.contains(x)` | `true` if some element equals `x` |
| `values.index_of(x)` | The first position of `x` as a `long`, `-1` if it is not there |
| `values.min()`, `values.max()` | The smallest and largest element; `0` for an empty array |
| `values.sum()` | The total: a `long` for integer arrays (`ulong` for `ulong` arrays, wrapping around on overflow), a `double` for `float` and `double` arrays |

`values.first` and `values.last` need no import ([language.md](language.md#arrays)).

Each method calls a function named `array_<method>_<type>`, which you can also call yourself: `array_sort_long(values)`. To sort part of an array or memory from `new`, call `array_sort_items_long(pointer, count)` (for every integer type).

How sorting works, chosen by measurement (numbers in [KNOWLEDGE.md](../KNOWLEDGE.md#sorting)):

1. An array that is already in order costs one pass and is left alone; one in strictly falling order is reversed.
2. From `48 × bits` elements up (3,072 for `long`, 1,536 for `int`, 384 for `byte`), a least-significant-digit radix sort: one pass counts every byte position at once, then one pass per byte moves the elements, skipping bytes that are equal in every element (so small numbers in a `long` array take few passes). It needs a second buffer of the same size from the heap; if that fails, it falls back to the next step.
3. Otherwise introsort: quicksort with the median of three, insertion sort below 25 elements, and heapsort if the recursion gets too deep, so the worst case stays `n log n`.
4. `float` and `double` are turned into ordered integer keys in place, sorted as `uint` or `ulong`, and turned back. The order is IEEE total order: `-0.0` before `0.0`, NaNs at the ends; no comparison is ever made with a NaN.

The functions are written once in `libraries/array/each/` (every number type) and `libraries/array/integers/` (integer types) with `T` for the element type, `T_BITS` for its size in bits, `T_FLIP` for its sign bit (`0` for unsigned types) and `T_SUM` for the type `sum` returns. `rzc` makes one copy per type when a program imports `array`; no other folder is treated this way.

## text

`import text;` — the character tests are in `text.characters`, the Unicode functions in `text.unicode`.

Text uses UTF-8. Use `string` for text you read and a `char` array as the destination when building or changing text. `length` counts bytes, while `utf8_length` counts Unicode code points. For example, `"Grüße"` is seven bytes and five code points.

### Unicode and Windows conversions

| Function | Result |
|---|---|
| `long to_utf16(ushort target[], string text)` | UTF-16 for Windows `W` functions, ending with 0; the count of UTF-16 units, or -1 if `target` is too small |
| `long from_utf16(char target[], ushort *wide)` | UTF-8 from UTF-16 text that ends with 0; the count of bytes, or -1 if `target` is too small |
| `long utf8_length(string text)` | characters (code points), not bytes: `utf8_length("Grüße")` is 5, `length` is 7 |
| `bool is_utf8(string text)` | whether the bytes are valid UTF-8 |
| `int utf8_decode(string text, long index, int *size)` | the code point starting at byte `index`, and its byte count in `*size` |

Invalid sequences become U+FFFD (`�`) when converting.

### Search, edit and convert numbers
| Function | Result |
|---|---|
| `long length(string text)` | bytes before the closing `\0` |
| `int compare(string left, string right)` | -1, 0 or 1 as `left` sorts before, equal to or after `right` (by byte value) |
| `bool equals(string left, string right)` | the texts are the same |
| `int compare_ignore_case(...)`, `bool equals_ignore_case(...)` | the same, ignoring ASCII letter case |
| `long find(string text, string part)` | index of the first `part` in `text`, or -1 |
| `long find_char(string text, char wanted)` / `find_last_char` | byte index of the first / last `wanted`, or `-1`. `find_char` stops at the first match or the terminating zero. |
| `bool contains(string text, string part)` | `find(...) >= 0` |
| `bool starts_with(string text, string prefix)` / `ends_with(string text, string suffix)` | checks the start or end of the byte string |
| `long copy(char target[], string source)` | copies `source` into `target` and returns the number of bytes copied |
| `long concat(char target[], string source)` | appends `source` to the text in `target` and returns the new byte length |
| `long append_char(char target[], char value)` | appends one byte and returns the new byte length |
| `long substring(char target[], string text, long start, long count)` | copies up to `count` bytes starting at byte offset `start` |
| `long trim(char target[], string text)` | copies `text` without leading and trailing spaces, tabs and line ends |
| `long split_part(char target[], string text, char separator, long index)` | copies part `index` (from 0) of `text` split at `separator`, as in CSV fields; -1 if there is no such part |
| `void to_upper(char text[])` / `to_lower(char text[])` | changes ASCII letters in place |
| `void reverse(char text[])` | reverses the text in place |
| `long int_to_text(char target[], long value)` / `ulong_to_text` / `hex_to_text` / `double_to_text` | writes a number as text; returns its length |
| `long parse_int(string text)` / `double parse_double(string text)` | the number at the start of `text` after spaces; 0 if there is none |
| `bool is_digit(char)`, `is_letter(char)`, `is_space(char)` | ASCII character tests |
| `char upper_char(char)`, `lower_char(char)` | one character in the other case |

Functions that write into a `char` array never go past its end: they cut the text and always end it with `\0`. A string that was never set counts as empty.

## math

`import math;` — parts: `math.integer`, `math.real`, `math.random`.

### math.integer

| Function | Result |
|---|---|
| `long abs(long value)`, `long sign(long value)` | value without sign; -1, 0 or 1 |
| `long min(long a, long b)`, `long max(long a, long b)`, `long clamp(long value, long low, long high)` | |
| `long power(long base, long exponent)` | `base` to the power `exponent` (≥ 0), wrapping on overflow |
| `long gcd(long a, long b)`, `long lcm(long a, long b)` | greatest common divisor, least common multiple |
| `bool is_prime(long value)` | trial division |

They take `long`, so any integer except `ulong` can be passed; store the result in a `long` or cast it: `int m = (int)max(a, b);`.

### math.real

| Function | Result |
|---|---|
| `sqrt`, `fabs`, `fmin`, `fmax`, `hypot(x, y)` | square root (`sqrtsd`), absolute value, smaller, larger, `sqrt(x*x + y*y)` |
| `floor`, `ceil`, `trunc`, `round` | down, up, toward zero, to nearest with halves away from zero (`round(2.5)` is 3.0); exact for every `double` |
| `fmod(value, divisor)` | remainder with the sign of `value` |
| `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2(y, x)` | in radians |
| `exp`, `exp2`, `log`, `log2`, `log10`, `pow(base, exponent)` | `pow` of a negative base needs a whole exponent, else NaN |
| `PI`, `TAU`, `E` | global `double` constants |

The trigonometric, logarithm and exponent functions use the x87 floating-point unit (`fsin`, `fyl2x`, `f2xm1`, ...): about 15 correct digits; `sin`, `cos` and `tan` need `|x| < 2^63`.

### math.random

| Function | Result |
|---|---|
| `void seed_random(ulong seed)` | the same seed gives the same sequence |
| `ulong random_next()` | the next 64-bit number (xorshift64*) |
| `long random_int(long low, long high)` | a number from `low` to `high` inclusive |
| `double random_double()` | from 0.0 up to, not including, 1.0 |

Without `seed_random`, the first call seeds from the processor's time stamp counter (`rdtsc`), so every run differs. Not suitable for cryptography.

## json

Use `import json;` on Windows and Linux. To read JSON, call `json_parse` with the input text and a `JsonToken` array. A successful parse puts the root at index `0`. Use `json_member` for an object property and `json_at` for an array item. Keep the original text because tokens point to ranges inside it.

### Reading JSON

| Function | Result |
|---|---|
| `int json_parse(string source, long size, JsonToken *tokens, int capacity)` | A positive token count, or `JSON_INVALID`, `JSON_CAPACITY`, or `JSON_DEPTH`. The root is at index 0. Pass the size in bytes and capacity in tokens. Discard token contents after an error. |
| `int json_member(string source, JsonToken *tokens, int used, int object, string key)` | The token index of the first matching member, or `-1`. Pass the count returned by `json_parse` as `used`. Keys are compared after decoding escapes. |
| `int json_at(JsonToken *tokens, int used, int array, int element)` | The token index of a zero-based array item, or `-1`. To walk every item in order, start at `array + 1` and follow each token’s `next` until `tokens[array].next`. |
| `int json_decode_string(string source, JsonToken token, char target[])` | The decoded UTF-8 byte count or a negative error. A successful result is zero-terminated and can contain embedded zero bytes from `\u0000`. Use the count to preserve such bytes. |
| `int json_copy_raw(string source, JsonToken token, char target[])` | Copies the original spelling of a value and adds a terminating zero. Returns the number of bytes copied or `JSON_CAPACITY`. |
| `bool json_read_long(string source, JsonToken token, long *value)` | Reads an integer in the signed 64-bit range. Fractional, exponent, non-number, and out-of-range tokens return `false`. The original spelling is available through `json_copy_raw`. |

Tokens refer to the original input. Keep the source and token buffer alive and unchanged for lookups and decoding. Each token has `kind`, `start` (inclusive), `end` (exclusive), and `next` (the first token after the entire subtree). The kinds are `JSON_OBJECT`, `JSON_ARRAY`, `JSON_STRING`, `JSON_NUMBER`, `JSON_TRUE`, `JSON_FALSE`, and `JSON_NULL`. An object stores each key as a string token immediately before its value. Use iteration and the decoded byte count to inspect keys with embedded zeros. The parser accepts one root, up to 128 nested containers and up to 2,147,483,647 input bytes. It rejects lone UTF-16 surrogate escapes, invalid UTF-8, and a leading BOM.

### Writing JSON

Initialize a `JsonOutput` with a buffer, write an object, array or primitive value, then call `json_writer_finish` to get the byte length. Use a buffer separate from the input strings.

| Function | Result |
|---|---|
| `void json_writer_init(JsonOutput *output, char *target, long capacity)` | Starts writing into a buffer whose capacity includes the final zero. |
| `int json_write_event(JsonOutput *output, int kind, string value, int size)` | Writes one event and returns `0` or a negative error. Use `JSON_OBJECT` or `JSON_ARRAY` to begin a container, `JSON_END` to close it, `JSON_KEY` for a name, and the primitive kinds above for values. Pass a byte count for keys, strings, and numbers. For the other events pass `""` and `0`. The writer inserts separators and escapes strings. A failed event leaves the previously written prefix and writer state intact. |
| `int json_write_long(JsonOutput *output, long value)` | Writes a signed 64-bit integer, returning `0` or a negative error. |
| `long json_writer_finish(JsonOutput *output)` | The byte length of a completed, zero-terminated root, or `JSON_INVALID` if the root is unfinished. |

### Example

This reads `count` from an object and prints the same integer in a JSON array.

```c
import json;
import stdio;
import text;

int main() {
    string source = "{\"count\":42}";
    JsonToken tokens[8];
    int used = json_parse(source, length(source), tokens, tokens.length);
    if (used < 0) {
        return 1;
    }
    int index = json_member(source, tokens, used, 0, "count");
    long count;
    if (index < 0 || !json_read_long(source, tokens[index], &count)) {
        return 1;
    }

    char result[64];
    JsonOutput writer;
    json_writer_init(&writer, result, result.length);
    if (json_write_event(&writer, JSON_ARRAY, "", 0) != 0 ||
        json_write_long(&writer, count) != 0 ||
        json_write_event(&writer, JSON_END, "", 0) != 0 ||
        json_writer_finish(&writer) < 0) {
        return 1;
    }
    print(result);
    return 0;
}
```

## time

Use `import time;` on Windows and Linux. Use `time_monotonic_ns()` to measure how long something takes and `time_unix_us()` to record when it happened. Format a UTC timestamp with `time_format_utc`.

| Function | Result |
|---|---|
| `long time_monotonic_ns()` | Monotonic nanoseconds from an unspecified origin, or `-1` on failure. Subtract two readings to measure elapsed time. |
| `long time_unix_us()` | UTC microseconds since 1970-01-01T00:00:00Z, or `-1` on a clock-read failure. System clock adjustments can move this reading backward. |
| `bool time_format_utc(long microseconds, char target[])` | Writes `YYYY-MM-DDTHH:MM:SS.ffffffZ` using the Gregorian calendar. Requires 28 bytes including the terminating zero. Accepts years 0001 through 9999 and timestamps before the Unix epoch. |

The clocks use the [Windows performance counter](https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency) and [precise system time](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemtimepreciseasfiletime), or Linux [`CLOCK_MONOTONIC` and `CLOCK_REALTIME`](https://man7.org/linux/man-pages/man2/clock_gettime.2.html). Nanoseconds and microseconds are the units, and clock resolution depends on the host.

## filesystem

Use `import filesystem;`. Use `read_text` and `write_text` for small text files, or `open` to read or write in chunks. A relative path starts from the process's current directory. Use UTF-8 for paths and `/` as the separator in code that runs on both platforms.

On Linux the same functions call the kernel directly (`libraries/filesystem/*/linux/`) and behave the same, with the system's own conventions: `write_line` ends lines with `\n` (Windows: `\r\n`); `temp_directory` is `$TMPDIR` or `/tmp/`, always with a trailing `/`; `full_path` resolves `.` and `..` like `GetFullPathNameW` (without following symbolic links); `rename` copies and deletes when the target is on another file system, as Windows does; `entry_size` is 0 for folders, as on Windows. `tests/programs/files_portable.rz` runs the same checks on both systems.

### Open files

```c
import filesystem;
import stdio;

int main() {
    File input;
    if (!open(input, "notes.txt", "r")) {
        print_error("Could not open notes.txt");
        return 1;
    }

    char line[256];
    while (read_line(input, line)) {
        print(line);
    }
    close(input);
    return 0;
}
```

| Function | Does |
|---|---|
| `struct File` | an open file: its handle and a 4 KiB buffer. Declare one per file; it is passed by reference |
| `bool open(File file, string path, string mode)` | `"r"` reads an existing file, `"w"` creates or empties one, `"a"` appends (creating it if needed); `false` if it cannot be opened |
| `bool read_line(File file, char line[])` | next line without its line end, cut to fit; `false` at the end |
| `long read_bytes(File file, byte data[])` | fills `data` with the next bytes; returns how many (less at the end) |
| `void write(File file, string text)`, `void write_line(File file, string text)` | writes text. `write_line` also adds the platform's line ending. |
| `void write_bytes(File file, byte data[], long count)` | Writes up to `count` bytes from `data` to a file opened for writing or appending. A write to a read-only `File` records a failure that `flush` reports. |
| `bool seek(File file, long offset)` / `long tell(File file)` | move to / report a byte offset from the start; a refused `seek` returns `false` and changes nothing. In append mode the position starts at the end of the file |
| `bool flush(File file)` | writes what is buffered now; `false` if this or any earlier write of the file failed (the failure is remembered until the file is closed) |
| `void close(File file)` | writes what is buffered and closes. Always close a file you wrote to |

Reads fetch and writes collect 4 KiB at a time. Binary reads and writes copy contiguous chunks through the buffer. Numbers go into a file through `int_to_text` or `double_to_text`.

### Paths

| Function | Does |
|---|---|
| `bool exists(string path)`, `bool is_file(string path)`, `bool is_directory(string path)` | |
| `long file_size(string path)` | size in bytes, or -1 if there is no such file |
| `bool delete_file(string path)` | |
| `bool rename(string from, string to)` | renames or moves, replacing an existing `to`, across drives too |
| `bool copy_file(string from, string to)` | replaces an existing `to` |
| `bool create_directory(string path)` | also `true` if it already exists |
| `bool create_directories(string path)` | creates every missing folder along the path |
| `bool delete_directory(string path)` | the folder must be empty |
| `long current_directory(char target[])`, `bool change_directory(string path)` | |
| `long full_path(char target[], string path)`, `long temp_directory(char target[])` | |
| `long read_text(string path, char target[])` | the whole file into `target`, cut to fit; -1 if it cannot be opened |
| `bool write_text(string path, string text)`, `bool append_text(string path, string text)` | write or append a whole text in one call |

On Linux, a regular-file copy builds a sibling replacement and publishes it only after the copy succeeds. Copying onto the same file through a hard link or symlink returns `false`. A successful copy or cross-filesystem move replaces a destination symlink itself. The cross-filesystem move preserves ordinary permission bits and removes the source after publication.

### Listing a folder

```c
Directory folder;
char name[260];
if (open_directory(folder, ".")) {
    while (next_entry(folder, name)) {
        write_string(name);
        if (entry_is_directory(folder)) {
            print("  (folder)");
        } else {
            write_string("  ");
            print(entry_size(folder));
        }
    }
    close_directory(folder);
}
```

| Function | Does |
|---|---|
| `struct Directory` | a listing in progress |
| `bool open_directory(Directory directory, string path)` | `false` if the folder does not exist |
| `bool next_entry(Directory directory, char name[])` | next file or folder name, skipping `.` and `..`; `false` at the end |
| `bool entry_is_directory(Directory directory)`, `long entry_size(Directory directory)` | about the entry just returned |
| `void close_directory(Directory directory)` | |

The helpers `file_fill` and the struct fields are part of the library; do not reuse those names.

## windows

`import windows;` makes Windows API functions callable like razum functions. It declares the commonly used functions of `kernel32`, `user32`, `gdi32`, `shell32`, `advapi32`, `dwmapi`, CNG cryptography in `bcrypt`, and the GDI+ flat API in `gdiplus` (see [libraries/windows/](../libraries/windows)), their structs, `RGB`, and the usual constants (`MB_OK`, `GENERIC_READ`, `VK_ESCAPE`, `HKEY_CURRENT_USER`, ...). Only the functions and constants a program uses end up in its `.exe`, and only the DLLs they come from are imported.

```c
import windows;

int main() {
    int answer = MessageBoxA(0, "Save the file?", "razum", MB_YESNO + MB_ICONQUESTION);
    if (answer == IDYES) {
        Beep(880, 200);
    }
    SYSTEMTIME now;
    GetLocalTime(now);
    return now.hour;
}
```

| Windows type | razum type |
|---|---|
| `HANDLE`, `HWND`, `HDC`, `HKEY`, any pointer you only pass along | `long` (pass `0` for `NULL`); typed pointers are `T *` |
| `DWORD`, `UINT`, `COLORREF` | `uint` |
| `BOOL`, `int` | `int` (compare with `!= 0`; Windows returns values other than 1 for true) |
| `WORD`, `SHORT` | `ushort`, `short` |
| `LPCSTR` (text in) | `string` |
| `LPSTR` (text out), byte buffers | `char *buffer`, `byte *data` — pass a `char` or `byte` array; it converts to a pointer |
| `DWORD *`, `HANDLE *`, `LONGLONG *` (one value out) | `uint *written` — pass `&written` |
| A callback (`WNDPROC`, `THREAD_START_ROUTINE`, ...) | a function type: `fn(long window, uint message, ulong wparam, long lparam) -> long`; pass a razum function |
| `RECT *`, `SYSTEMTIME *`, ... | the struct: `RECT area; GetClientRect(window, area);` — a `RECT *` pointer can be passed too |

Structs keep their Windows names and exact C layouts: `POINT`, `RECT`, `FILETIME`, `SYSTEMTIME`, `WIN32_FIND_DATAA`, `STARTUPINFOA`, `PROCESS_INFORMATION`, `WNDCLASSEXA`/`WNDCLASSEXW`, `MSG`, `PAINTSTRUCT`, `BITMAPINFOHEADER`, `TRACKMOUSEEVENT`, and GDI+'s `RectF`. The library uses the `A` (ANSI) versions of functions that take text, plus the `W` window functions (`CreateWindowExW`, `DefWindowProcW`, ...) that a Unicode window needs.

### Any other function, from any DLL

Declare it with `extern` in your own file, using the same type rules:

```c
extern "winmm" uint timeGetTime();
extern "ws2_32" int WSAStartup(ushort version, byte data[]);
extern "user32" int MessageBoxA(long window, string text, string caption, uint type);
```

See [language.md](language.md#extern-functions) for the rules. From an `asm` block, `call [iat@winmm@timeGetTime]` imports a function without declaring it (`iat@Name` alone means `kernel32`).

`W` (UTF-16) functions take `ushort *` text: convert with `to_utf16(wide, text)` from [text](#text) (`ushort wide[260];`) and back with `from_utf16(target, wide)`. The common ones are declared: `CreateFileW`, `DeleteFileW`, `MoveFileExW`, `CopyFileW`, `CreateDirectoryW`, `FindFirstFileW`, `GetFullPathNameW`, `GetCommandLineW`, `WriteConsoleW`, `ReadConsoleW`, `MessageBoxW`, `SetWindowTextW`, ... `int message_box(string text, string caption, uint type)` shows a message box with UTF-8 text. Not possible yet: variable argument lists (`wsprintfA`).

## winsock

Use `import winsock;` to call the Windows socket APIs from `ws2_32.dll`. Socket handles are `long`. Socket addresses and options are byte buffers with the layout expected by Winsock. For HTTP servers, use the higher-level [http](#http) library.

| Function | Use |
|---|---|
| `WSAStartup(ushort version, byte *data)`, `WSACleanup()` | Initialize Winsock and release it when finished. `WSAStartup` returns zero on success. |
| `WSASocketW(int family, int type, int protocol, long info, uint group, uint flags)` | Creates a socket or returns `INVALID_SOCKET`. For TCP, use `AF_INET` or `AF_INET6`, `SOCK_STREAM` and `IPPROTO_TCP`. |
| `bind(long socket, byte *address, int length)`, `listen(long socket, int backlog)`, `connect(long socket, byte *address, int length)` | Assign a local address, mark a socket as listening, or connect to a peer. |
| `send(long socket, char *buffer, int length, int flags)`, `recv(long socket, char *buffer, int length, int flags)` | Send or receive bytes. A call can transfer fewer bytes than requested. A stream receive of zero, with a nonzero buffer length, means the peer closed its sending side. |
| `getsockname(long socket, byte *address, int *length)`, `getpeername(long socket, byte *address, int *length)` | Get the local or peer address and update its length. |
| `inet_pton(int family, string text, byte *address)`, `inet_ntop(int family, byte *address, char *text, ulong size)` | Convert IPv4 or IPv6 addresses between text and binary form. |
| `setsockopt(long socket, int level, int name, byte *value, int length)`, `ioctlsocket(long socket, uint command, uint *argument)` | Set socket options and controls, such as `TCP_NODELAY` or `FIONBIO`. |
| `WSARecv(...)`, `WSASend(...)`, `WSAIoctl(...)` | Start operations that can use overlapped I/O. `WSABUF` holds a byte count and a data pointer. `GUID` identifies extensions requested through `WSAIoctl`. |
| `shutdown(long socket, int how)`, `closesocket(long socket)` | Shut down one or both directions or release the socket handle. |
| `WSAGetLastError()` | Gets the last Winsock error for this thread after a failed call. |

Completion-port APIs such as `CreateIoCompletionPort` and `GetQueuedCompletionStatusEx` are declared in [windows](#windows).

## linux

`import linux;` is the Linux counterpart of `windows` and is available on Linux only. It is what the system-specific files of the other libraries use:

| Function or constant | Does |
|---|---|
| `long system_call(long number, long first, long second, long third, long fourth)` | One Linux system call with up to four arguments (`syscall` in an `asm` block, keeping `rsi` and `rdi`); the result is the kernel's return value, `-errno` on failure |
| `long system_call6(long number, long first, long second, long third, long fourth, long fifth, long sixth)` | The same with six arguments (`r10`, `r8` and `r9`) |
| `long environment_value(string name, char target[])` | Copies an environment variable into a nonempty `target`, truncates it to fit and adds a terminating zero. Returns the number of bytes copied, or `-1` if the variable is missing. |
| `SYS_*`, `O_*`, `SEEK_*`, `PROT_*`, `MAP_*`, ... | System call numbers and flags as `const long` |

Linux library code passes text as `(long)path` and buffers as `(long)&buffer[0]`. A negative result is `-errno`; `EINTR` (`-4`) means try again. The library is not available on Windows, where `windows` and `winsock` take its place.

## gui

`import gui;` opens a window and draws an immediate-mode user interface into it: each frame, the program calls one function per element, in order, and reads back what the user did. There are no widget objects, callbacks or event handlers to wire up; state lives in the program's own variables.

```c
import gui;

bool dark = true;
double volume = 0.5;
char name[64];
int clicks = 0;

int main() {
    if (!gui_open("My app", 640, 420)) {
        return 1;
    }
    while (gui_frame()) {                       // false once the window is closed
        gui_heading("Settings");
        gui_begin_panel(0);
        gui_input(name, "Your name");
        if (gui_switch("Dark mode", &dark)) {   // true in the frame it was toggled
            gui_set_dark(dark);
        }
        gui_slider(&volume, 0, 1);
        gui_begin_row();
        if (gui_button("Count")) {              // true in the frame it was clicked
            clicks++;
        }
        if (gui_button_quiet("Reset")) {
            clicks = 0;
        }
        gui_end();
        gui_end();
    }
    gui_close();
    return 0;
}
```

[examples/gui_demo.rz](../examples/gui_demo.rz) is a complete two-panel window.

The same widget and layout calls work on Windows and Linux. On Linux, the library tries Wayland first using `xdg-shell` and presents a double-buffered software surface through `wl_shm`. When no Wayland display opens, it tries a local X11 socket using `DISPLAY` and `XAUTHORITY`. X11 window decorations come from the window manager. The Wayland client draws a title bar and asks the compositor to move or resize the window. [Desktop compatibility](platforms.md#linux-desktop-sessions)

How it looks and behaves:

- Both platforms draw into an offscreen buffer. Windows uses anti-aliased GDI+ and ClearType with Segoe UI. Linux draws the same commands in software with an embedded Latin-1 bitmap font and uses a replacement glyph for other characters.
- Rounded corners, soft panel shadows, hover and press states, and eased motion: buttons, switches, checkboxes, sliders, progress bars and focus rings move between states instead of jumping. A theme change fades over a quarter of a second.
- Both platforms expose light and dark themes. Windows starts with the system app mode and scales for each monitor. Linux starts with the light theme and uses one pixel per layout unit, so the compositor may scale the surface on high-DPI displays.
- Text buffers are UTF-8. Linux uses the compositor keyboard map on Wayland and the server key map on X11. Basic keys, modifiers, and Wayland layout groups are supported. Input method composition, key repeat on Wayland, and full Unicode font rendering remain open.
- Idle frames wait for input or a text caret blink. Windows animations follow the display refresh, and Linux animations are scheduled at roughly 60 frames per second when there is no input.

### Window and frame

| Function | Does |
|---|---|
| `bool gui_open(string title, int width, int height)` | Opens a window with a client area of `width` × `height` units; `false` if the display or renderer cannot be opened |
| `bool gui_frame()` | Draws the previous frame, waits for input or the next animation step, and starts a new frame; `false` once the user closes the window |
| `void gui_close()` | Destroys the window and frees everything; `gui_open` may be called again |
| `void gui_set_dark(bool dark)` | Switches the theme (with a fade once frames run); `gui.dark` holds the current choice |
| `void gui_set_accent(uint color)` | Sets the accent colour, as `0xAARRGGBB` (default `0xFF6366F1`) |
| `bool gui_open_offscreen(int width, int height)` | The same without a window, for tests and image rendering; `gui_frame` then never waits |
| `uint gui_pixel(int x, int y)` | A pixel of the last drawn frame, as `0xAARRGGBB`; `0` outside the buffer |

### Layout

Elements are placed one after another inside the current container. The window itself is a column with 24 units of padding and 12 between elements.

| Function | Does |
|---|---|
| `void gui_begin_panel(double width)` | A card: rounded surface, border and shadow, with 20 units of padding; children go in a column |
| `void gui_begin_row()` | Children go side by side, 12 units apart, vertically centred on a 36-unit line |
| `void gui_begin_column(double width)` | Children go below each other, 12 units apart |
| `void gui_end()` | Closes the innermost panel, row or column |
| `double gui_column_width(int count)` | Width for `count` equal containers that share the rest of the current row |
| `double gui_available_width()` | Width left for the next element |
| `void gui_space(double size)` | Empty space |

A `width` of `0` (or more than fits) means all the remaining width. After each element, `gui.slot_x`, `gui.slot_y`, `gui.slot_width` and `gui.slot_height` hold where it was placed.

### Elements

| Function | Draws | Returns |
|---|---|---|
| `void gui_heading(string text)` | Large semibold text | |
| `void gui_strong(string text)` | Semibold body text | |
| `void gui_text(string text)` | Body text; in a column it wraps at the column's width | |
| `void gui_muted(string text)` | Small, dimmer text; wraps like `gui_text` | |
| `bool gui_button(string label)` | A filled accent button | `true` in the frame it is clicked |
| `bool gui_button_quiet(string label)` | An outlined, neutral button | `true` in the frame it is clicked |
| `bool gui_checkbox(string label, bool *value)` | A checkbox with an animated check mark | `true` when `*value` was toggled |
| `bool gui_switch(string label, bool *value)` | A toggle switch with a sliding knob | `true` when `*value` was toggled |
| `bool gui_slider(double *value, double minimum, double maximum)` | A track and a draggable knob | `true` while dragging changes `*value` |
| `bool gui_input(char text[], string placeholder)` | A one-line text field editing the `char` array in place | `true` when the text changed |
| `void gui_progress(double fraction)` | A bar filled to `fraction` (0 to 1), easing to new values | |
| `void gui_separator()` | A thin line | |

`gui_input` supports typing, Backspace, Delete, Left, Right, Home and End, and clicking to place the caret; Enter or Escape, or a click elsewhere, ends editing. The array's size limits the text.

Each element's hover and animation state is kept by its position in the frame (the first element is 0, the next 1, ...), so a program should call the same elements in the same order every frame, adding or removing them only at the end or between frames where a brief animation jump does not matter.

## threads

`import threads;` runs code on real operating-system threads, in parallel on every core, and lets threads talk through **channels** in the style of Go: one thread sends values, another receives them, and the channel does the waiting and locking. Mutexes, wait groups and atomic operations are there for the cases where shared memory is simpler.

It works on Windows and Linux. The channel, mutex and wait-group code is shared and calls the Windows lock functions by name (`AcquireSRWLockExclusive`, `SleepConditionVariableSRW`, ...): on Windows those are kernel32 directly, on Linux `libraries/threads/system/linux/` provides the same names on futexes, so Windows pays nothing for Linux. On Linux a thread runs on its own 1 MiB stack from `mmap` with an inaccessible guard page below it (running past the end stops the program, as a stack overflow does on Windows) and is started with `clone`. `thread_join` frees the stack; a `thread_detach`ed thread's stack is freed by the next `thread_start` after the thread ended.

```c
import stdio;
import threads;

Channel *jobs;
Channel *results;

void square_numbers(long unused) {
    long value = 0;
    while (channel_receive(jobs, &value)) {     // false once jobs is closed and empty
        channel_send(results, value * value);
    }
}

int main() {
    jobs = channel_new(16);
    results = channel_new(16);
    long worker = thread_start(square_numbers, 0);
    for (long i = 1; i <= 3; i++) {
        channel_send(jobs, i);
    }
    channel_close(jobs);
    long sum = 0;
    long value = 0;
    for (int i = 0; i < 3; i++) {
        channel_receive(results, &value);
        sum += value;
    }
    thread_join(worker);
    print(sum);                                 // 14
    return 0;
}
```

[examples/worker_pool.rz](../examples/worker_pool.rz) spreads a prime count over one worker per core.

### Starting and joining threads

| Function | Does |
|---|---|
| `long thread_start(fn(long argument) work, long argument)` | Runs `work(argument)` on a new thread and returns its handle, or 0 if Windows refused. Pass several values by pointing `argument` at a struct: `(long)&settings` |
| `void thread_join(long thread)` | Waits until the thread's function returns, then releases the handle |
| `void thread_detach(long thread)` | Releases the handle of a thread nobody will join; it keeps running |
| `int cpu_count()` | Logical processors in all processor groups |
| `void thread_sleep(uint milliseconds)`, `void thread_yield()` | Pause; give the core to another thread |

Threads are the operating system's, not green threads: starting one costs about 90 µs, so for many small tasks start one worker per core and feed them through a channel, as the example does. Each thread gets the program's 8 MiB stack reservation (4 KB committed).

### Channels

A channel carries `long` values: numbers, or pointers cast with `(long)`. It is safe to use from any number of threads at once.

| Function | Does |
|---|---|
| `Channel *channel_new(int capacity)` | A channel that holds up to `capacity` values; `0` makes it **unbuffered**: each send waits until a receiver took the value, as in Go |
| `bool channel_send(Channel *channel, long value)` | Waits for room, then sends; `false` if the channel is closed |
| `bool channel_receive(Channel *channel, long *value)` | Waits for a value; `false` once the channel is closed and empty |
| `ChannelResult channel_receive_timeout(Channel *channel, long *value, uint milliseconds)` | `ChannelResult.RECEIVED`, `TIMED_OUT` or `CLOSED`; `INFINITE` waits forever, `0` only checks (`EMPTY`) |
| `bool channel_try_send(...)`, `bool channel_try_receive(...)` | Never wait; `false` if it would have to |
| `int channel_select(Channel *channels[], long *value, uint milliseconds)` | Waits on several channels at once, like Go's `select`: returns the index of the channel that delivered into `*value`, `-1` when all are closed and empty, or `-2` after `milliseconds` (`0` polls, `INFINITE` waits). When several are ready, the start rotates so none starves |
| `void channel_close(Channel *channel)` | No more sends; receivers drain what is left, then get `false` / `CLOSED`; waiting senders and receivers wake up |
| `int channel_length(Channel *channel)` | Values waiting right now |
| `void channel_free(Channel *channel)` | Frees a channel no thread uses any more |

### Shared memory

| Function | Does |
|---|---|
| `Mutex`, `mutex_lock(m)`, `mutex_unlock(m)`, `bool mutex_try_lock(m)` | A lock (a Windows slim reader/writer lock). A `Mutex` variable or field starts unlocked; no setup needed |
| `WaitGroup`, `wait_group_add(g, n)`, `wait_group_done(g)`, `wait_group_wait(g)` | Count running jobs, wait until all are done |
| `long atomic_add(long *target, long delta)` | Adds and returns the new value, in one indivisible step (`lock xadd`) |
| `long atomic_load(long *target)`, `void atomic_store(long *target, long value)` | Read or write a value other threads change |
| `long atomic_exchange(long *target, long value)` | Stores and returns the old value |
| `bool atomic_compare_exchange(long *target, long expected, long desired)` | Stores `desired` only if the value is still `expected` |

### Printing from threads

Every `print` and `write_...` call writes its text whole: two threads never mix the characters of one call. Separate calls from different threads can interleave, so build a line in a `char` array and print it with one call when it must stay together.

## http

`import http;` serves HTTP/1.1 from a razum program. You register a function per path; the library listens, parses, keeps connections alive, answers pipelined requests, serves files and times out idle or slow clients. It is built for many connections on few cores (measurements below).

On Linux the same functions work with an epoll loop per worker thread instead of I/O completion ports (`libraries/http/*/linux/`): every worker waits on the listening sockets with `EPOLLEXCLUSIVE`, owns the connections it accepts (no locks), sends what fits and waits for `EPOLLOUT` for the rest, and closes idle or slow connections every 250 ms. IPv4 and IPv6 addresses work as on Windows (`::1`, `[::]`, `::ffff:1.2.3.4`; an IPv6 listener also accepts IPv4), and Ctrl+C stops a running server cleanly (a second Ctrl+C after it stopped ends the program). HTTPS on Linux is a TLS server written in razum (`libraries/http/tls/linux/`): TLS 1.3 (X25519, TLS_AES_128_GCM_SHA256, the RFC 8446 key schedule) and TLS 1.2 (ECDHE-ECDSA-AES128-GCM-SHA256 with X25519, secure renegotiation indication), with an ECDSA P-256 certificate from a `.pfx` file (legacy or AES-encrypted). `tests/test_https.sh` runs the same curl checks on both systems. Not yet: RSA certificates, session resumption, HelloRetryRequest (clients that send no X25519 key share). Parsing, routing and responses are the same code on both systems. `tests/programs/http_server_linux.rz` checks routes, bodies, files, keep-alive and a 300 KB response with `curl`.

```c
import http;

void hello(HttpRequest *request, HttpResponse *response) {
    http_text(response, "Hello, World!");
}

int main() {
    HttpServer *server = http_new();
    http_route(server, "GET", "/", hello);
    http_listen(server, "127.0.0.1", 8080);
    http_run(server);                   // until Ctrl+C
    return 0;
}
```

[examples/http_hello.rz](../examples/http_hello.rz) adds a query parameter, a POST echo, a counter and a static folder.

### Listening

`http_listen(server, host, port)` may be called several times before the server starts (up to 8 addresses). `port` 0 lets Windows choose; `http_port(server, index)` tells which.

| `host` | Reachable from |
|---|---|
| `"127.0.0.1"`, `"localhost"` | this computer only |
| `"0.0.0.0"`, `"*"`, `""` | every IPv4 address of this computer: the LAN and, if routed, the internet |
| `"192.168.1.20"` (any address of this computer) | that network only |
| `"::"` | every IPv6 **and** IPv4 address (dual stack) |
| `"::1"`, `"[::1]"` | IPv6 loopback |

The first time a program listens on an address other than loopback, Windows Firewall asks whether to allow it. Listening sockets use `SO_EXCLUSIVEADDRUSE`, so no other program can take over the port while it runs.

### HTTPS

```c
http_listen_tls(server, "0.0.0.0", 443, "site.pfx", "pfx password");
```

`bool http_listen_tls(server, string host, int port, string certificate_file, string password)` listens like `http_listen` but speaks TLS 1.3 and 1.2 through Windows' own TLS stack (SChannel); HTTP and HTTPS addresses can be mixed on one server. On failure it returns `false` and `http_last_error` holds the Windows or SChannel code (for example `0x8009030E`: the certificate has no usable private key).

The certificate is a `.pfx` (PKCS #12) file with its private key, as exported by Windows, `openssl pkcs12 -export`, or ACME clients such as win-acme for Let's Encrypt. Windows' TLS service can only use a private key that is stored on disk, so `http_listen_tls` imports the key into the user's key store, lets SChannel load it, and deletes it from the store again at once; nothing is left behind (the test checks this).

A certificate for local development, valid for `localhost` and `127.0.0.1` (PowerShell; it adds the certificate to your store, exports it, and removes it again):

```powershell
$certificate = New-SelfSignedCertificate -DnsName localhost, 127.0.0.1 -CertStoreLocation Cert:\CurrentUser\My -KeyAlgorithm ECDSA_nistP256 -NotAfter (Get-Date).AddYears(5)
Export-PfxCertificate -Cert $certificate -FilePath dev.pfx -Password (ConvertTo-SecureString "dev" -AsPlainText -Force)
Remove-Item "Cert:\CurrentUser\My\$($certificate.Thumbprint)" -DeleteKey
```

Browsers warn about a self-signed certificate until you trust it; `curl -k` skips the check. `tests/https/localhost.pfx` (password `razum-test`) is such a certificate, for the tests only.

### Server

| Function | Does |
|---|---|
| `HttpServer *http_new()` | A server, or `null` if Winsock or the completion port could not start |
| `bool http_route(server, string method, string path, handler)` | Calls `handler(request, response)` for that method and path. `method` `"*"` matches every method; `GET` routes also answer `HEAD`. A path ending in `*` matches every path with that start, and `request.rest` holds the part after it. Routes are tried in the order they were added; up to 64 |
| `bool http_files(server, string prefix, string folder)` | Serves the files below `folder` under `prefix` (`"/static/"`), with the content type from the extension and `index.html` for folders. Paths with `..`, `\\` or `:` get 403 |
| `bool http_listen(server, string host, int port)` | Adds an address (table above) |
| `bool http_listen_tls(server, host, port, certificate_file, password)` | Adds an HTTPS address (see [HTTPS](#https)) |
| `void http_workers(server, int count)` | Worker threads (default: `cpu_count()`) |
| `void http_timeouts(server, uint idle_ms, uint request_ms)` | Close keep-alive connections idle for `idle_ms` (default 60000) and clients that take longer than `request_ms` to send a request or receive a response (default 10000, the slow-loris guard) |
| `void http_max_body(server, long bytes)` | Largest request body (default 1 MiB); larger ones get 413 |
| `bool http_run(server)` | Starts and blocks until `http_stop` or Ctrl+C |
| `bool http_start(server)`, `void http_wait(server)` | The same in two halves, so the starting thread can do other work |
| `void http_stop(server)` | Stops listening and lets the workers end; `http_wait` then closes every connection |
| `long http_request_count(server)`, `long http_connection_count(server)` | Requests answered so far; connections open now |

### Requests

A handler receives `HttpRequest *request`. Its strings stay valid until the handler returns.

| Field or function | Gives |
|---|---|
| `request.method`, `request.path`, `request.query` | `"GET"`, `"/users/42"` (as sent, not percent-decoded), `"page=2&sort=name"` |
| `request.rest` | For a route ending in `*`: the path after the prefix |
| `request.body`, `request.body_length` | The body bytes (`char *`), not `\0`-terminated |
| `string http_header(request, string name)` | A header value, name in any case; `""` if absent |
| `bool http_query_value(request, string key, char target[])` | One query parameter, percent- and `+`-decoded into `target` |
| `long http_body_text(request, char target[])` | Copies the body as text into `target` |

### Responses

The response starts as `200 OK`, `text/plain; charset=utf-8`, empty body. Status, headers and body can be set in any order.

| Function | Does |
|---|---|
| `http_text(response, text)`, `http_html(response, html)`, `http_json(response, json)` | Sets the body and its content type |
| `http_write(response, text)`, `http_write_long(response, number)`, `http_write_bytes(response, data, length)` | Appends to the body |
| `http_status(response, int status)` | Any status from 100 to 999; the reason text is added for the common ones |
| `http_content_type(response, type)` | Overrides the content type |
| `bool http_set_header(response, name, value)` | Adds a header (up to 16; names and values are copied) |
| `http_redirect(response, location)` | 302 with `Location` |
| `http_close_after(response)` | Closes the connection after this response |

Every response gets `Server`, `Date`, `Content-Length` (or none for 1xx, 204 and 304) and, when the connection closes, `Connection: close`.

### Handler concurrency

Handlers run on the worker threads, several at once. Shared variables need `atomic_add` or a `Mutex` from the [threads](#threads) library; locals and the request and response are private to the call.

### What the library does for you

- HTTP/1.1 keep-alive and HTTP/1.0; pipelined requests are answered in order with one send.
- Request bodies by `Content-Length`, arriving in any number of pieces; `Expect: 100-continue` is answered.
- `400` for malformed requests, `431` for headers over 16 KiB or more than 32 headers, `413` for large bodies, `501` for `Transfer-Encoding: chunked` request bodies (not supported yet), `404` without a route.

### How it scales

One I/O completion port is shared by the worker threads, which take up to 128 completions per call. Accepted sockets are non-blocking: an idle connection keeps only a zero-byte receive posted, so it holds no buffer; when data arrives, a worker reads it into its own 64 KiB buffer, answers every complete request, and sends all answers at once. Only a half-received request gets its own buffer. The `Date` header is formatted four times a second by the timeout thread, not per request. Measurements (in KNOWLEDGE.md): about 1.5 million pipelined or 160,000 single requests per second on two logical cores (HTTPS: 1.07 million and 117,000), and under 1 KB of process memory per idle connection, so the limit for a million connections is the kernel's few kilobytes per socket, not CPU.

## crypt

`import crypt;` covers what Python's `hashlib`, `hmac`, `secrets`, `uuid`, `base64` and `binascii` do: hashes, HMAC, PBKDF2, AES encryption, secure random numbers, tokens and UUIDs, base64/32/16/85, xor, CRC-32 and Adler-32. Parts: `crypt.hash`, `crypt.hmac`, `crypt.kdf`, `crypt.cipher`, `crypt.random`, `crypt.encoding`, `crypt.bytes`. Windows uses CNG (`bcrypt.dll`) for primitives and random bytes. Linux implements the primitives in Razum and uses `getrandom` for secure random bytes. The encodings and checksums are plain Razum on both.

On Linux, AES encryption uses AES-NI and GCM authentication uses PCLMULQDQ with SSSE3 when the running CPU supports them. Older CPUs use scalar implementations automatically. The Linux key schedule and scalar AES routines use table lookups. [Linux AES-GCM measurements](../KNOWLEDGE.md#linux-aes-gcm-phase-timings)

```c
import stdio;
import text;
import crypt;

int main() {
    char hex[129];
    hash_text_hex(Hash.SHA256, hex, "abc");
    print(hex);                                 // ba7816bf...f20015ad

    string password = "correct horse";
    byte salt[16];
    byte key[32];
    secure_bytes(salt, salt.length);
    pbkdf2(Hash.SHA256, key, as_bytes(password), length(password), salt, salt.length, 600000);

    string message = "meet at noon";
    byte nonce[AES_GCM_NONCE_SIZE];
    byte sealed[64];
    byte opened[64];
    secure_bytes(nonce, nonce.length);
    long sealed_size = aes_gcm_encrypt(sealed, key, key.length, nonce, null, 0, as_bytes(message), length(message));
    long opened_size = aes_gcm_decrypt(opened, key, key.length, nonce, null, 0, sealed, sealed_size);
    print(opened_size);                         // 12; -1 if the data or tag was changed

    char text64[128];
    base64_encode(text64, sealed, sealed_size);
    print(text64);                              // 40 characters, different every run

    char id[37];
    uuid4(id);
    print(id);                                  // for example 1b4e28ba-2fa1-4d3b-a3f5-ef2f0b6e1a9c
    return 0;
}
```

The same rules hold in every part:

- The result goes first, into an array: `byte digest[]`, `char target[]`. The library checks the array's size and never writes past it.
- Input bytes are a pointer and a count: `byte *data, long length`. A `byte` array passes as it is; text passes as `as_bytes(text), length(text)`. A `char` array cannot be cast to `byte *` directly; `as_bytes` does it.
- Functions return the number of bytes or characters they produced, or `-1` for bad input, a target that is too small, or a failed check. Text results always end with `\0`, and are empty after `-1`.

### crypt.hash

`Hash` is an enum: `Hash.MD5`, `Hash.SHA1`, `Hash.SHA256`, `Hash.SHA384`, `Hash.SHA512`.

| Function | Result |
|---|---|
| `long hash(Hash kind, byte digest[], byte *data, long length)` | the digest of `data`; returns its size |
| `long hash_text(Hash kind, byte digest[], string text)` | the digest of a text |
| `long hash_hex(Hash kind, char target[], byte *data, long length)` / `long hash_text_hex(Hash kind, char target[], string text)` | the digest as lowercase hex, like Python's `hexdigest()`; `target` needs `2 * size + 1` characters |
| `long md5(byte digest[], byte *data, long length)`, `sha1`, `sha256`, `sha384`, `sha512` | `hash` with that kind |
| `int hash_size(Hash kind)` | 16, 20, 32, 48 or 64 bytes |
| `struct Hasher` | a hash in progress, for data that arrives in parts |
| `bool hash_start(Hasher hasher, Hash kind)` | begins one |
| `bool hash_update(Hasher hasher, byte *data, long length)` | adds bytes; call it as often as needed |
| `long hash_finish(Hasher hasher, byte digest[])` | writes the digest and ends the `Hasher` (also on `-1`); start it again to reuse it |

MD5 and SHA-1 are broken for signatures and certificates; use them only for checksums and old protocols.

### crypt.hmac

| Function | Result |
|---|---|
| `long hmac(Hash kind, byte mac[], byte *key, long key_length, byte *data, long length)` | the HMAC of `data` under `key` (any length); returns its size |
| `long hmac_hex(Hash kind, char target[], byte *key, long key_length, byte *data, long length)` | the same as lowercase hex |
| `bool hmac_start(Hasher hasher, Hash kind, byte *key, long key_length)` | begins an HMAC in parts; continue with `hash_update` and `hash_finish` |
| `bool compare_digest(byte *left, long left_length, byte *right, long right_length)` | compares two MACs in constant time (in `crypt.bytes`); use it instead of a loop that stops at the first difference |

### crypt.kdf

| Function | Result |
|---|---|
| `long pbkdf2(Hash kind, byte key[], byte *password, long password_length, byte *salt, long salt_length, long iterations)` | fills all of `key` with PBKDF2-HMAC; returns `key.length`. `iterations` must be at least 1 |
| `long hkdf_extract(Hash kind, byte secret[], byte *salt, long salt_length, byte *input, long input_length)` | HKDF-Extract (RFC 5869): the pseudorandom key in `secret`, `hash_size(kind)` bytes; an empty salt means zeros |
| `long hkdf_expand(Hash kind, byte output[], long length, byte *secret, long secret_length, byte *info, long info_length)` | HKDF-Expand: `length` bytes (at most 255 × the hash size) into `output`; `-1` when too long |
| `void x25519(byte *shared, byte *scalar, byte *point)` / `void x25519_public(byte *public_key, byte *private_key)` | X25519 key exchange (RFC 7748), 32-byte values: the shared secret of a private scalar and the other side's public point, and the public key of a private key. Plain razum on both systems (`crypt/exchange/`) |
| `bool p256_public(byte *public_key, byte *private_key)` | The uncompressed P-256 public key (65 bytes, `04` then x and y) of a 32-byte private key; `false` for a key outside 1 .. n - 1 |
| `long ecdsa_p256_sign(byte signature[], byte *private_key, byte *digest)` | ECDSA P-256 signature of a 32-byte SHA-256 digest, DER-encoded as TLS and X.509 use it (at most 72 bytes); the nonce is deterministic (RFC 6979), so no random numbers are needed. Plain razum on both systems (`crypt/sign/`); not constant-time |
| `bool pfx_parse(Pfx pfx, byte *data, long length, string password)` | Reads a PKCS#12 (`.pfx`/`.p12`) file already in memory: checks its MAC (a wrong password gives `false`), decrypts it (legacy SHA-1 with Triple-DES, or PBES2 with PBKDF2 and AES-128/192/256-CBC), and fills `pfx.private_key` (a P-256 key, 32 bytes) and `pfx.certificates` (DER, `pfx.certificate_count` of them, at `certificate_starts[i]` with `certificate_lengths[i]`; the certificate that matches the key comes first). RSA keys are not read yet. Plain razum on both systems (`crypt/certificate/`) |
| `long des3_cbc_decrypt(byte target[], byte *key, byte *iv, byte *data, long length)` | Triple-DES (EDE, 24-byte key) CBC decryption with PKCS#7 padding, for legacy files; returns the plain length or `-1` |

For stored passwords, OWASP recommends 600,000 iterations with SHA-256 and a random 16-byte salt from `secure_bytes`.

### crypt.cipher

| Function | Result |
|---|---|
| `long aes_gcm_encrypt(byte target[], byte *key, long key_length, byte *nonce, byte *aad, long aad_length, byte *data, long length)` | AES-GCM: `target` gets the ciphertext followed by the 16-byte tag, as in Python's `AESGCM.encrypt`; returns `length + 16` |
| `long aes_gcm_decrypt(byte target[], byte *key, long key_length, byte *nonce, byte *aad, long aad_length, byte *data, long length)` | checks the tag and decrypts; returns `length - 16`, or `-1` (and zeroes `target`) if the key, nonce, data, tag or `aad` differ |
| `long aes_cbc_encrypt(byte target[], byte *key, long key_length, byte *iv, byte *data, long length)` | AES-CBC with PKCS#7 padding; returns the padded size, `(length / 16 + 1) * 16` |
| `long aes_cbc_decrypt(byte target[], byte *key, long key_length, byte *iv, byte *data, long length)` | removes the padding; `-1` if it is wrong; `target` needs `length` bytes |
| `long aes_ctr(byte target[], byte *key, long key_length, byte *counter, byte *data, long length)` | AES-CTR with a 16-byte starting counter block that counts up big-endian; the same call encrypts and decrypts, and `target` may be `data` |
| `AES_BLOCK_SIZE`, `AES_GCM_NONCE_SIZE`, `AES_GCM_TAG_SIZE` | 16, 12, 16 |

The key is 16, 24 or 32 bytes (AES-128, -192, -256). `aad` is extra data that is authenticated but not encrypted; pass `null, 0` for none. Never use the same nonce twice with one key: take a fresh one from `secure_bytes` for each message and send it along. CBC and CTR do not detect changes to the data; prefer GCM.

### crypt.random

Numbers come from the system's cryptographic generator (`BCryptGenRandom`), so they are safe for keys, tokens and passwords, unlike `math.random`. Every function is safe to call from several threads.

| Function | Result |
|---|---|
| `bool secure_bytes(byte *target, long length)` | fills `length` bytes, like Python's `secrets.token_bytes` |
| `ulong secure_ulong()` | 64 random bits |
| `long secure_int(long low, long high)` | from `low` to `high` inclusive, every value equally likely; `low` if `high <= low` |
| `long token_hex(char target[], long count)` | `count` random bytes as `2 * count` hex characters, like `secrets.token_hex` |
| `long token_urlsafe(char target[], long count)` | `count` random bytes as unpadded URL-safe base64, like `secrets.token_urlsafe` |
| `long uuid4(char target[])` | a random UUID, `xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx`; `target` needs 37 characters |

`secure_ulong` and `secure_int` cannot report a failure, so if the generator ever failed they would print an error and end the program with exit code 3.

### crypt.encoding

| Function | Result |
|---|---|
| `long base64_encode(char target[], byte *data, long length)` / `long base64_decode(byte target[], string text)` | standard base64 (`+/`, with `=` padding), RFC 4648 |
| `long base64_url_encode(...)` / `long base64_url_decode(...)` | the URL- and file-safe alphabet (`-_`), with padding |
| `long base64_encode_with(char target[], byte *data, long length, bool url_safe, bool padding)` | either alphabet, with or without padding (JWT uses `true, false`) |
| `long base64_decode_with(byte target[], string text, bool url_safe)` | either alphabet |
| `long base32_encode(char target[], byte *data, long length)` / `long base32_decode(byte target[], string text)` | base32, RFC 4648; decoding also accepts lowercase |
| `long hex_encode(char target[], byte *data, long length)` / `long base16_encode(...)` | lowercase / uppercase (RFC 4648 base16) hex |
| `long hex_decode(byte target[], string text)` | hex in either case |
| `long base85_encode(char target[], byte *data, long length)` / `long base85_decode(byte target[], string text)` | base85 with the RFC 1924 alphabet, the same as Python's `base64.b85encode` and Git's binary patches |

Sizes for `target`: base64 `4 * ((length + 2) / 3) + 1` characters, base32 `8 * ((length + 4) / 5) + 1`, hex `2 * length + 1`, base85 `5 * ((length + 3) / 4) + 1`. The decoders accept text with or without the `=` padding, but reject anything else that is not in the alphabet, including spaces and line breaks.

### crypt.bytes

| Function | Result |
|---|---|
| `byte *as_bytes(string text)` | the text's bytes, for passing text where `byte *` is expected |
| `void xor_bytes(byte *target, byte *left, byte *right, long length)` | `target[i] = left[i] ^ right[i]`; `target` may be one of the inputs |
| `bool xor_key(byte *target, byte *data, long length, byte *key, long key_length)` | xor with a repeating key; the same call undoes it; `false` for an empty key |
| `uint crc32(uint crc, byte *data, long length)` | CRC-32 as in zip, PNG and Python's `zlib.crc32`; start with 0 and pass the previous result to continue: `crc32(0, as_bytes("123456789"), 9)` is `0xcbf43926` |
| `uint adler32(uint adler, byte *data, long length)` | Adler-32 as in zlib; start with 1 |
| `bool compare_digest(byte *left, long left_length, byte *right, long right_length)` | equal lengths and bytes, in time that does not depend on where they differ |

The library needs Windows 10 or later: it uses CNG's algorithm pseudo-handles (`BCRYPT_SHA256_ALG_HANDLE`, ...), which need no opening or closing and are safe to share between threads.

## process

`import process;` runs another program, gives it input, and collects what it prints (standard output and standard error together), with a time limit, an output limit and a memory limit. It is what the learning website uses to compile and run code.

On Linux the command line is split with the Windows rules (spaces separate, double quotes group, `\"` is a quote) and the program is found on `PATH` and started directly, as `CreateProcessW` does; shell syntax needs `sh -c "..."`, as it needs `cmd /c` on Windows. A program that cannot be started makes `process_run` return `false` with `started` false; one stopped for its time or output limit reports exit code 1; the whole process group is stopped. `max_memory` is the address-space limit (`RLIMIT_AS`). `max_processes` sets `RLIMIT_NPROC` to the user's current number of tasks plus the limit, the closest Linux has to a job object's process limit (the kernel does not apply it to root).

```c
import stdio;
import process;

int main() {
    ProcessLimits limits;
    limits.timeout_milliseconds = 5000;
    limits.max_output = 65536;
    limits.max_memory = 256 * 1048576;
    limits.max_processes = 4;
    ProcessResult result;
    if (process_run(result, "cmd /c sort", "pear\napple\n", "", limits)) {
        print(result.output);                  // apple, pear
        print(result.exit_code);               // 0
    }
    process_free(result);
    return 0;
}
```

| Function or field | Does |
|---|---|
| `bool process_run(ProcessResult result, string command, string input, string directory, ProcessLimits limits)` | Starts `command` (a command line, UTF-8; the program is found on `PATH`) in `directory` (`""` for the current one), writes `input` (up to 64 KiB) to its standard input, and waits. `false` if it could not start |
| `result.output`, `result.output_length` | Everything it printed, ending with `\0` |
| `result.exit_code`, `result.milliseconds` | Its exit code and run time |
| `result.timed_out`, `result.truncated` | It was stopped after `timeout_milliseconds`, or after printing more than `max_output` bytes |
| `limits.max_memory`, `limits.max_processes` | Memory per process in bytes and processes at once (0: no limit), enforced by a Windows job object, which also ends every process it started when the run ends |
| `void process_free(ProcessResult result)` | Frees the output |

## program

The compiler links the program runtime as needed on Windows and Linux. It reads command-line arguments and backs the language's heap allocation operations. No import is required.

| Language feature | Use |
|---|---|
| `int main(string args[])` | Receive the command line as an array of strings. `args[0]` is the program name and `args.length` is the number of entries. Check the length before indexing. |
| `new T`, `new T[n]` | Allocate zero-initialized memory for one value or `n` values. The result is a pointer or `null` if allocation fails. |
| `free(pointer)` | Release memory from `new`. `free(null)` has no effect. |

```c
import stdio;

int main(string args[]) {
    if (args.length == 2) {
        print(args[1]);
    } else {
        print("Pass one argument");
    }
    return 0;
}
```

See [functions](language.md#functions) for the entry point and [pointers](language.md#pointers) for allocation and ownership.

## Assembly routines

The `.asm` files under `libraries/*/assembly/` define routines named `rz@name`. They are linked into a program only when something references them, and are always available to `asm` blocks without an `import`. All follow the Windows x64 calling convention.

| Routine | Input | Output |
|---|---|---|
| `rz@write` | `rcx` address, `rdx` byte count | appends to the output buffer |
| `rz@flush` | | sends the output buffer to standard output |
| `rz@write_handle` | `rcx` address, `rdx` byte count | writes to standard output directly |
| `rz@write_string`, `rz@write_newline`, `rz@write_int`, `rz@write_uint`, `rz@write_double`, `rz@write_float` | `rcx` text / value, `xmm0` for floating | |
| `rz@write_error` | `rcx` text | writes to standard error |
| `rz@read_line` | `rcx` buffer, `rdx` its size | `rax` length, or -1 at the end of input |
| `rz@read_int` / `rz@read_double` / `rz@parse_int` | — / — / `rcx` text | `rax` / `xmm0` / `rax` |
| `rz@text_length`, `rz@text_compare` | `rcx` (and `rdx`) texts; null counts as empty | `rax` / `eax` -1, 0, 1 |
| `rz@text_copy`, `rz@text_append` | `rcx` target, `rdx` its size, `r8` source | `rax` resulting length |
| `rz@text_from_int`, `rz@text_from_uint`, `rz@text_from_real` | `rcx` target, `rdx` its size, `r8` value (`xmm0` and `r8d` digits for real) | `rax` length |
| `rz@round_with` | `xmm0` value, `ecx` mode (0 nearest, 1 down, 2 up, 3 toward zero) | `xmm0` |
| `rz@arguments` | | `rax` argument strings, `rdx` count; used by `main(string args[])` |
| `rz@format_int`, `rz@format_uint`, `rz@format_real`, `rz@pow10`, `rz@parse_number`, `rz@parse_double` | helpers of the routines above | |

## Adding a library

1. Make a folder under `libraries/`, for example `libraries/geometry/`.
2. Write each function in its own `.rz` file named after it: `libraries/geometry/area.rz`. Group related files in subfolders if you like; `import geometry;` loads the whole tree, `import geometry.shapes;` one subfolder.
3. Use other libraries with `import` at the top of the file (`import text;`, `import windows;`).
4. For Windows functions not in `windows`, add `extern` declarations. For machine-level code, use `asm { }` blocks; a routine shared by several functions goes into `libraries/geometry/assembly/NAME.asm` with a label `rz@NAME:` (or a constant `rz@NAME equ value`). Variables a routine keeps are labels with `dq 0`; large buffers are allocated at run time, as `rz@write` does.
5. Constants are global variables; the ones a program does not use are left out of it.
6. Rebuild with `cmake --build build`; CMake notices new files and embeds them. Add a test under `tests/programs/` and run `bash tests/run_all.sh`.

Library files cannot define `main`, and every name must be unique across everything a program imports.

## Your own modules

The same works without rebuilding `rzc`: put `.rz` files or folders next to your program and `import` them by name ([language.md](language.md#modules-and-import)). A local module wins over a built-in library of the same name.
