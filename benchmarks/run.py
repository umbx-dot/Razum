import argparse
import os
import shutil
import statistics
import subprocess
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
EXE = ".exe" if os.name == "nt" else ""


def build(work, source, arguments):
    name = source.stem
    commands = {
        "razum": [Path(arguments.rzc).resolve(), source, *arguments.flags],
    }
    if arguments.baseline:
        commands["old -O"] = [Path(arguments.baseline).resolve(), source, "-O"]

    c_flags = [f"-O{arguments.c_opt}", "-fwrapv", "-ffp-contract=off"]
    if arguments.cpu:
        c_flags.append(f"-march={arguments.cpu}")
    for language in arguments.compare:
        if language == "rust":
            rust_flags = ["-C", "opt-level=3"]
            if arguments.cpu:
                rust_flags += ["-C", f"target-cpu={arguments.cpu}"]
            commands["Rust"] = ["rustc", *rust_flags, HERE / "rust" / f"{name}.rs"]
        else:
            c_source = HERE / "c" / f"{name}.c"
            if language == "cpp":
                commands["C++"] = ["g++", "-x", "c++", *c_flags, c_source]
            else:
                commands["C"] = ["gcc", *c_flags, c_source]

    goose_source = HERE / "goose" / f"{name}.goose"
    if arguments.goose and goose_source.is_file():
        commands["Goose"] = [Path(arguments.goose).resolve(), "build", "--cc", "gcc", goose_source]

    programs = {}
    for index, (label, command) in enumerate(commands.items()):
        exe = work / f"{name}-{index}{EXE}"
        env = os.environ.copy()
        if label == "Goose":
            env["GOOSE_HOME"] = str(work / "goose-home")
        subprocess.run([*map(str, command), "-o", str(exe)], check=True, env=env)
        programs[label] = exe
    return programs


def measure(programs, runs, inputs):
    expected = None
    times = {label: [] for label in programs}
    entries = list(programs.items())
    for iteration in range(runs + 1):
        offset = iteration % len(entries)
        for label, exe in entries[offset:] + entries[:offset]:
            start = time.perf_counter()
            result = subprocess.run([str(exe), *inputs], capture_output=True, check=True)
            elapsed = time.perf_counter() - start
            output = result.stdout.replace(b"\r\n", b"\n")
            if expected is None:
                expected = output
            elif output != expected:
                raise SystemExit(f"{exe.name} ({label}) printed {output!r}, expected {expected!r}")
            if iteration:
                times[label].append(elapsed * 1000)
    return {label: statistics.median(samples) for label, samples in times.items()}


def main():
    parser = argparse.ArgumentParser(description="compare identical benchmark outputs and median runtimes")
    parser.add_argument("--rzc", default=str(Path(os.environ.get("RZ_BUILD", ROOT / "build")) / f"rzc{EXE}"))
    parser.add_argument("--baseline", help="previous rzc, compiled with -O")
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--only", choices=[source.stem for source in sorted((HERE / "razum").glob("*.rz"))])
    parser.add_argument("--compare", choices=["c", "cpp", "rust"], action="append", help="repeat to select peers (default: c)")
    parser.add_argument("--cpu", help="architecture for GCC, G++ and rustc, for example znver3")
    parser.add_argument("--c-opt", choices=[2, 3], default=3, type=int, help="GCC and G++ optimization level (default: 3)")
    parser.add_argument("--goose", help="optional Goose compiler path (Fibonacci)")
    parser.add_argument("flags", nargs="*", help="extra rzc flags, after --")
    arguments = parser.parse_args()
    arguments.compare = arguments.compare or ["c"]
    if arguments.runs < 1:
        parser.error("--runs must be positive")
    for language in dict.fromkeys(arguments.compare + (["c"] if arguments.goose else [])):
        compiler = {"c": "gcc", "cpp": "g++", "rust": "rustc"}[language]
        if shutil.which(compiler) is None:
            parser.error(f"{compiler} is not on PATH")

    work = (Path(os.environ.get("RZ_BUILD", ROOT / "build")) / "benchmarks").resolve()
    work.mkdir(parents=True, exist_ok=True)
    print("Median runtime (ms), identical output required", flush=True)
    for source in sorted((HERE / "razum").glob("*.rz")):
        if arguments.only and source.stem != arguments.only:
            continue
        programs = build(work, source, arguments)
        inputs = ["38"] if source.stem == "fib" else []
        measured = measure(programs, arguments.runs, inputs)
        columns = "  ".join(f"{label} {elapsed:7.2f}" for label, elapsed in measured.items())
        print(f"{source.stem:<10} {columns}  ({programs['razum'].stat().st_size} bytes)", flush=True)


if __name__ == "__main__":
    main()
