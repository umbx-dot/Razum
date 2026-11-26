import argparse
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]


def run(args, cwd, env=None):
    started = time.perf_counter_ns()
    process = subprocess.run(args, cwd=cwd, env=env, capture_output=True, text=True, timeout=180)
    if process.returncode != 0:
        raise RuntimeError(process.stdout + process.stderr)
    return (time.perf_counter_ns() - started) / 1e6, process.stdout


def metrics(output):
    return (
        float(re.search(r"Dependencies: ([\d.]+) ms", output)[1]),
        sum(float(value) for value in re.findall(r"Compiling .*?\n  ([\d.]+) ms", output)),
    )


def make_csv(remote):
    shutil.copytree(ROOT / "examples/packages/csv", remote)
    run(["git", "init", "-q", "--template="], remote)
    run(["git", "add", "."], remote)
    run(["git", "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid",
         "commit", "-qm", "CSV fixture"], remote)
    run(["git", "tag", "v0.1.0"], remote)


def measure(driver, directory, count):
    remote = directory / "csv"
    make_csv(remote)
    cold, fetch, compile_, warm = [], [], [], []
    for index in range(count):
        project = directory / f"project-{index}"
        shutil.copytree(ROOT / "examples/project_build", project, ignore=shutil.ignore_patterns("build"))
        manifest = project / "razum.toml"
        with manifest.open("a", encoding="utf-8") as file:
            file.write(f'\n[dependencies]\ncsv = {{ git = "{remote.as_uri()}", tag = "v0.1.0" }}\n')
        entry = project / "src/report/main.rz"
        source = entry.read_text(encoding="utf-8")
        entry.write_text("import csv;\n" + source.replace(
            '    print(report_message());',
            '    int cursor = 0;\n    char value[8];\n'
            '    if (csv_field("csv", &cursor, value) != 0) { return 1; }\n'
            '    print(report_message());'), encoding="utf-8")
        env = {**os.environ, "RZ_CACHE": str(directory / f"cache-{index}")}
        elapsed, output = run([driver, "build", "--profile", "release"], project, env)
        details = metrics(output)
        cold.append(elapsed)
        fetch.append(details[0])
        compile_.append(details[1])
        elapsed, output = run([driver, "build", "--profile", "release", "--offline"], project, env)
        assert "Up to date" in output
        warm.append(elapsed)
    for label, samples in zip(("Cold total", "Cold dependencies", "Cold Razum compile", "Warm total"),
                              (cold, fetch, compile_, warm)):
        print(f"{label:<23} {statistics.median(samples):.2f} ms")


def measure_native(driver, directory):
    project = directory / "native"
    shutil.copytree(ROOT / "examples/mixed_static", project, ignore=shutil.ignore_patterns("build"))
    command = [driver, "build", "--profile", "release", "--target", "integration"]
    cold, cold_output = run(command, project)
    platform = "windows-x64" if os.name == "nt" else "linux-x64"
    artifact = "integration.exe" if os.name == "nt" else "integration"
    log = project / "build" / ".cmake" / platform / "release/integration/.ninja_log"
    rows = [line.split("\t") for line in log.read_text().splitlines()[1:]]
    link_ms = [int(end) - int(start) for start, end, _, out, _ in rows if out == artifact][-1]
    warm, warm_output = run([*command, "--offline"], project)
    print(f"Native cold total       {cold:.2f} ms")
    print(f"Native Razum compile    {metrics(cold_output)[1]:.2f} ms")
    print(f"Native link             {link_ms:.2f} ms")
    print(f"Native warm total       {warm:.2f} ms")
    print("Native warm link        " + ("reused" if "no work to do" in warm_output else "rebuilt"))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("driver", type=Path)
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--native", action="store_true")
    args = parser.parse_args()
    if args.runs < 1:
        parser.error("use a positive number of runs")
    with tempfile.TemporaryDirectory(prefix="razum-project-bench-") as directory:
        temp = Path(directory)
        measure(args.driver.resolve(), temp, args.runs)
        if args.native:
            measure_native(args.driver.resolve(), temp)


if __name__ == "__main__":
    main()
