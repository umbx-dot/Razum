import argparse
import os
import queue
import socket
import statistics
import subprocess
import tempfile
import threading
import time
from pathlib import Path
from typing import NamedTuple


ROOT = Path(__file__).resolve().parent.parent
WINDOWS = os.name == "nt"
HOST = "windows" if WINDOWS else "linux"
EXE = ".exe" if WINDOWS else ""
ENV = {**os.environ, "RZ_LIBRARY_BENCHMARK": "razum"}
STDIO = b"".join(f"value={i}\n".encode() for i in range(120000))


class Case(NamedTuple):
    name: str
    arguments: tuple[str, ...] = ()
    host: str = ""


CASES = (
    Case("array"),
    Case("crypt.sha256", ("sha256",)),
    Case("crypt.aes_gcm", ("aes_gcm",)),
    Case("filesystem"),
    Case("gui"),
    Case("http"),
    Case("json"),
    Case("linux", host="linux"),
    Case("math"),
    Case("process"),
    Case("program", ("value",)),
    Case("stdio"),
    Case("text.early", ("early",)),
    Case("text.late", ("late",)),
    Case("threads"),
    Case("time"),
    Case("windows", host="windows"),
    Case("winsock", host="windows"),
)


def library(case):
    return case.name.split(".", 1)[0]


def bench_http(executable):
    with subprocess.Popen([str(executable)], stdin=subprocess.PIPE, stdout=subprocess.PIPE) as server:
        ready = queue.Queue()
        listener = threading.Thread(target=lambda: ready.put(server.stdout.readline()), daemon=True)
        listener.start()
        try:
            port = int(ready.get(timeout=10))
            request = f"GET / HTTP/1.1\r\nHost: 127.0.0.1:{port}\r\nConnection: keep-alive\r\n\r\n".encode()
            with socket.create_connection(("127.0.0.1", port), timeout=10) as client, client.makefile("rb") as stream:
                client.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                start = time.perf_counter_ns()
                for _ in range(200):
                    client.sendall(request * 128)
                    for _ in range(128):
                        if stream.readline() != b"HTTP/1.1 200 OK\r\n":
                            raise RuntimeError("HTTP response status did not match")
                        headers = {}
                        for line in iter(stream.readline, b"\r\n"):
                            name, value = line.split(b":", 1)
                            headers[name.lower()] = value.strip()
                        if headers.get(b"content-length") != b"5" or stream.read(5) != b"probe":
                            raise RuntimeError("HTTP response body did not match")
                return time.perf_counter_ns() - start
        finally:
            server.stdin.close()
            try:
                server.wait(timeout=10)
            except subprocess.TimeoutExpired:
                server.kill()
                raise
            if server.returncode:
                raise RuntimeError(f"HTTP server exited with status {server.returncode}")


def run(case, executable):
    if case.name == "http":
        return bench_http(executable)
    start = time.perf_counter_ns()
    result = subprocess.run(
        [str(executable), *case.arguments], cwd=executable.parent, env=ENV,
        capture_output=True, timeout=60, check=True,
    )
    elapsed = time.perf_counter_ns() - start
    if library(case) == "text":
        elapsed = int(result.stdout)
        if elapsed <= 0:
            raise RuntimeError(f"{case.name} reported an invalid elapsed time")
    else:
        expected = STDIO if case.name == "stdio" else b""
        if result.stdout != expected:
            raise RuntimeError(f"{case.name} printed unexpected output")
    return elapsed


def build(rzc, case, executable, flags):
    if not executable.exists():
        source = ROOT / "benchmarks" / "libraries" / f"{library(case)}_probe.rz"
        subprocess.run([str(rzc), str(source), *flags, "-o", str(executable)], check=True)
    return executable


def measure(case, programs, runs):
    samples = [[] for _ in programs]
    for iteration in range(runs + 1):
        for offset in range(len(programs)):
            index = (offset + iteration) % len(programs)
            elapsed = run(case, programs[index])
            if iteration:
                samples[index].append(elapsed)
    return [statistics.median(times) / 1_000_000 for times in samples]


def main():
    parser = argparse.ArgumentParser(description="benchmark each Razum library with a representative workload")
    default_build = Path(os.environ.get("RZ_BUILD", ROOT / "build"))
    parser.add_argument("--rzc", type=Path, default=default_build / f"rzc{EXE}")
    parser.add_argument("--baseline", type=Path, help="previous compiler built for the same host")
    parser.add_argument("--only", choices=sorted({library(case) for case in CASES}))
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--zen3", action="store_true")
    args = parser.parse_args()
    if args.runs < 1:
        parser.error("--runs must be positive")
    modules = {path.name for path in (ROOT / "libraries").iterdir() if path.is_dir()}
    covered = {library(case) for case in CASES}
    if modules != covered:
        parser.error(f"benchmark inventory mismatch: {', '.join(sorted(modules ^ covered))}")
    compilers = [rzc.resolve() for rzc in (args.baseline, args.rzc) if rzc is not None]
    flags = ["-march=znver3"] if args.zen3 else ["-O"]
    print(f"{HOST} {flags[0]} | median ms, {args.runs} measured runs", flush=True)
    with tempfile.TemporaryDirectory(prefix="razum-libraries-") as directory:
        work = Path(directory).resolve()
        for case in CASES:
            if args.only and args.only != library(case):
                continue
            if case.host and case.host != HOST:
                print(f"{case.name:<17} {case.host} only", flush=True)
                continue
            programs = [
                build(rzc, case, work / f"{library(case)}-{i}{EXE}", flags)
                for i, rzc in enumerate(compilers)
            ]
            try:
                medians = measure(case, programs, args.runs)
            except subprocess.CalledProcessError as error:
                raise SystemExit(f"{case.name} failed with exit code {error.returncode}: {error.stderr!r}") from error
            times = " -> ".join(f"{ms:.3f}" for ms in medians)
            print(f"{case.name:<17} {times}", flush=True)


if __name__ == "__main__":
    main()
