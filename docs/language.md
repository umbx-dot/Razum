# razum language reference

This is the language reference. Start with [a first program](#a-first-program), look up syntax by topic, or use the [library reference](libraries.md) to find an import or function. The [tutorial](tutorial.md) introduces the language one step at a time. [Code generation](codegen.md) describes the assembly emitted by `rzc`.

Contents: [first program](#a-first-program) · [files and comments](#source-files-and-comments) · [program structure](#program-structure) · [names](#names-and-keywords) · [types](#types) · [literals](#literals) · [variables](#variables) · [conversions](#conversions) · [operators](#operators) · [statements](#statements) · [control flow](#control-flow) · [functions](#functions) · [strings](#strings) · [arrays](#arrays) · [structs](#structs) · [constants and enums](#constants-and-enums) · [pointers](#pointers) · [function values](#function-values-and-callbacks) · [modules](#modules-and-import) · [extern functions](#extern-functions) · [DLLs](#making-a-dll) · [asm blocks](#asm-blocks) · [errors](#errors)

## A first program

Save this as `hello.rz`:

```c
import stdio;

int main() {
    print("Hello from razum!");
    return 0;
}
```

Run `rzc hello.rz --run` in a terminal with the compiler on `PATH`. On the learning site, paste the program into the Playground and select **Run**.

```text
Hello from razum!
```

- `import stdio;` brings in the standard input and output library.
- `int main()` is the entry point. Its result is the process exit code. Use `return 0;` for success.
- `print("Hello from razum!");` writes one line. See [stdio](libraries.md#stdio) for reading input or writing several values on the same line.

## Source files and comments

- A source file has the extension `.rz` and is UTF-8 or ASCII text.
- Spaces, tabs and line breaks only separate tokens; indentation has no meaning.
- `// comment` runs to the end of the line. `/* comment */` can span lines and does not nest.

## Program structure

Place declarations such as imports and functions at the top level. Put statements that run, such as an assignment or a function call, inside a function.

| Item | Example |
|---|---|
| Import | `import stdio;` `import math.integer;` |
| Struct definition | `struct Point { int x; int y; }` |
| Enum definition | `enum Color { RED, GREEN, BLUE }` |
| Constant | `const int SIZE = 16;` |
| Extern function | `extern "user32" int MessageBoxA(long window, string text, string caption, uint type);` |
| Global variable | `int counter;` `double rate = 1.5;` `string name = "razum";` `int primes[] = {2, 3, 5};` |
| Function | `int add(int a, int b) { return a + b; }` |
| Program kind | `program gui;` (a window program, no console window) or `program console;` (the default) |

- An executable starts at `int main()` or `int main(string args[])`. The second form receives the [command-line arguments](libraries.md#program). In PowerShell, read the exit code with `$LASTEXITCODE`; in Bash, use `$?`.
- A function or global can be used above the line that defines it, so razum needs no forward declarations. A C-style declaration without a body, `int add(int a, int b);`, is still accepted; it must match the function's definition exactly and is otherwise ignored.
- `program gui;` makes Windows start the program without a console window (PE subsystem 2), for programs that only show windows. `rzc --gui` and `rzc --console` choose the same from the command line and win over the source. A gui program has no console, so `print` output goes nowhere unless it is redirected.
- Statements only appear inside functions; `asm { }` blocks only inside functions.

## Names and keywords

A name starts with a letter or `_` and continues with letters, digits and `_`. Case matters: `count` and `Count` are different.

Reserved words that cannot be names:

| Group | Words |
|---|---|
| Types | `void bool char byte sbyte short ushort int uint long ulong float double string` |
| Statements | `if else while for do switch case default return break continue` |
| Other | `import asm struct enum const extern export fn true false null new sizeof` |
| Built in | `print`, `printf`, `free` |

Any other word is allowed, even assembly words such as `add`, `mov` or `rax`; `rzc` escapes them (see [codegen.md](codegen.md#names-in-the-output)). `print` is built in and cannot be redefined.

## Types

Use `int` for ordinary whole numbers, `long` if the values need more range, `double` for fractional calculations, and `string` for text. Use `char` for one byte of text. For example, a UTF-8 string can use several bytes to encode a single character.

| Type | Size | Values | Default | Literal |
|---|---|---|---|---|
| `bool` | 1 byte | `false`, `true` | `false` | `true` |
| `char` | 1 byte | character codes 0 to 255 | `"\0"` | `"a"` |
| `byte` | 1 byte | 0 to 255 | 0 | `200` |
| `sbyte` | 1 byte | -128 to 127 | 0 | `-5` |
| `short` | 2 bytes | -32768 to 32767 | 0 | `1000` |
| `ushort` | 2 bytes | 0 to 65535 | 0 | `60000` |
| `int` | 4 bytes | -2147483648 to 2147483647 | 0 | `42` |
| `uint` | 4 bytes | 0 to 4294967295 | 0 | `4000000000` |
| `long` | 8 bytes | -9223372036854775808 to 9223372036854775807 | 0 | `5000000000` |
| `ulong` | 8 bytes | 0 to 18446744073709551615 | 0 | `18446744073709551615` |
| `float` | 4 bytes | IEEE 754 single precision, about 7 digits | 0.0 | `1.5` |
| `double` | 8 bytes | IEEE 754 double precision, about 15 digits | 0.0 | `1.5` |
| `string` | 8 bytes | address of zero-terminated text | `""` | `"text"` |
| `T *` | 8 bytes | address of a `T`, or `null` | `null` | `null` |
| `fn(A, B) -> R` | 8 bytes | address of a function taking `A`, `B` and returning `R` | `null` | a function's name |
| `void` | none | only as a function's return type | | |

- Signed integers (`sbyte`, `short`, `int`, `long`) can hold negative values. Unsigned integers (`char`, `byte`, `ushort`, `uint`, `ulong`) start at zero.
- `float` and `double` are the floating-point types. Together with the integer types they are the numeric types.
- `bool` is not a number: `true + 1` is an error. Convert with `(int)flag` to get 0 or 1.
- Integer arithmetic wraps around on overflow: `int big = 2147483647; big++;` makes `big` -2147483648.

## Literals

| Kind | Examples | Type |
|---|---|---|
| Decimal integer | `0` `42` `-7` | `int` if it fits, else `long`, else `ulong` |
| Hexadecimal integer | `0xFF` `0x7fffffff` | the same rule |
| Floating point | `1.5` `.5` `5.` `2e10` `1.5E-3` | `double` |
| Character | `"a"` `"\n"` `"\""`, or in single quotes `'a'` `'\n'` | `char` |
| Boolean | `true` `false` | `bool` |
| String | `"text"` `"line\r\n"` | `string` |

Escapes in characters and strings: `\n` (10), `\t` (9), `\r` (13), `\0` (0), `\\`, `\"`, `\'`.

A text in double quotes that is exactly one byte, such as `"a"` or `"\n"`, is a `char` wherever a `char` is expected: stored in a `char` variable, field, element, constant or global, passed to a `char` parameter, returned from a `char` function, used as a `case` value, as a side of `? :` whose other side is a `char` (`flag ? c : "-"`), or next to an integer in an operation (`c == "a"`, `c >= "a"`, `digit - "0"`, `c == (url_safe ? "-" : "+")`). Everywhere else it stays a `string`. Single quotes (`'a'`) give the same `char` and still work. A longer text where a `char` is expected is an error: `char c = "ab";` and `char c = "é";` (2 bytes in UTF-8) are refused.

An integer literal fits any integer type whose range holds it, so `byte b = 200;` and `ulong u = 1;` need no conversion. A floating literal can be stored in a `float`. A leading `-` belongs to the literal: `-2147483648` is an `int`.

## Variables

```c
int count;              // starts at 0
int limit = 10;
double rate = 2;        // the int 2 becomes 2.0
string title;           // starts as ""
bool done = limit > 5;  // true
```

- A local variable lives from its declaration to the end of its `{ }` block.
- A global variable is declared outside every function and lives for the whole program. Its initial value must be known when compiling: a literal, a [constant](#constants-and-enums) or an expression of them.
- A variable without an initial value starts at its [type's default](#types).
- An inner block may declare a name that already exists outside; inside that block the inner one is used. Declaring the same name twice in one block is an error.

```c
int x = 1;
if (x > 0) {
    int x = 2;      // a new x, only inside this block
    print(x);       // 2
}
print(x);           // 1
```

## Conversions

### Automatic (widening)

A value converts automatically when the target can hold every value of its type: in assignments, initial values, arguments and `return`.

| From | Converts automatically to |
|---|---|
| `sbyte` | `short`, `int`, `long`, `float`, `double` |
| `byte`, `char` | `byte`, `short`, `ushort`, `int`, `uint`, `long`, `ulong`, `float`, `double` |
| `short` | `int`, `long`, `float`, `double` |
| `ushort` | `int`, `uint`, `long`, `ulong`, `float`, `double` |
| `int` | `long`, `float`, `double` |
| `uint` | `long`, `ulong`, `float`, `double` |
| `long`, `ulong` | `float`, `double` |
| `float` | `double` |

Nothing converts automatically to `char` (except `char` itself), to `bool`, or from `bool` and `string`. Every other conversion needs a cast.

### Casts

`(type)value` converts explicitly between any two numeric types, and between `bool` and the integer types.

| Cast | Result |
|---|---|
| Wider integer to narrower integer | Keeps the low bits: `(byte)260` is 4, `(sbyte)200` is -56, `(short)40000` is -25536 |
| Signed to unsigned of the same size | Same bits: `(uint)-1` is 4294967295, `(ushort)-1` is 65535 |
| Floating to integer | Truncates toward zero: `(int)-2.7` is -2; out-of-range values are undefined |
| Integer to floating | Nearest value: `(double)18446744073709551615` is 1.84467440737096e19 |
| `double` to `float` | Rounds to the nearest float: `(float)(1.0 / 3)` is 0.3333333 |
| Integer to `bool` | `false` for 0, `true` for anything else |
| `bool` to integer | 0 or 1 |
| Integer to `char` | The low 8 bits as a character code: `(char)65` is `"A"` |

```c
long big = 5000000000;
int small = (int)big;       // 705032704: the low 32 bits
double ratio = (double)3 / 4;   // 0.75; without the cast 3 / 4 is 0
```

## Operators

From lowest to highest precedence. Operators on one level group left to right: `a - b - c` is `(a - b) - c`. Parentheses override everything.

| Level | Operators | Operands | Result |
|---|---|---|---|
| 1 | `c ? a : b` | `c` bool or integer; `a`, `b` of one type, or two numbers | `a` if `c` is true, else `b`; only one of them runs |
| 2 | `a \|\| b` | bool or integer | `bool`; `b` runs only if `a` is false |
| 3 | `a && b` | bool or integer | `bool`; `b` runs only if `a` is true |
| 4 | `\|` | two integers, or two bools | bitwise or |
| 5 | `^` | two integers, or two bools | bitwise exclusive or |
| 6 | `&` | two integers, or two bools | bitwise and (both sides always run) |
| 7 | `==` `!=` | two numbers, or two bools | `bool` |
| 8 | `<` `<=` `>` `>=` | two numbers | `bool` |
| 9 | `<<` `>>` | two integers | the left side's type, at least `int` |
| 10 | `+` `-` | two numbers | the operation type (below) |
| 11 | `*` `/` `%` | two numbers; `%` integers only | the operation type |
| 12 | `-x` `!x` `~x` `(type)x` `&x` `*p` | number; bool or integer; integer; see casts; a place; a pointer | `-x`, `~x`: x's type, at least `int`; `!x`: `bool`; see [pointers](#pointers) |

`? :` groups to the right: `a ? b : c ? d : e` is `a ? b : (c ? d : e)`. As in C, `&`, `|` and `^` bind looser than `==`: write `(flags & 4) != 0`.

### The operation type

Arithmetic and comparisons first bring both sides to one type, as in C#:

1. If either side is `double`, the operation is `double`; otherwise if either is `float`, it is `float`.
2. Integers smaller than `int` (`char`, `byte`, `sbyte`, `short`, `ushort`) count as `int`.
3. A plain integer literal takes the other side's type when it fits: in `u + 1` with `ulong u` the operation is `ulong`.
4. Same signedness: the wider type. A signed type wider than the unsigned one: the signed type (`long` + `uint` is `long`).
5. Mixed signedness of at most 32 bits: `long` (`int` + `uint` is `long`, so `-1 < (uint)1` is `true`).
6. `long` with `ulong` is an error: cast one side.

So `byte + byte` is `int`, `"a" + 1` is `int` (98), and `float * int` is `float`.

### Notes

- `<<` shifts left; `>>` shifts right, keeping the sign for signed types (`-64 >> 2` is -16) and filling with zeros for unsigned ones. The shift count is taken modulo 32 for types up to `int`/`uint` and modulo 64 for `long`/`ulong`, as in C#: `1 << 33` on an `int` is 2.
- Integer `/` truncates toward zero and `%` takes the sign of the left side, as in C: `-7 / 2` is -3, `-7 % 3` is -1. Integer division by zero stops the program with Windows exception 0xC0000094.
- `float` expressions are computed in double precision and rounded to `float` when stored, passed, returned or cast. This is like C on x86-64 with `FLT_EVAL_METHOD` 1.
- Comparisons involving NaN are false, except `!=`, which is true.
- Conditions (`if`, `while`, `for`, `&&`, `||`, `!`) take a `bool` or any integer; an integer is true when it is not 0. For a floating value, compare explicitly: `if (x != 0.0)`.
- Strings have no operators: no `+`, `==` or indexing (see [strings](#strings)).

## Statements

| Statement | Example | Notes |
|---|---|---|
| Declaration | `int x = 5;` | see [variables](#variables) |
| Assignment | `x = x * 2;` `values[i] = 0;` `p.x = 5;` | the value must convert automatically to the target's type; a whole array or struct copies from another of the same shape |
| Compound assignment | `x += 2;` `x -= 2;` `x *= 2;` `x /= 2;` `x %= 2;` | numeric variables; the result wraps to x's type, so `byte b = 250; b += 10;` gives 4 |
| Bitwise assignment | `x &= 4;` `x \|= 1;` `x ^= 8;` `x <<= 2;` `x >>= 1;` | integer variables, elements and fields |
| Increment, decrement | `x++;` `values[i]--;` `++p.x;` | any numeric variable, element or field; floating ones change by 1.0 |
| Call | `show(x);` | a result, if any, is discarded |
| Block | `{ int t = x; x = y; y = t; }` | opens a new scope |
| `if`, `while`, `for`, `break`, `continue`, `return` | | see [control flow](#control-flow) |
| `asm { }` | `asm { nop }` | see [asm blocks](#asm-blocks) |

`x++` is a statement on its own, never part of an expression: `y = x++;` is an error. Expressions therefore have no side effects except function calls.

## Control flow

Use `if` to choose between paths, `for` to repeat with an update such as a counter, and `while` to repeat while a condition holds. Use `switch` when one value selects between several cases.

### if / else if / else

```c
if (score >= 90) {
    print("A");
} else if (score >= 75) {
    print("B");
} else {
    print("C");
}
```

`else` belongs to the nearest `if`. Braces are optional around a single statement: `if (x < 0) x = -x;`.

### while

Repeats while the condition is true; it is tested before every round.

```c
int n = 1;
while (n < 1000) {
    n *= 2;
}
print(n);       // 1024
```

### for

`for (start; condition; step) body` runs `start` once, then `body` and `step` while `condition` is true. `start` may declare a variable that exists only inside the loop. Each of the three parts may be left out; a missing condition means "forever".

```c
for (int i = 0; i < 5; i++) {
    print(i);           // 0 1 2 3 4
}

for (;;) {
    if (read_int() == 0) break;
}
```

### do / while

Runs the body first and tests afterwards, so the body runs at least once.

```c
int n = 0;
do {
    n += 2;
} while (n < 7);
print(n);       // 8
```

### switch

Compares an integer or `char` value with constant cases. Each case runs only its own statements; there is no falling through into the next case, so `break` at the end of a case is optional.

```c
switch (day) {
    case 0, 6:
        print("weekend");
    case 3:
        print("midweek");
        break;
    default:
        print("workday");
}
```

| Rule | Details |
|---|---|
| Value | Any integer type or `char` |
| Cases | `case` with one or more constant values separated by commas: `case 1, 2, 3:`, `case "a", "e":`, `case LIMIT + 1:`. Each must fit the value's type; a value may appear only once. In a switch on an [enum](#constants-and-enums), every case must be one of its members: `case Color.RED:` |
| `default` | Runs when no case matches; at most one, anywhere in the list. Without it, nothing runs |
| `break` | Leaves the switch at once |
| `continue` | Inside a loop, goes on with the loop's next round |
| Scope | Variables declared in a case belong to that case |

A function whose `switch` has a `default` and returns in every case counts as returning.

### break and continue

`break;` leaves the innermost loop or switch. `continue;` skips to the next round; in a `for` loop the step still runs.

```c
for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) continue;   // skip even numbers
    if (i > 7) break;           // stop after 7
    print(i);                   // 1 3 5 7
}
```

### return

`return value;` leaves a function with a result; `return;` leaves a `void` function. A non-`void` function must end in `return` on every path; `rzc` checks this. An `if` with an `else` where both sides return counts as returning; a loop never does.

## Functions

Give a function a name, a return type, and any parameters it needs. Calling it runs the body and gives the caller the value from `return`. Declare the return type as `void` for a function that only performs an action.

```c
long factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

void greet(string name, int times) {
    for (int i = 0; i < times; i++) {
        write_string("hello, ");
        print(name);
    }
}
```

- Form: `return_type name(type parameter, ...) { statements }`. `void` means no result.
- Any number of parameters. Each argument must convert automatically to its parameter's type: `factorial(5)` passes an `int`, `average(3, 4)` may pass ints to `double` parameters.
- Parameters are local variables: assigning to one does not change the caller's variable.
- Functions may call themselves (recursion) and each other in any order.
- An array or struct parameter refers to the caller's array or struct; nothing is copied, and changes are visible to the caller (see [arrays](#arrays) and [structs](#structs)).
- A function cannot return an array or struct; pass one in and fill it.
- Two functions cannot share a name, and a function cannot have a global's name.
- `int main()` is the entry point. When it returns, the program ends with that exit code. Write `int main(string args[])` to receive the command line: `args[0]` is the program as it was started, `args[1]` the first argument, and `args.length` their count. Words are separated by spaces; double quotes keep spaces inside one argument (`"two words"`) and are removed.

## Strings

A `string` is the address of zero-terminated text. String literals live in the program's data, and a string variable holds one 8-byte address. Text is UTF-8: `"Grüße, 世界"` works in literals, on the console, in file names and in command-line arguments; `s[i]` reads one byte, and the [text](libraries.md#text) library counts characters and converts to UTF-16 for Windows.

```c
string greeting = "hello";
string copy = greeting;     // the same text, not a copy of the bytes
print(copy);
write_string("no newline");
```

You can store strings in variables, pass them to and return them from functions, print them, and read single characters with `s[i]`, which gives a `char`; the text of a string cannot be changed through it. The `text` library ([libraries.md](libraries.md#text)) has `length`, `compare`, `find`, `contains`, `copy`, `concat`, `substring`, `trim`, `split_part`, `to_upper`, `int_to_text`, `parse_int` and more. To build new text, use a `char` array:

```c
import text;

char line[64] = "total: ";
char digits[24];
int_to_text(digits, 42);
concat(line, digits);
print(line);                // total: 42
print(line[0]);             // t
```

A string field or element of an array or struct that was never set has no text: `print` and the `text` functions treat it as empty, but `s[i]` on it stops the program.

## Arrays

An array is a fixed number of values of one type, stored next to each other.

```c
int numbers[5];                     // five ints, all 0
int primes[] = {2, 3, 5, 7, 11};    // the size comes from the list
double weights[4] = {0.5, 1.5};     // the rest starts at 0.0
char name[16] = "razum";            // text; the rest are '\0'
int grid[3][4];                     // 3 rows of 4 ints

numbers[0] = 4;
numbers[i + 1] += 2;
grid[2][3] = 7;
print(numbers.length);              // 5
print(grid[1].length);              // 4
print(name);                        // a char array prints as text
```

| Rule | Details |
|---|---|
| Size | A whole number from 1 up, written as a literal or a [constant](#constants-and-enums) expression (`char name[NAME_LENGTH + 1]`); it can be left out only when a `{ }` list or text gives it |
| Index | Any integer expression, starting at 0. A literal index outside the array is a compile error; a computed one is not checked when the program runs |
| `.length` | The number of elements, an `int` |
| `.first`, `.last` | The first and the last element, as a place you can read and assign: `values.last = 0;` is `values[values.length - 1] = 0;`. Written on a variable or a field (`values.last`, `p.items.first`). An empty array has neither; this is not checked when the program runs |
| Methods (`import array;`) | On arrays of numbers and `char`s: `values.sort()`, `values.sort_descending()`, `values.reverse()`, `values.fill(x)`, `values.contains(x)`, `values.index_of(x)` (`-1` if missing), `values.min()`, `values.max()`, `values.sum()`. See [libraries.md](libraries.md#array) |
| Starting values | Every element starts at its type's default, then the `{ }` list fills elements in order. A list may hold fewer values, never more |
| Element types | Any type, including structs and other arrays |
| Copying | `a = b;` copies every element when both have the same element type and size |
| As a parameter | Write it without a size: `long sum(int values[])`. Any `int` array can be passed; the function sees the caller's elements and its `.length` |
| As text | A `char` array can be passed where a `string` is expected and printed with `print`; text stops at the first `'\0'` |
| Not possible | Comparing arrays with `==`, returning an array, arrays that grow |

```c
long sum(int values[]) {
    long total = 0;
    for (int i = 0; i < values.length; i++) {
        total += values[i];
    }
    return total;
}
```

```c
import stdio;
import array;

int main() {
    int scores[5] = {70, 95, 80, 60, 88};
    print(scores.last);             // 88
    scores.sort_descending();       // 95 88 80 70 60
    print(scores.first);            // 95, the best score
    print(scores.sum());            // 393
    print(scores.contains(60));     // true
    return 0;
}
```

## Structs

A struct groups named fields into one value.

```c
import stdio;

struct Point {
    int x;
    int y;
}

struct Player {
    char name[16];
    Point position;
    int scores[3];
}

Point origin = {0, 0};
Player hero = {"Ada", {5, 7}, {10, 20, 30}};

int main() {
    Point p = {3, 4};
    Point q = p;                    // a copy
    q.x = 30;                       // p.x is still 3
    hero.position.y += 1;
    hero.scores[2]++;
    Point path[10];                 // an array of structs
    path[2].x = 5;
    print(hero.name);
    return 0;
}
```

| Rule | Details |
|---|---|
| Definition | `struct Name { type field; ... }` at the top level of a file, in any order with functions; at least one field |
| Fields | Any type, including arrays with a size and other structs. A struct cannot contain itself |
| Starting values | All fields start at their defaults; a `{ }` list fills fields in order, nested lists fill nested structs and arrays |
| Copying | `p = q;` and `Point p = q;` copy every field |
| As a parameter | `void move(Point p, int dx)` refers to the caller's struct: `p.x += dx;` changes it |
| Not possible | Comparing structs with `==`, returning a struct, methods |

Field layout follows C: each field starts at a multiple of its own size (arrays and structs: of their largest element), and the struct's size is rounded up to that multiple. `Player` above is 16 + 8 + 12 = 36 bytes.

## Constants and enums

### const

`const` declares a name for a value that never changes. `rzc` computes the value while compiling and writes it straight into each instruction that uses it, so a constant takes no memory and costs nothing at run time.

```c
import stdio;

const int SIZE = 16;
const int CELLS = SIZE * SIZE;          // 256, computed while compiling
const long BIG = (long)1 << 40;
const double TAU = 2 * PI;              // may use a constant declared further down
const double PI = 3.141592653589793;
const string NAME = "razum";
const bool VERBOSE = SIZE > 8;

int grid[CELLS];                        // constants can give array sizes

int main() {
    const int LAST = CELLS - 1;         // local constants work the same way
    char row[SIZE + 1];
    print(LAST);                        // 255
    return 0;
}
```

| Rule | Details |
|---|---|
| Form | `const type NAME = value;` at the top level of a file or inside a function |
| Types | Any number type, `bool`, `char`, `string`, or an enum |
| Value | Known when compiling: literals, other constants, enum members, arithmetic, comparisons, `&& \|\| !`, bit operators, casts between number types, and `? :`. It follows the same rules as at run time: `7 / 2` is 3, `1 << 40` on an `int` shifts by 8 |
| Narrowing | An integer constant fits any integer type that holds its value: `const int LIMIT = 200;` can be stored in a `byte`, as in C# |
| Uses | Anywhere a value is allowed, plus array sizes, `case` values and global initial values |
| Not possible | Assigning to it (`'SIZE' is a const; its value cannot change`), taking its address, arrays or structs as constants |

Global constants may refer to each other in any order; a constant that depends on itself is an error.

### enum

An `enum` is a type with a fixed set of named values, its **members**:

```c
import stdio;

enum Color {
    RED,            // 0
    GREEN,          // 1
    BLUE,           // 2
}

enum Status { OK = 0, NOT_FOUND = 404, TEAPOT = 418, NEXT }    // NEXT is 419

string color_name(Color color) {
    switch (color) {
        case Color.RED:
            return "red";
        case Color.GREEN:
            return "green";
        default:
            return "blue";
    }
}

int main() {
    Color color = Color.GREEN;
    print(color_name(color));                   // green
    int number = color;                         // 1: an enum converts to int by itself
    Color next = (Color)(color + 1);            // back from a number needs a cast
    print(color == Color.BLUE);                 // false
    print(next == Color.BLUE);                  // true
    return 0;
}
```

| Rule | Details |
|---|---|
| Members | Named with the enum's name in front: `Color.RED`. Without `= value` a member is one more than the one before; the first is 0. Values are whole numbers that fit an `int`; a trailing comma is allowed |
| Storage | An enum value is an `int` (4 bytes), in variables, fields, arrays, parameters, return values and `extern` functions |
| Into an enum | Only a member or another value of the same enum, or a cast: `Color c = 1;` is an error, `(Color)1` is not. A cast does not check that the number is a member |
| Out of an enum | Converts to `int` and wider types by itself, so it can be printed, compared, used in arithmetic and as an index |
| `switch` | Every `case` must be a member of the switched enum |
| Comparison | `==`, `!=`, `<`, ... compare the numbers |

## Pointers

A pointer holds the address of a value. `T *` is a pointer to a `T`: `int *`, `Point *`, `Node **`.

```c
import stdio;

struct Node {
    int value;
    Node *next;                 // a struct may point to its own kind
}

Node *push(Node *head, int value) {
    Node *node = new Node;      // zeroed memory from the heap
    node.value = value;         // . reaches through a pointer; node->value works too
    node.next = head;
    return node;
}

void swap(int *a, int *b) {
    int kept = *a;
    *a = *b;
    *b = kept;
}

int main() {
    int x = 1;
    int y = 2;
    swap(&x, &y);

    int n = x + 5;
    long *squares = new long[n];    // n known only while running
    squares[3] = 9;
    free(squares);

    Node *list = null;
    list = push(list, 10);
    for (Node *node = list; node != null; node = node.next) {
        print(node.value);
    }
    return 0;
}
```

| Form | Meaning |
|---|---|
| `&x` | the address of a variable, element (`&values[i]`), field (`&p.y`) or `*p`; type `T *` |
| `*p` | the value `p` points to; can be read, assigned (`*p = 5;`), changed (`*p += 1;`), or copied when it is a struct |
| `p[i]` | element `i` after the address in `p`, counted in elements: `p[0]` is `*p` |
| `p.field`, `p->field` | a field of the struct `p` points to |
| `null` | no address; fits every pointer type |
| `new T` | a zeroed `T` on the heap; its address, or `null` if memory ran out |
| `new T[n]` | `n` zeroed `T`s in a row; `n` may be any integer expression |
| `free(p)` | gives memory from `new` back; `free(null)` does nothing |
| `sizeof(T)` | the size of a type in bytes, as an `int`: `sizeof(Point)`, `sizeof(long[10])` |
| `p == q`, `p != null`, `if (p)` | compare pointers to the same type; a pointer is true when it is not `null` |
| `(T *)value`, `(ulong)p` | casts between pointer types, and between pointers and integers |

Conversions: an array converts automatically to a pointer to its first element (`int *first = numbers;`), as in C; a `string` converts to and from `char *`. Pointers to different types never convert automatically.

There is no pointer arithmetic beyond indexing (`&p[i]` gives the address `i` elements further), no garbage collector (every `new` needs a `free`, or the memory stays in use until the program ends), and no checks when the program runs: using a pointer after `free`, or `null`, stops the program with an access violation.

## Function values and callbacks

A function type `fn(parameters) -> result` holds the address of a function; leave out `-> result` for `void`. Parameter names are optional and only document: `fn(long window, uint message) -> long`.

```c
import stdio;

long add(long a, long b) {
    return a + b;
}

long apply(fn(long, long) -> long operation, long a, long b) {
    return operation(a, b);
}

int main() {
    fn(long, long) -> long operation = add;     // a function's name is its value; &add is the same
    print(operation(2, 3));                     // 5
    print(apply(add, 6, 7));                    // 13
    fn(long, long) -> long table[2] = {add, add};
    print(table[1](1, 2));                      // 3
    return 0;
}
```

| Rule | Details |
|---|---|
| Values | the name of any razum function, or of an `extern` function whose parameters are not arrays (use `T *` instead), `&name`, `null`, or a cast `(fn(int) -> int)address` |
| Array parameters | `fn(int[]) -> int` takes an array without a size, passed like any array parameter (the array and its `.length`): `fn(int[]) -> int total = sum_all; total(numbers)` |
| Globals | a global function value may start as a razum function's name, also inside arrays and structs: `fn(int) -> int steps[2] = {twice, plus_one};`. An `extern` function's address is known only at run time, so set such a global in `main` |
| Calls | `value(arguments)`, also on elements, fields and call results: `table[i](x)`, `pick("+")(1, 2)` |
| Comparing | `==` and `!=` with the same type or `null`; a function value is true when it is not `null` |
| As a callback | Pass a razum function to C or Windows wherever a function pointer is expected: `EnumWindows(count_window, 0)`, `CreateThread(0, 0, worker, 0, 0, &id)`, `qsort(values, n, sizeof(int), compare)`. The calling convention is the same, so it needs no wrapper |
| Loading at run time | `fn(int) -> int metrics = (fn(int) -> int)GetProcAddress(LoadLibraryA("user32.dll"), "GetSystemMetrics");` then `metrics(0)` |

A struct parameter in a function type is passed by reference, as everywhere in razum. A razum function that is used as a value re-extends its narrow integer parameters on entry, because C and Windows callers leave the upper register bits undefined. Several threads may run razum code at once, but `print` and the other buffered output functions are not safe to call from two threads at the same time.

## Modules and import

`import name;` makes the declarations in a library or project module available. Call the functions by their plain names, for example `print("hello")` after `import stdio;`.

Use [the project manifest](projects.md) to declare named build targets, compiler profiles and locked Git dependencies. `rz build` resolves package directories and passes them to `rzc` as import roots.

```c
import stdio;           // the built-in stdio library
import math.integer;    // only the integer part of math
import geometry;        // your geometry.rz file or geometry/ folder
```

`rzc` looks for `import a.b;` in this order:

1. Next to the importing file: `a/b.rz`, or every `.rz` file in the folder `a/b/` and its subfolders, plus every `.asm` file in that folder tree.
2. The same, next to the main file (the file given to `rzc`). So `ui/window.rz` can `import math.vector;` to reach `math/vector.rz` beside the main file: a project's folders can use each other.
3. The same, in each folder given with `rzc -I <folder>`, in order. The VS Code extension passes the open workspace folder, so a module in a subfolder checks correctly on its own.
4. The libraries built into `rzc`: `libraries/a/b.rz`, or everything in `libraries/a/b/` ([libraries.md](libraries.md)).

Rules:

- An imported file can import others; each file is loaded once.
- An imported file cannot contain `main`.
- The compiler includes only the code and data reachable from the program's entry points. An import makes its names available during compilation, even when only one function ends up in the executable.
- If your function has the same name as a library function, `rzc` reports both places.

Example layout, as in `tests/modules/`:

```
app.rz                  import geometry; import units; import report.describe_square;
report/describe_square.rz       import geometry.area; (a sibling folder, found next to app.rz)
units.rz                long to_centimeters(long meters) { ... }
geometry/area.rz        long rectangle_area(long width, long height) { ... }
geometry/perimeter.rz   long square_perimeter(long side) { ... }
geometry/assembly/triple.asm    rz@triple: ... (used from an asm block)
```

### Checking a module on its own

A module file has no `main`. When an editor checks it (`rzc file.rz --check`, as the VS Code extension does on every change), `rzc` treats a file without `main` as a module: it also loads the other `.rz` files in the same folder (skipping any that have their own `main`), follows all `import`s, and checks everything without asking for an entry point. So a module may use functions and structs from the files beside it; anything from another folder or library it must `import`, exactly as a program would. To reach a sibling folder of the project when checking a file alone, pass the project folder: `rzc -I . ui/window.rz --check`. Compiling a module file into an `.exe` still reports that it has no `main`.

## extern functions

`extern` declares a function that lives in a Windows DLL. razum code then calls it like its own functions, and `rzc` adds the DLL to the program's import table.

```c
extern "user32" int MessageBoxA(long window, string text, string caption, uint type);
extern "kernel32" void Sleep(uint milliseconds);
extern "winmm.dll" uint timeGetTime();

int main() {
    MessageBoxA(0, "Hello from razum", "razum", 0);
    return 0;
}
```

| Rule | Details |
|---|---|
| Form | `extern "dll" return_type name(parameters);` at the top level, no body. The DLL name may include `.dll`; letter case does not matter |
| Name | Exactly the exported name, including the `A` or `W` suffix: `MessageBoxA` takes ANSI text, `MessageBoxW` UTF-16 (`ushort *`, from `to_utf16`) and so handles every language |
| Parameters | Any razum type. `string` passes the text's address; a pointer passes itself (`&value` for out-parameters); an array passes only its address (no length); a struct passes its address, so the function can fill it |
| Results | `void`, any integer, `float`, `double` or `string` |
| Calling convention | Windows x64, the same one razum uses everywhere |

[libraries.md](libraries.md#windows) maps Windows types to razum types; `import windows;` already declares the most common functions. One name can be declared only once in a program, so a function that `windows` declares should not be declared again when `windows` is imported.

## Making a DLL

Mark the functions or mutable globals a Windows DLL offers with `export` and build with `--dll`:

```c
struct Point {
    int x;
    int y;
}

export int answer = 42;

export long add(long a, long b) {
    return a + b;
}

export void move_point(Point p, int dx, int dy) {
    p.x += dx;
    p.y += dy;
}
```

```powershell
rzc mathlib.rz --dll
```

This writes `mathlib.dll`, `mathlib.def` (the export list, for making an import library) and `mathlib.h` (C and C++ declarations for the public interface). The `.def` marks shared variables as `DATA`. Each DLL has its own globals and its own output buffer. The source may import libraries and modules like a program.

| Using the DLL from | How |
|---|---|
| razum | `extern "mathlib" long add(long a, long b);` and keep `mathlib.dll` next to the `.exe` (or on `PATH`) |
| C or C++ with GCC/MinGW or Clang | `#include "mathlib.h"`, then link the DLL itself: `gcc main.c mathlib.dll` |
| C or C++ with MSVC | make an import library once: `lib /def:mathlib.def /machine:x64 /out:mathlib.lib`, then `cl main.c mathlib.lib` |
| CMake | `add_razum_library(mathlib mathlib.rz)` and `target_link_libraries(app PRIVATE mathlib)`; see [cmake.md](cmake.md) |

How razum types appear in C:

| razum | C |
|---|---|
| `bool`, `char`, `byte`, `short`, `int`, `long`, `ulong`, `float`, `double`, ... | `bool`, `char`, `uint8_t`, `int16_t`, `int32_t`, `int64_t`, `uint64_t`, `float`, `double`, ... |
| `string` | `const char *` |
| `T *` | `T *` |
| `fn(int) -> int` | `int32_t (*)(int32_t)` |
| a struct parameter `Point p` | `Point *p` (passed by reference, as in razum) |
| an array parameter `int values[]` | two parameters: `int32_t *values, int64_t values_length` |

`LoadLibraryA`, `GetProcAddress` and `FreeLibrary` are available through `import windows;`. Cast the result of `GetProcAddress` to a [function type](#function-values-and-callbacks) for a function or a typed pointer for a data export. Check the returned address before using it. Keep the DLL loaded while any of those addresses are in use.

### Run code when Windows loads a DLL

```c
import windows.dll;

export int ready = 0;

int DllMain(long module, uint reason, void *reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        ready = 1;
    }
    return 1;
}
```

`DllMain` is optional and uses the signature shown above. A DLL with just `DllMain` may omit exports. Windows supplies the module handle, a reason code and a reserved pointer. `DLL_PROCESS_ATTACH` (1) and `DLL_PROCESS_DETACH` (0) report load and unload. `DLL_THREAD_ATTACH` (2) and `DLL_THREAD_DETACH` (3) report thread startup and exit when Windows sends those notifications.

Return `0` from process attach to refuse the load, or a nonzero value to accept it. Windows ignores the return value for the other reasons. On process attach `reserved` is null for `LoadLibrary` and non-null for load-time linking. On process detach it is non-null when the process is terminating.

The Windows loader holds its lock while calling the hook. Keep the hook to simple local initialization and put work that loads DLLs or waits on other threads in an exported initializer called after loading. Any linked Razum output buffer is flushed after the process detach hook. [Windows DLL entry-point guidance](https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain)

## asm blocks

`asm { ... }` inside a function puts NASM instructions directly into the output, unchanged except for indentation. Use it for anything razum cannot express yet.

```c
import stdio;

long add_three(long value) {
    asm {
        mov rax, qword [rsp + .value]
        add rax, 3
    }
}

int main() {
    long count = 5;
    asm {
        shl qword [rsp + .count], 1     ; count = count * 2
    }
    print(count);                       // 10
    return 0;
}
```

| What | How |
|---|---|
| Local variable or parameter `x` | `qword [rsp + .x]` (a variable shadowing an outer one is `.x@1`, `.x@2`, ...; check with `rzc -S`) |
| Local array or struct `a` | Its bytes start at `[rsp + .a]`: element `i` of an `int` array is `dword [rsp + .a + 4*i]`, a field is at its offset. An array or struct parameter's slot holds its address; an array parameter's length is in `qword [rsp + .a@length]` |
| Global variable `x` | `qword [$x]` |
| Your function `f` | `call $f`, with arguments in `rcx`, `rdx`, `r8`, `r9` or `xmm0`-`xmm3` |
| A library routine | `call rz@write_string` (see [libraries.md](libraries.md#assembly-routines)) |
| A Windows function | `call [iat@GetTickCount64]` for `kernel32.dll`, `call [iat@user32@MessageBoxA]` for any other DLL; `rzc` adds the import automatically |
| A result | A non-`void` function may end with an `asm` block that leaves its result in `rax` (integers, bool, char, string) or `xmm0` (`double`; `float` as single precision) |
| Labels | `.done:` is local to the function. Do not reuse a variable's name, since `.x` is already defined for variable `x` |
| Comments | `;` to the end of the line, as in NASM |

[examples/asm_tour.rz](../examples/asm_tour.rz) shows each of these: `popcnt` and `bswap` on parameters, `bsr` with a razum guard for 0, a loop with a local label, writing an `int` variable with the extend rule, reading the time stamp counter, calling `GetTickCount64` from `kernel32.dll`, and `cpuid` with `rbx` saved and its own stack space for a call.

Rules that keep the surrounding code correct:

- Every variable has an 8-byte slot. Integer variables hold their value sign- or zero-extended to 64 bits. Reading a narrow type with a narrower operand is fine (`mov eax, dword [rsp + .i]`). After writing one, extend it first: `movsxd rax, eax` then `mov qword [rsp + .i], rax`. A `float` uses the low 4 bytes (`movss dword [rsp + .f], xmm0`).
- You may change `rax`, `rcx`, `rdx`, `r8`-`r11` and `xmm0`-`xmm5`. Save and restore any other register you change (`rbx`, `rsi`, `rdi`, `rbp`, `r12`-`r15`, `xmm6`-`xmm15`). With `rzc -O` this matters even more: the calling function may keep its variables in exactly those registers ([optimizer.md](optimizer.md)). A function that contains an `asm` block keeps all its own variables in their stack slots, so `[rsp + .x]` works the same with and without `-O`.
- Leave `rsp` as you found it. At the start of an asm block `rsp` is 16-byte aligned and `[rsp]` to `[rsp + 31]` is free shadow space, so you can `call` directly. A call with more than four arguments needs its own stack space: `sub rsp, 48` ... `add rsp, 48`, keeping 16-byte alignment.
- A `}` inside a `;` comment or a quoted string does not end the block.
- Output from `print` and `write_*` is buffered. Before ending the program yourself with `call [iat@ExitProcess]`, `call rz@flush`.

## Errors

`rzc` reports every error it finds, one per line as `file:line:column: error: message`, and exits with code 1. After a syntax error it continues with the next top-level item (a line that starts at column 1) and stops before type checking; type errors are reported per statement, so one mistake does not hide the next. It stops after 100 errors. Each message says what is wrong and, where possible, how to fix it:

```
tour.rz:3:13: error: cannot store a double in int 'a'; convert it with (int)
app.rz:5:14: error: 300 does not fit in byte, which holds 0 to 255
app.rz:7:17: error: '+' mixes a long and a ulong; convert one side with (long) or (ulong)
```

`tests/errors/` holds one example of every kind of error, with its exact message on the first line; `tests/errors_multiple/` holds files with several errors and their full expected output.

For editors, `rzc file.rz --symbols` prints every declaration as tab-separated fields: kind (`function`, `parameter`, `local`, `const`, `global`, `struct`, `field`, `enum`, `member`, `builtin`), name, a one-line description (`long add(long a, long b)`, `const int SIZE = 16`), file, line, column, and for parameters and locals the lines their function spans. It works on files with errors too. `rzc --source <rzc>/libraries/stdio/output/print_long.rz` prints a built-in library file.
