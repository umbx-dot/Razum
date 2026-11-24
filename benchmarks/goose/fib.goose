import std;
import os;

recursive fn fib(n: i64) -> i64 {
    if n < 2 {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

fn main() {
    let argv = args();
    if argv.len < 2 {
        print(fib(38));
        return;
    }
    let n, ok = parse_int(argv[1]);
    guard ok else { abort("expected an integer"); }
    print(fib(n));
}
