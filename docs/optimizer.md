# The optimizer

`rzc -O program.rz` optimizes for portable x86-64. `rzc -march=znver3 program.rz` enables the same optimizations and allows instructions supported by AMD Zen 3. Use the portable mode for binaries you intend to run on other machines. The compiler does not detect the build machine and silently change the output's CPU requirements.

## What changes

- Scalar locals and parameters can stay in registers. Integers and pointers use `rbx`, `rsi`, `rdi`, and `r12` through `r15`. Floating-point values use `xmm6` through `xmm15`. The function saves the registers it uses. Taking a variable's address, passing an array with a hidden length, or using inline assembly keeps the relevant values in their existing stack slots.
- When eligible values outnumber integer registers, a value referenced in loops can displace a colder value to its stack slot. Nested loops and references used in scaled addresses carry more weight. The allocator preserves the single wide integer parameter that tail-recursion lowering may need. [Register allocation measurements](../KNOWLEDGE.md#current-compiler-benchmarks)
- Register parameters can go directly into their assigned registers. A direct internal call with one scalar argument can compute it directly into the argument register. Computed call arguments still run in source order.
- Small direct calls that return `long`, `ulong`, `bool` or `double` can be inlined. The compiler substitutes literals and unaliased local values into a short return expression or an `if` with two return paths. Calls with computed arguments use the normal call path. Simple integer selections can use a conditional move.
- Pure integer and double expression trees use scratch registers when they fit. Integer address arithmetic can use `lea`, and a pointer and index that already live in registers can be used directly by loads and stores. Signed division by a positive power of two still rounds toward zero.
- `for` and `while` can enter at the condition and take a conditional branch back to the body. A simple bit-mask comparison can use `test`. A small conditional assignment to a register can use `cmov` when evaluating either alternative early has no visible effects.
- A floating-point loop test can keep up to two products of register doubles for reuse in the body. Products are still computed at their original point in the test, after any short-circuit guard. The reuse window ends when an operand changes or a statement may have side effects. Pure double assignments can also write directly to their destination register when it is independent of the expression.
- A `for` or `while` loop can evaluate a leading declaration once when the initializer is pure integer arithmetic or an indexed address and the loop leaves its inputs and result unchanged. The first condition still runs before the initializer. This preserves short-circuit behavior and avoids evaluating a division when the loop runs zero times.
- Simple increasing `for` loops with an unchanged upper bound can process several integer elements at once. Supported bodies include wrapping sums and contiguous stores with a pure 32- or 64-bit integer expression. Read-only reductions can load contiguous elements. Stores can read the same element they write or read from a separate fixed array. Potentially overlapping pointers fall back to scalar code. The vector code processes complete groups and the regular scalar loop handles the remainder.
- Some nested double loops, including Mandelbrot, can process independent outer iterations together. Each iteration initializes local double state, advances it in a bounded inner `while`, and increments a total only if it reaches the step limit. The inner condition and updates must use pure additions, subtractions, multiplications, and an ordered double comparison. The mask tracks which lanes can still run, and finished lanes are zeroed so they cannot raise floating-point exceptions. Operations retain their rounding and order within each lane. Portable `-O` uses two SSE2 lanes and `-march=znver3` uses four AVX2 lanes. The scalar loop handles the tail, enabled floating-point traps, and unordered SSE2 comparisons.
- Eligible single-argument integer self-recursion reuses its frame for a tail call, including the right side of a wrapping integer sum. The prefix of the sum is still evaluated before the recursive arguments. A small base case can return before saving registers. For a pure recurrence with two or more recursive calls on strictly smaller inputs, the compiler can precompute 16 small results. The table only applies to the recognized recurrence and its input range.
- Portable `-O` uses SSE2 for vector loops. `-march=znver3` uses AVX2 for wider vector loops, BMI2 variable shifts (`shlx`, `shrx`, `sarx`), and AVX three-operand scalar math. Floating-point operations retain their separate rounding.

The compiler retains the Windows x64 calling convention on both platforms. Calls, writes through pointers, and floating-point rounding limit which computations can be moved or combined. Without `-O` or a CPU target, the assembly stays in the original style.

## Benchmarks

Run the same six workloads against GCC, G++, and Rust with the same target:

~~~powershell
python benchmarks/run.py --compare c --compare cpp --compare rust --cpu znver3 -- -O -march=znver3
~~~

Use `--baseline PATH` to compare a previous `rzc -O` in the same run. `--goose PATH` adds a comparable Fibonacci build using Goose's GCC backend. `--only fib` runs just that workload. `--c-opt 2` uses `-O2` for GCC and G++. Every executable must print the same output on every run. The runner measures whole-process time, discards one warm-up, rotates run order, and reports the median. Fibonacci gets its input at runtime to keep the comparison from becoming a constant-folding test.

See [the masked double loop results](../KNOWLEDGE.md#masked-double-loops), [the inlining and invariant measurements](../KNOWLEDGE.md#inlining-and-loop-invariants) and [the earlier optimization measurements](../KNOWLEDGE.md#compiler-optimization-on-zen-3).
