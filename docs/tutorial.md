# Learn razum step by step

This tutorial teaches programming with razum from the very beginning. You need no earlier experience. Each lesson explains one idea, shows a complete program you can type in and run, and ends with tasks. The solutions are hidden under **Solution**: try the task first, then compare.

Type the examples yourself instead of copying them. Typing teaches your fingers the shapes of the language, and the mistakes you make on the way teach you how to read the compiler's messages.

When you want every detail of a feature, the [language reference](language.md) has it; the [library reference](libraries.md) lists every function you can call.

Lessons: [0 What razum is](#0-what-razum-is) · [1 Setting up](#1-setting-up) · [2 First program](#2-your-first-program) · [3 Output](#3-output) · [4 Variables](#4-variables-and-types) · [5 Arithmetic](#5-arithmetic) · [6 Input](#6-reading-input) · [7 Decisions](#7-decisions) · [8 Loops](#8-loops) · [9 Functions](#9-functions) · [10 Arrays](#10-arrays) · [11 Text](#11-text) · [12 Structs](#12-structs) · [12b Constants and enums](#12b-constants-and-enums) · [13 Modules](#13-splitting-a-program-into-files) · [14 Pointers](#14-pointers-and-the-heap) · [15 Function values](#15-functions-as-values) · [16 A window](#16-a-window-with-buttons) · [17 Errors](#17-reading-error-messages) · [18 Final project](#18-final-project-a-grade-book) · [19 Next steps](#19-where-to-go-next)

## 0. What razum is

A **program** is a list of instructions for a computer. The processor itself only understands **machine code**: numbers that mean things like "add these two values" or "jump to that instruction". Writing numbers by hand is slow and error-prone, so people write programs in a **programming language**, which is text that humans can read, and let another program translate it.

razum is such a language. You write a `.rz` file, and the **compiler** `rzc` translates it:

```
hello.rz  ──rzc──>  hello.asm  ──NASM──>  hello.exe
your text           assembly             a Windows program
```

- **Assembly** is machine code written as words (`mov`, `add`, `call`). `rzc` writes it as a readable `.asm` file, with each line of your program above the instructions it became.
- **NASM** is an assembler: it turns the assembly into the bytes of a Windows `.exe`. The installer brings it along, and `rzc` runs it for you.
- The `.exe` runs on any 64-bit Windows PC, on its own. It needs no razum installation, no runtime and no extra files.

Why learn with razum?

- **It looks like C.** Its syntax (braces, semicolons, `if`, `while`, `for`) is the same family as C, C++, C#, Java and JavaScript. What you learn here carries over.
- **Nothing is hidden.** A "hello world" is a 1,328-byte `.exe` on Windows (728 bytes on Linux), and you can read every instruction in it (`rzc hello.rz --asm`). When you are curious how a computer really works, the answer is one file away.
- **It is strict and explains itself.** `rzc` refuses programs that mix types carelessly, and every error message says what is wrong and usually how to fix it.

razum is **compiled** (translated once, then run as often as you like) and **statically typed** (every value has a type that is known before the program runs). Both ideas come up again in lessons 4 and 5.

## 1. Setting up

1. Run `razum-setup.exe` (the installer; [README.md](../README.md#build) shows how to build it). It installs `rzc` and NASM, adds them to your PATH, gives `.rz` files their icon, and offers the VS Code extension.
2. Open a **new** terminal (PowerShell) so it sees the new PATH, and check:

   ```powershell
   rzc --help
   ```

   It prints the usage line `usage: rzc <file.rz> ...`.
3. Install [Visual Studio Code](https://code.visualstudio.com/) if you do not have it, and let the installer add the razum extension. The editor then colours razum code, closes brackets for you, and underlines errors while you type.
4. Make a folder for your lessons, for example `Documents\razum-lessons`, and open it in VS Code (**File > Open Folder**). Open its terminal with **Terminal > New Terminal**.

## 2. Your first program

Create `hello.rz` with this text:

```c
import stdio;

int main() {
    print("Hello, world!");
    return 0;
}
```

Run it:

```powershell
rzc hello.rz --run
```

```
Hello, world!
```

Line by line:

| Line | Meaning |
|---|---|
| `import stdio;` | Use the standard input/output library, which contains `print`. |
| `int main() {` | Every program starts in a **function** called `main`. `int` says it gives back a whole number when it ends. |
| `print("Hello, world!");` | Write the text and go to the next line. Text in double quotes is a **string**. |
| `return 0;` | End `main` and give back 0, which tells Windows "finished without problems". |
| `}` | The end of `main`. Braces `{ }` group the lines that belong together. |

Every instruction (a **statement**) ends with a semicolon `;`. Indentation (the spaces at the start of lines) does not change the meaning; it is there so people can see the structure.

`rzc hello.rz --run` built `hello.exe` and ran it. You can run the `.exe` again without compiling: `.\hello.exe`.

**Comments** are notes for people; the compiler ignores them:

```c
// this is a comment until the end of the line
/* this one can
   span several lines */
```

### Tasks

1. Print your name on one line and your favourite food on the next.
2. Delete the `;` after the `print(...)` line and compile. Read the message, then put the `;` back.

<details><summary>Solution</summary>

```c
import stdio;

int main() {
    print("Ada");
    print("pancakes");
    return 0;
}
```

Without the semicolon, `rzc` reports `hello.rz:5:5: error: expected ';', found 'return'`: on line 5, column 5, it expected the `;` that ends the previous statement. The position points at the first thing after the missing character, so look just before it.

</details>

## 3. Output

`print` writes a value and then ends the line. To write several values on one line, use the `write_` functions, which do not end the line:

```c
import stdio;

int main() {
    write_string("Apples: ");
    write_long(12);
    print_line();
    write_string("Price: ");
    write_double(1.5);
    write_char('\n');
    print(42);
    print(true);
    return 0;
}
```

```
Apples: 12
Price: 1.5
42
true
```

| Function | Writes |
|---|---|
| `print(value)` | any value, then a line end |
| `write_string(text)` | text |
| `write_long(number)` | a whole number |
| `write_double(number)` | a number with a fraction |
| `write_char(character)` | one character, written like a one-letter text: `"a"`, `"\n"` (line end), `"\t"` (tab) |
| `print_line()` | only a line end |

### Tasks

1. Print `3 + 4 = 7` on one line, writing the three numbers with `write_long`.
2. Print this box with three `print` calls:

   ```
   +-----+
   | rz  |
   +-----+
   ```

<details><summary>Solution</summary>

```c
import stdio;

int main() {
    write_long(3);
    write_string(" + ");
    write_long(4);
    write_string(" = ");
    write_long(7);
    print_line();
    print("+-----+");
    print("| rz  |");
    print("+-----+");
    return 0;
}
```

</details>

## 4. Variables and types

A **variable** is a named place that holds a value. You create it with a **declaration**: its type, its name, and usually a starting value.

```c
import stdio;

int main() {
    int apples = 12;
    double price = 0.25;
    bool ripe = true;
    char grade = "A";
    string fruit = "apple";

    print(apples);
    apples = apples + 3;
    print(apples);
    print(price);
    print(ripe);
    print(grade);
    print(fruit);
    return 0;
}
```

```
12
15
0.25
true
A
apple
```

`apples = apples + 3;` is an **assignment**: compute the right side (12 + 3), then store the result in the variable on the left. The `=` means "store", not "is equal".

The **type** says what kind of value a variable holds. The ones you need most:

| Type | Holds | Example |
|---|---|---|
| `int` | whole numbers from about -2.1 billion to 2.1 billion | `int count = 10;` |
| `long` | whole numbers up to about 9.2 × 10¹⁸ | `long stars = 100000000000;` |
| `double` | numbers with a fraction, about 15 digits exact | `double pi = 3.14159;` |
| `bool` | `true` or `false` | `bool done = false;` |
| `char` | one character | `char letter = "x";` |
| `string` | text | `string city = "Vienna";` |

There are more (`byte`, `short`, `uint`, `float`, ...) for special needs; [language.md](language.md#types) lists them all.

Rules for names: they start with a letter or `_`, continue with letters, digits or `_`, and case matters (`total` and `Total` are different). Choose names that say what the value means: `seconds_left` beats `s`.

A variable without a starting value starts at zero: `0`, `0.0`, `false`, or empty text.

A variable cannot change its type. `int count = 2.5;` is an error, because 2.5 does not fit in a whole number. That is what **statically typed** means: `rzc` knows every type before the program runs and catches the mismatch for you.

### Tasks

1. Make variables for your age (`int`), your height in metres (`double`) and your name (`string`), and print them.
2. Make an `int` called `score` that starts at 10. Add 5, then double it, using assignments. Print it (the answer is 30).

<details><summary>Solution</summary>

```c
import stdio;

int main() {
    int age = 30;
    double height = 1.72;
    string name = "Ada";
    print(age);
    print(height);
    print(name);

    int score = 10;
    score = score + 5;
    score = score * 2;
    print(score);
    return 0;
}
```

</details>

## 5. Arithmetic

| Operator | Meaning | `7 ? 2` with `int` | `7.0 ? 2.0` with `double` |
|---|---|---|---|
| `+` | add | 9 | 9.0 |
| `-` | subtract | 5 | 5.0 |
| `*` | multiply | 14 | 14.0 |
| `/` | divide | **3** | 3.5 |
| `%` | remainder | 1 | not allowed |

Watch out for `/` with whole numbers: `7 / 2` is `3`, because the fraction is cut off. If either side is a `double`, the result is a `double`: `7 / 2.0` is `3.5`. The remainder `%` answers "what is left over": `7 % 2` is 1, so a number is even when `n % 2` is 0.

`*`, `/` and `%` go before `+` and `-`, as in school: `2 + 3 * 4` is 14. Use parentheses to change the order: `(2 + 3) * 4` is 20.

**Casts** convert a value to another type: `(double)7` is `7.0`, `(int)3.9` is `3` (cut, not rounded). Converting to a type that holds more, such as `int` to `double`, happens on its own; converting to one that holds less needs a cast, so you always see where information can be lost.

Shortcuts: `x += 5;` means `x = x + 5;` (also `-=`, `*=`, `/=`, `%=`), and `x++;` adds 1, `x--;` takes 1 away.

```c
import stdio;

int main() {
    int total_minutes = 135;
    int hours = total_minutes / 60;
    int minutes = total_minutes % 60;
    write_long(hours);
    write_string(" h ");
    write_long(minutes);
    print(" min");

    int a = 7;
    int b = 2;
    print(a / b);
    print((double)a / b);

    int lives = 3;
    lives--;
    lives += 10;
    print(lives);
    return 0;
}
```

```
2 h 15 min
3
3.5
12
```

### Tasks

1. Turn `int seconds = 7384;` into hours, minutes and seconds, and print `2 h 3 min 4 s`.
2. Compute the average of the three `int`s 4, 7 and 8 as a `double` and print it (6.333...).
3. A pizza has 8 slices and 3 friends share it equally. Print how many whole slices each gets and how many are left over.

<details><summary>Solution</summary>

```c
import stdio;

int main() {
    int seconds = 7384;
    write_long(seconds / 3600);
    write_string(" h ");
    write_long(seconds % 3600 / 60);
    write_string(" min ");
    write_long(seconds % 60);
    print(" s");

    int x = 4;
    int y = 7;
    int z = 8;
    double average = (x + y + z) / 3.0;
    print(average);

    print(8 / 3);
    print(8 % 3);
    return 0;
}
```

`(x + y + z) / 3.0` divides by a `double`, so the fraction is kept. `(x + y + z) / 3` would print 6.

</details>

## 6. Reading input

Programs become interesting when they react to the person using them. `stdio` reads one line at a time:

| Function | Reads |
|---|---|
| `read_int()` | a line as a whole number (`int`); 0 if it is not a number |
| `read_long()` | the same as a `long` |
| `read_double()` | a line as a `double` |
| `read_input(line)` | a line of text into a `char` array (lesson 11 explains arrays of characters) |

```c
import stdio;

int main() {
    write_string("First number: ");
    int a = read_int();
    write_string("Second number: ");
    int b = read_int();
    write_string("Sum: ");
    print(a + b);
    return 0;
}
```

```
First number: 5
Second number: 8
Sum: 13
```

The question uses `write_string`, so the answer is typed on the same line.

### Tasks

1. Ask for a temperature in Celsius and print it in Fahrenheit: F = C × 9 / 5 + 32. (37 gives 98.6.)
2. Ask for the width and height of a rectangle and print its area and perimeter.

<details><summary>Solution</summary>

```c
import stdio;

int main() {
    write_string("Celsius: ");
    double celsius = read_double();
    write_string("Fahrenheit: ");
    print(celsius * 9 / 5 + 32);

    write_string("Width: ");
    double width = read_double();
    write_string("Height: ");
    double height = read_double();
    write_string("Area: ");
    print(width * height);
    write_string("Perimeter: ");
    print(2 * (width + height));
    return 0;
}
```

</details>

## 7. Decisions

`if` runs statements only when a **condition** is true:

```c
import stdio;

int main() {
    write_string("Your score (0-100): ");
    int score = read_int();
    if (score >= 90) {
        print("Excellent");
    } else if (score >= 50) {
        print("Passed");
    } else {
        print("Not passed yet");
    }
    return 0;
}
```

The checks run from the top; the first true one wins, and `else` catches everything left.

| Comparison | True when |
|---|---|
| `a == b` | equal (two `=`; one `=` would be an assignment) |
| `a != b` | not equal |
| `a < b`, `a <= b` | less, less or equal |
| `a > b`, `a >= b` | greater, greater or equal |

A comparison gives a `bool`. Combine conditions with `&&` (and), `||` (or) and `!` (not):

```c
bool teenager = age >= 13 && age <= 19;
if (day == 6 || day == 7) {
    print("weekend");
}
if (!teenager) {
    print("not a teenager");
}
```

When one value is compared with many fixed values, `switch` is clearer:

```c
import stdio;

int main() {
    int day = 3;
    switch (day) {
        case 1:
            print("Monday");
        case 2:
            print("Tuesday");
        case 3:
            print("Wednesday");
        case 6, 7:
            print("Weekend");
        default:
            print("Another day");
    }
    return 0;
}
```

Only the matching case runs (here `Wednesday`); `default` runs when none matches. Unlike C, a razum case never "falls through" into the next one.

### Tasks

1. Read a number and print whether it is `even` or `odd`.
2. Read three numbers and print the largest.
3. Read a year and print whether it is a leap year: divisible by 4, except years divisible by 100, except years divisible by 400 (2000 and 2024 are leap years, 1900 and 2023 are not).

<details><summary>Solution</summary>

```c
import stdio;

int main() {
    write_string("Number: ");
    int n = read_int();
    if (n % 2 == 0) {
        print("even");
    } else {
        print("odd");
    }

    write_string("Three numbers, one per line: ");
    int a = read_int();
    int b = read_int();
    int c = read_int();
    int largest = a;
    if (b > largest) {
        largest = b;
    }
    if (c > largest) {
        largest = c;
    }
    print(largest);

    write_string("Year: ");
    int year = read_int();
    bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    print(leap);
    return 0;
}
```

</details>

## 8. Loops

A **loop** repeats statements. `while` repeats as long as its condition is true:

```c
import stdio;

int main() {
    int countdown = 3;
    while (countdown > 0) {
        print(countdown);
        countdown--;
    }
    print("Liftoff!");
    return 0;
}
```

```
3
2
1
Liftoff!
```

`for` puts the three parts of a counting loop on one line: the start, the condition, and the step after each round.

```c
import stdio;

int main() {
    int sum = 0;
    for (int i = 1; i <= 100; i++) {
        sum += i;
    }
    print(sum);

    for (int row = 1; row <= 3; row++) {
        for (int column = 1; column <= 5; column++) {
            write_long(row * column);
            write_char('\t');
        }
        print_line();
    }
    return 0;
}
```

```
5050
1	2	3	4	5
2	4	6	8	10
3	6	9	12	15
```

The inner loop runs completely for every round of the outer one: that is how tables and grids are built.

Inside a loop, `break` leaves the loop at once and `continue` skips to the next round. `do { ... } while (condition);` is a loop that always runs at least once, because it checks at the end.

A loop whose condition never becomes false runs forever. If that happens, press **Ctrl+C** in the terminal to stop the program.

### Tasks

1. Print the numbers 1 to 20 on one line, skipping multiples of 3 (use `continue`).
2. **FizzBuzz**: print 1 to 30, but `Fizz` instead of multiples of 3, `Buzz` instead of multiples of 5, and `FizzBuzz` instead of multiples of both.
3. **Guess the number**: the program picks a number from 1 to 100 with `random_int(1, 100)` (from `import math;`), and the player has 7 tries. After each guess it says `higher` or `lower`, and it stops on the right answer or after the last try.

<details><summary>Solution</summary>

```c
import stdio;

int main() {
    for (int i = 1; i <= 20; i++) {
        if (i % 3 == 0) {
            continue;
        }
        write_long(i);
        write_char(' ');
    }
    print_line();

    for (int i = 1; i <= 30; i++) {
        if (i % 15 == 0) {
            print("FizzBuzz");
        } else if (i % 3 == 0) {
            print("Fizz");
        } else if (i % 5 == 0) {
            print("Buzz");
        } else {
            print(i);
        }
    }
    return 0;
}
```

The `i % 15` check comes first: a number divisible by 15 is also divisible by 3, so checking 3 first would print `Fizz` for it.

```c
import stdio;
import math;

int main() {
    int secret = (int)random_int(1, 100);
    bool found = false;
    for (int attempt = 1; attempt <= 7 && !found; attempt++) {
        write_string("Your guess: ");
        int guess = read_int();
        if (guess < secret) {
            print("higher");
        } else if (guess > secret) {
            print("lower");
        } else {
            found = true;
        }
    }
    if (found) {
        print("You got it!");
    } else {
        write_string("Out of tries. It was ");
        print(secret);
    }
    return 0;
}
```

`random_int` gives a `long`, so `(int)` converts it. Seven tries are always enough if you halve the range each time: try 50, then 25 or 75, and so on.

</details>

## 9. Functions

A **function** is a named piece of program that you can use again and again. You have used library functions such as `print` and `read_int`; now you write your own.

```c
import stdio;

int square(int x) {
    return x * x;
}

void greet(string name, int times) {
    for (int i = 0; i < times; i++) {
        write_string("Hello, ");
        print(name);
    }
}

int main() {
    print(square(7));
    print(square(3) + square(4));
    greet("Ada", 2);
    return 0;
}
```

```
49
25
Hello, Ada
Hello, Ada
```

- `int square(int x)` gives back (**returns**) an `int` and takes one **parameter** `x`. The value in the call, `square(7)`, is the **argument**; it becomes `x` inside.
- `return` ends the function and hands the value back to the caller.
- `void` means the function returns nothing; it just does something.
- Variables declared inside a function exist only there. Two functions may each have their own `i`.
- Functions can be defined in any order, above or below the place that uses them.

Why bother? A name like `is_prime(n)` explains what code does; you fix a bug in one place instead of every copy; and `main` stays short enough to read.

A function may call itself. This is **recursion**, and it works when each call gets closer to a case that needs no further call:

```c
long factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

`factorial(4)` is `4 * factorial(3)` = `4 * 3 * factorial(2)` = `4 * 3 * 2 * 1` = 24.

### Tasks

1. Write `bool is_prime(int n)` (a prime has exactly two divisors, 1 and itself; 1 is not prime) and print all primes below 50.
2. Write `long raise_to(long base, int exponent)` that multiplies `base` by itself `exponent` times. `raise_to(2, 10)` is 1024.
3. Write `int count_digits(long n)` recursively: a number below 10 has one digit, otherwise it has one more digit than `n / 10`.

<details><summary>Solution</summary>

```c
import stdio;

bool is_prime(int n) {
    if (n < 2) {
        return false;
    }
    for (int divisor = 2; divisor * divisor <= n; divisor++) {
        if (n % divisor == 0) {
            return false;
        }
    }
    return true;
}

long raise_to(long base, int exponent) {
    long result = 1;
    for (int i = 0; i < exponent; i++) {
        result *= base;
    }
    return result;
}

int count_digits(long n) {
    if (n < 10) {
        return 1;
    }
    return 1 + count_digits(n / 10);
}

int main() {
    for (int n = 1; n < 50; n++) {
        if (is_prime(n)) {
            write_long(n);
            write_char(' ');
        }
    }
    print_line();
    print(raise_to(2, 10));
    print(count_digits(2026));
    return 0;
}
```

`is_prime` only tests divisors up to the square root (`divisor * divisor <= n`): if `n` had a larger divisor, it would also have a smaller partner below the square root.

</details>

## 10. Arrays

An **array** holds a fixed number of values of one type, side by side. Each value (an **element**) has a number, its **index**, starting at **0**.

```c
import stdio;

int main() {
    int scores[5] = {72, 95, 61, 88, 79};
    print(scores[0]);
    print(scores[4]);
    scores[2] = 65;
    print(scores.length);

    int total = 0;
    int best = scores[0];
    for (int i = 0; i < scores.length; i++) {
        total += scores[i];
        if (scores[i] > best) {
            best = scores[i];
        }
    }
    print(total);
    print(best);
    return 0;
}
```

```
72
79
5
399
95
```

- `int scores[5]` makes five `int`s; `{...}` gives their starting values. Elements you do not list start at 0.
- `scores[i]` is element number `i`. The last one is `scores[scores.length - 1]`.
- `.length` is the number of elements. Loop with `i < scores.length`, never `<=`.
- An index outside the array is a serious mistake: with a fixed number (`scores[5]`) `rzc` reports it, but with a computed index the program reads or overwrites other memory. Always keep indexes between 0 and `length - 1`.

Arrays can be passed to functions. The function then works on the **caller's** elements, not on a copy, so it can change them:

```c
void fill(int values[], int value) {
    for (int i = 0; i < values.length; i++) {
        values[i] = value;
    }
}
```

Write the parameter without a size (`int values[]`); `values.length` still works inside.

### Tasks

1. Print the `scores` array backwards.
2. Write `double average(int values[])` and count how many scores are above the average.
3. **Sort** the array from smallest to largest with bubble sort: walk through the array, swapping neighbours that are in the wrong order, and repeat until a walk makes no swap.

<details><summary>Solution</summary>

```c
import stdio;

double average(int values[]) {
    long total = 0;
    for (int i = 0; i < values.length; i++) {
        total += values[i];
    }
    return (double)total / values.length;
}

void sort(int values[]) {
    bool swapped = true;
    while (swapped) {
        swapped = false;
        for (int i = 0; i + 1 < values.length; i++) {
            if (values[i] > values[i + 1]) {
                int keep = values[i];
                values[i] = values[i + 1];
                values[i + 1] = keep;
                swapped = true;
            }
        }
    }
}

int main() {
    int scores[5] = {72, 95, 61, 88, 79};
    for (int i = scores.length - 1; i >= 0; i--) {
        write_long(scores[i]);
        write_char(' ');
    }
    print_line();

    double mean = average(scores);
    int above = 0;
    for (int i = 0; i < scores.length; i++) {
        if (scores[i] > mean) {
            above++;
        }
    }
    print(mean);
    print(above);

    sort(scores);
    for (int i = 0; i < scores.length; i++) {
        write_long(scores[i]);
        write_char(' ');
    }
    print_line();
    return 0;
}
```

Swapping needs a third variable (`keep`): after `values[i] = values[i + 1];` the old value would be gone otherwise.

</details>

### The short way

Writing a sort once teaches you how it works. After that, let the `array` library do it: it is much faster on large arrays and handles every number type.

```c
import stdio;
import array;

int main() {
    int scores[6] = {72, 95, 61, 88, 79, 95};
    print(scores.first);            // 72, the first element
    print(scores.last);             // 95, the last one: no "- 1" needed
    scores.sort_descending();       // largest first: 95 95 88 79 72 61
    print(scores.first);            // 95
    print(scores.max());            // 95
    print(scores.sum());            // 490
    print(scores.index_of(88));     // 2
    scores.reverse();               // 61 72 79 88 95 95
    return 0;
}
```

`.first` and `.last` work without any import. The rest is in [libraries.md](libraries.md#array): `sort`, `sort_descending`, `reverse`, `fill`, `contains`, `index_of`, `min`, `max`, `sum`.

## 11. Text

A `string` points at text that already exists, such as `"hello"`. You can print it, pass it around and read its characters with `text[i]`, but not change it. To **build** or **change** text, use an array of characters, `char name[32]`, which has room for up to 31 characters plus an invisible end marker `'\0'`.

razum has no `+` or `==` for text; the `text` library has functions instead:

```c
import stdio;
import text;

int main() {
    char name[32];
    write_string("What is your name? ");
    read_input(name);

    char greeting[64] = "Hello, ";
    concat(greeting, name);
    concat(greeting, "!");
    print(greeting);

    write_string("Letters: ");
    print(length(name));
    print(name[0]);

    if (equals(name, "Ada")) {
        print("Nice to meet you, Countess.");
    }

    to_upper(name);
    print(name);

    char number[16];
    int_to_text(number, 2026);
    concat(greeting, number);
    print(greeting);
    return 0;
}
```

```
What is your name? Ada
Hello, Ada!
Letters: 3
A
Nice to meet you, Countess.
ADA
Hello, Ada!2026
```

| Function | Does |
|---|---|
| `length(text)` | number of characters |
| `copy(target, text)` | replaces the text in a `char` array |
| `concat(target, text)` | appends to the text in a `char` array |
| `equals(a, b)` | `true` if the texts are the same |
| `find(text, part)` | where `part` starts, or -1 |
| `to_upper(target)`, `to_lower(target)` | changes letter case in place |
| `int_to_text(target, number)`, `parse_int(text)` | number to text and back |

[libraries.md](libraries.md#text) has the full list. These functions never write past the end of the array; text that does not fit is cut off.

A `char` is a small number (its character code), so characters can be compared: `c >= "a" && c <= "z"` is true for a lowercase letter.

### Tasks

1. Read a word and count its vowels (a, e, i, o, u, upper or lower case). Tip: `lower_char(c)` gives the lowercase version of a character.
2. Read a word and print whether it is a **palindrome** (the same backwards, like `level` or `racecar`).

<details><summary>Solution</summary>

```c
import stdio;
import text;

bool is_vowel(char c) {
    switch (lower_char(c)) {
        case "a", "e", "i", "o", "u":
            return true;
        default:
            return false;
    }
}

bool is_palindrome(string word) {
    int size = (int)length(word);
    for (int i = 0; i < size / 2; i++) {
        if (word[i] != word[size - 1 - i]) {
            return false;
        }
    }
    return true;
}

int main() {
    char word[64];
    write_string("Word: ");
    read_input(word);
    int vowels = 0;
    for (int i = 0; i < length(word); i++) {
        if (is_vowel(word[i])) {
            vowels++;
        }
    }
    write_string("Vowels: ");
    print(vowels);
    write_string("Palindrome: ");
    print(is_palindrome(word));
    return 0;
}
```

The palindrome check compares the first character with the last, the second with the second-to-last, and so on, until the middle.

</details>

## 12. Structs

A **struct** groups values that belong together into one new type. Each part is a **field**.

```c
import stdio;
import math;

struct Point {
    double x;
    double y;
}

struct Student {
    char name[16];
    int score;
}

double distance(Point a, Point b) {
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    return sqrt(dx * dx + dy * dy);
}

void add_bonus(Student student, int bonus) {
    student.score += bonus;
}

int main() {
    Point home = {0, 0};
    Point school = {3, 4};
    print(distance(home, school));

    Student class_list[3] = {{"Ada", 91}, {"Linus", 78}, {"Grace", 85}};
    add_bonus(class_list[1], 5);
    for (int i = 0; i < class_list.length; i++) {
        write_string(class_list[i].name);
        write_string(": ");
        print(class_list[i].score);
    }
    return 0;
}
```

```
5.0
Ada: 91
Linus: 83
Grace: 85
```

- `struct Point { ... }` defines the type; `Point home = {0, 0};` makes a variable of it, filling the fields in order.
- `home.x` is a field. Fields can be any type, including arrays and other structs.
- `Point copy = home;` copies all fields at once.
- A struct parameter works on the caller's struct, like an array, which is why `add_bonus` changed Linus's score.

### Tasks

1. Make `struct Rectangle { double width; double height; }` and functions `double area(Rectangle r)` and `double perimeter(Rectangle r)`.
2. Using `class_list`, print the name of the student with the best score.

<details><summary>Solution</summary>

```c
import stdio;

struct Rectangle {
    double width;
    double height;
}

struct Student {
    char name[16];
    int score;
}

double area(Rectangle r) {
    return r.width * r.height;
}

double perimeter(Rectangle r) {
    return 2 * (r.width + r.height);
}

int main() {
    Rectangle door = {0.9, 2.0};
    print(area(door));
    print(perimeter(door));

    Student class_list[3] = {{"Ada", 91}, {"Linus", 78}, {"Grace", 85}};
    int best = 0;
    for (int i = 1; i < class_list.length; i++) {
        if (class_list[i].score > class_list[best].score) {
            best = i;
        }
    }
    print(class_list[best].name);
    return 0;
}
```

Remembering the **index** of the best student, rather than only the best score, lets you print any of its fields afterwards.

</details>

## 12b. Constants and enums

Some values never change: the size of a board, the number of lives, a tax rate. Give them a name with `const`:

```c
import stdio;

const int SIZE = 3;
const int MAX_LIVES = 3;
const double TAX = 0.2;

int main() {
    char board[SIZE][SIZE];
    print(board.length);
    print(100 * (1 + TAX));
    return 0;
}
```

```
3
120.0
```

- A `const` must get its value where it is declared, and nothing can change it later. `MAX_LIVES = 4;` is an error.
- The value must be known when compiling: numbers, text, other constants, and operators on them (`SIZE * SIZE`).
- Constants may give array sizes, which variables may not.
- By habit, constants are written in `UPPER_CASE`, so a reader sees at once that they never change.

When a value is one of a few choices (a direction, a colour, the state of a game), an **enum** names the choices:

```c
import stdio;

enum Direction { NORTH, EAST, SOUTH, WEST }

Direction turn_right(Direction direction) {
    if (direction == Direction.WEST) {
        return Direction.NORTH;
    }
    return (Direction)(direction + 1);
}

string direction_name(Direction direction) {
    switch (direction) {
        case Direction.NORTH:
            return "north";
        case Direction.EAST:
            return "east";
        case Direction.SOUTH:
            return "south";
        default:
            return "west";
    }
}

int main() {
    Direction facing = Direction.NORTH;
    for (int i = 0; i < 5; i++) {
        print(direction_name(facing));
        facing = turn_right(facing);
    }
    return 0;
}
```

```
north
east
south
west
north
```

Each member is a number underneath (`NORTH` is 0, `EAST` 1, ...), but the enum keeps you from mixing them up: `Direction facing = 2;` is an error, because 2 is not a direction. `(Direction)2` converts on purpose.

### Tasks

1. Make `enum Weekday { MONDAY, ..., SUNDAY }` and a function `bool is_weekend(Weekday day)`.
2. A traffic light goes `RED` → `GREEN` → `YELLOW` → `RED`. Write `Light next_light(Light light)` with a `switch`, and print six steps starting at `RED`.

<details><summary>Solution</summary>

```c
import stdio;

enum Weekday { MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY }

enum Light { RED, GREEN, YELLOW }

bool is_weekend(Weekday day) {
    return day == Weekday.SATURDAY || day == Weekday.SUNDAY;
}

Light next_light(Light light) {
    switch (light) {
        case Light.RED:
            return Light.GREEN;
        case Light.GREEN:
            return Light.YELLOW;
        default:
            return Light.RED;
    }
}

int main() {
    print(is_weekend(Weekday.SUNDAY));
    print(is_weekend(Weekday.MONDAY));
    Light light = Light.RED;
    for (int step = 0; step < 6; step++) {
        print(light);
        light = next_light(light);
    }
    return 0;
}
```

`print(light)` prints the member's number: 0, 1, 2, 0, 1, 2.

</details>

## 13. Splitting a program into files

When a program grows, put related functions in their own file, a **module**, and `import` it. Make a folder with two files:

`geometry.rz`:

```c
double circle_area(double radius) {
    return 3.141592653589793 * radius * radius;
}

double square_area(double side) {
    return side * side;
}
```

`app.rz`:

```c
import stdio;
import geometry;

int main() {
    print(circle_area(2));
    print(square_area(3));
    return 0;
}
```

```powershell
rzc app.rz --run
```

`import geometry;` finds `geometry.rz` next to `app.rz` (or a folder `geometry\` full of `.rz` files). A module has no `main`. Everything in it can be used by its plain name, so give functions names that will not clash with others. Only the functions the program actually calls end up in the `.exe`.

The libraries you have used (`stdio`, `text`, `math`) are modules too; they are built into `rzc`.

### Task

Move `is_prime` and `count_digits` from lesson 9 into `numbers.rz` and use them from a `main` in another file.

<details><summary>Solution</summary>

`numbers.rz` holds the two functions exactly as in lesson 9, without `main` and without `import stdio;` (they do not print). The program:

```c
import stdio;
import numbers;

int main() {
    print(is_prime(97));
    print(count_digits(123456));
    return 0;
}
```

</details>

## 14. Pointers and the heap

Every variable lives somewhere in memory, at an **address**. A **pointer** is a variable that holds an address. This lesson is a first look; [language.md](language.md#pointers) has the details.

| Write | Means |
|---|---|
| `int *p` | `p` holds the address of an `int` |
| `&x` | the address of `x` |
| `*p` | the value at the address in `p` |
| `null` | "no address" |

A function receives copies of plain values like `int`. With pointers it can change the caller's variables:

```c
import stdio;

void swap(int *a, int *b) {
    int keep = *a;
    *a = *b;
    *b = keep;
}

int main() {
    int x = 1;
    int y = 2;
    swap(&x, &y);
    print(x);
    print(y);

    write_string("How many squares? ");
    int count = read_int();
    if (count < 1) {
        count = 1;
    }
    long *squares = new long[count];
    for (int i = 0; i < count; i++) {
        squares[i] = (long)i * i;
    }
    print(squares[count - 1]);
    free(squares);
    return 0;
}
```

An array's size is fixed when you write the program. `new long[count]` instead asks for memory while the program runs, on the **heap**, so the size can come from the user. The memory stays until you give it back with `free`. Using a pointer after `free`, or one that is `null`, crashes the program, so free memory exactly once, when you no longer need it.

### Task

Read a number `n`, make an array of `n` `double`s with `new`, fill it with 1, 1/2, 1/3, ..., 1/n, print their sum, and free the array.

<details><summary>Solution</summary>

```c
import stdio;

int main() {
    write_string("n: ");
    int n = read_int();
    if (n < 1) {
        print("n must be at least 1");
        return 1;
    }
    double *parts = new double[n];
    for (int i = 0; i < n; i++) {
        parts[i] = 1.0 / (i + 1);
    }
    double sum = 0;
    for (int i = 0; i < n; i++) {
        sum += parts[i];
    }
    print(sum);
    free(parts);
    return 0;
}
```

Returning 1 from `main` tells whoever started the program that something went wrong.

</details>

## 15. Functions as values

A function can be stored in a variable and passed to another function. The type `fn(int, int) -> int` means "a function that takes two `int`s and returns an `int`":

```c
import stdio;

int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int combine(int values[], fn(int, int) -> int operation, int start) {
    int result = start;
    for (int i = 0; i < values.length; i++) {
        result = operation(result, values[i]);
    }
    return result;
}

int main() {
    int values[4] = {1, 2, 3, 4};
    print(combine(values, add, 0));
    print(combine(values, multiply, 1));
    fn(int, int) -> int chosen = multiply;
    print(chosen(6, 7));
    return 0;
}
```

```
10
24
42
```

`combine` does not know what it combines with; the caller decides by passing a function. Windows uses the same idea for **callbacks**: you hand it one of your functions, and it calls it when something happens.

### Task

Write `int largest(int a, int b)` and use `combine` to find the largest value in `{4, 17, 9, 12}`.

<details><summary>Solution</summary>

Add this function to the example and call `print(combine(values, largest, values[0]));` with the new values:

```c
int largest(int a, int b) {
    return a > b ? a : b;
}
```

`condition ? a : b` is a short `if` that gives a value: `a` when the condition is true, otherwise `b`.

</details>

## 16. A window with buttons

The `gui` library opens a real window. Each time the window is drawn (a **frame**), your loop says what it contains, from top to bottom. Buttons return `true` in the frame they are clicked, so you react with a plain `if`:

```c
import gui;
import text;

int clicks = 0;
bool dark = true;

int main() {
    if (!gui_open("Counter", 360, 240)) {
        return 1;
    }
    char line[32];
    char number[16];
    while (gui_frame()) {
        gui_heading("Counter");
        int_to_text(number, clicks);
        copy(line, "Clicks: ");
        concat(line, number);
        gui_text(line);
        if (gui_button("Click me")) {
            clicks++;
        }
        if (gui_switch("Dark mode", &dark)) {
            gui_set_dark(dark);
        }
    }
    gui_close();
    return 0;
}
```

`gui_frame()` returns `false` when the window is closed, which ends the loop. Variables that must survive from one frame to the next, such as `clicks`, live outside the loop. [libraries.md](libraries.md#gui) lists every element: text fields, sliders, checkboxes, panels, rows and more; [examples/gui_demo.rz](../examples/gui_demo.rz) is a larger window.

### Task

Put a `Count` and a `Reset` button side by side (between `gui_begin_row();` and `gui_end();`) and show a progress bar (`gui_progress(fraction)`, fraction from 0 to 1) that is full after 10 clicks.

<details><summary>Solution</summary>

```c
import gui;
import text;

int clicks = 0;

int main() {
    if (!gui_open("Counter", 360, 240)) {
        return 1;
    }
    char line[32];
    while (gui_frame()) {
        gui_heading("Counter");
        copy(line, "Clicks: ");
        char number[16];
        int_to_text(number, clicks);
        concat(line, number);
        gui_text(line);
        gui_begin_row();
        if (gui_button("Count")) {
            clicks++;
        }
        if (gui_button_quiet("Reset")) {
            clicks = 0;
        }
        gui_end();
        gui_progress(clicks / 10.0);
    }
    gui_close();
    return 0;
}
```

`clicks / 10.0` divides by a `double`, so 3 clicks give 0.3 instead of 0.

</details>

## 17. Reading error messages

Errors are normal; every programmer sees them all day. `rzc` reports each one as `file:line:column: error: message`, all at once, and VS Code underlines them. Hover over a name to see its type, and press F12 to jump to where it is declared. Read the message, go to the line, and look at the column.

| Message | Cause | Fix |
|---|---|---|
| `expected ';', found 'return'` | a missing `;` at the end of the line **before** | add the `;` |
| `'totl' is not declared` | a typo, or a variable used outside the block that declares it | check the spelling and where it was declared |
| `cannot store a double in int 'a'; convert it with (int)` | a value that does not fit the variable's type | change the type, or cast if you really want to cut the fraction |
| `'+' needs a number, not a string` | `+` on text | use `concat` from `text` (lesson 11) |
| `'==' needs a number, not a string` | comparing text with `==` | use `equals(a, b)` |
| `'twice' can reach its end without returning an int` | a path through the function has no `return` | return a value on every path |
| `print needs 'import stdio;' at the top of the file` | a library function without its `import` | add the import |
| `index 3 is outside an array of 3 elements (0 to 2)` | indexes start at 0 | the last element of `a[3]` is `a[2]` |
| `'LIMIT' is a const; its value cannot change` | assigning to a constant | use a variable if the value must change |
| `cannot store an int in Color 'c'; convert it with (Color)` | a plain number where an enum is expected | use a member such as `Color.RED`, or cast |
| `'++' is a statement of its own in razum, not part of an expression` | `y = x++;` | write `x++;` on its own line, then `y = x;` |

When a program compiles but does the wrong thing, add `print` calls to see what the values really are at each step. That simple habit finds most bugs.

## 18. Final project: a grade book

Put everything together. Write a program that:

1. asks how many students there are (at most 20),
2. reads each student's name and score,
3. prints a table with each name, score and `pass` (score 50 or more) or `fail`,
4. prints the average and the best student.

Plan before you type: which struct do you need, which functions (reading, printing, average, best), and what does `main` call in which order?

<details><summary>Solution</summary>

```c
import stdio;
import text;

struct Student {
    char name[32];
    int score;
}

int read_count() {
    write_string("How many students (1-20)? ");
    int count = read_int();
    if (count < 1) {
        return 1;
    }
    if (count > 20) {
        return 20;
    }
    return count;
}

void read_student(Student student, int number) {
    write_string("Name of student ");
    write_long(number);
    write_string(": ");
    read_input(student.name);
    write_string("Score: ");
    student.score = read_int();
}

double average_score(Student students[], int count) {
    long total = 0;
    for (int i = 0; i < count; i++) {
        total += students[i].score;
    }
    return (double)total / count;
}

int best_student(Student students[], int count) {
    int best = 0;
    for (int i = 1; i < count; i++) {
        if (students[i].score > students[best].score) {
            best = i;
        }
    }
    return best;
}

void print_row(Student student) {
    write_string(student.name);
    for (int i = (int)length(student.name); i < 12; i++) {
        write_char(' ');
    }
    write_long(student.score);
    write_char('\t');
    print(student.score >= 50 ? "pass" : "fail");
}

int main() {
    Student students[20];
    int count = read_count();
    for (int i = 0; i < count; i++) {
        read_student(students[i], i + 1);
    }
    print_line();
    for (int i = 0; i < count; i++) {
        print_row(students[i]);
    }
    write_string("Average: ");
    print(average_score(students, count));
    write_string("Best: ");
    print(students[best_student(students, count)].name);
    return 0;
}
```

The array holds 20 students, but only the first `count` are used, so each function takes `count` instead of `students.length`. `print_row` pads names with spaces to 12 characters so the scores line up.

</details>

## 19. Where to go next

You now know the core of the language: values and types, decisions, loops, functions, arrays, text, structs, constants and enums, modules, pointers and function values. From here:

| To | Read |
|---|---|
| Look up any rule exactly | [language.md](language.md) |
| Find a library function | [libraries.md](libraries.md) |
| Call any Windows function (message boxes, files, the clock) | [libraries.md, windows](libraries.md#windows) and [examples/windows_tour.rz](../examples/windows_tour.rz) |
| Build windows with controls | [libraries.md, gui](libraries.md#gui) and [examples/gui_demo.rz](../examples/gui_demo.rz) |
| See what your code becomes in the processor | `rzc file.rz --asm`, then [codegen.md](codegen.md) |
| Write assembly inside razum | [language.md, asm blocks](language.md#asm-blocks) and [examples/asm_tour.rz](../examples/asm_tour.rz) |
| Make a DLL that C or C++ can use, or mix languages with CMake | [language.md, DLLs](language.md#making-a-dll) and [cmake.md](cmake.md) |

Good next projects: a unit converter with a menu, a to-do list saved to a file (`filesystem` library), a dice game with `random_int`, or a small calculator window with `gui`.
