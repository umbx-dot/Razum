import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(sys.argv[1]).resolve()
EXPECTED = "C=10 C++=12 Rust=15 Go=19 Razum=24 NASM=30 Lua=37"
WINDOWS = os.name == "nt"
PLATFORM = "windows-x64" if WINDOWS else "linux-x64"
SUFFIX = ".exe" if WINDOWS else ""


def run(*args, cwd, env=None):
    run = subprocess.run(args, cwd=cwd, env=env, text=True, capture_output=True, timeout=180)
    assert run.returncode == 0, run.stdout + run.stderr
    return run.stdout


with tempfile.TemporaryDirectory(prefix="razum-static-") as temporary:
    root = Path(temporary)
    project = root / "project"
    shutil.copytree(ROOT / "examples/mixed_static", project, ignore=shutil.ignore_patterns("build"))
    run(DRIVER, "build", "--profile", "release", "--target", "integration", cwd=project)
    warm = run(DRIVER, "build", "--profile", "release", "--target", "integration", "--offline", cwd=project)
    assert "Building with CMake" in warm and "no work to do" in warm, warm
    empty = root / "only executable"
    empty.mkdir()
    exe = empty / f"integration{SUFFIX}"
    shutil.copy2(project / "build" / PLATFORM / "release" / exe.name, exe)
    if WINDOWS:
        imports = run("objdump", "-p", exe, cwd=empty)
        names = [name.lower() for name in re.findall(r"DLL Name:\s*(\S+)", imports)]
        assert names and all(name in {"kernel32.dll", "ntdll.dll", "ucrtbase.dll", "msvcrt.dll", "userenv.dll",
                                      "ws2_32.dll", "dbghelp.dll", "winmm.dll", "advapi32.dll"} or
                             name.startswith(("api-ms-win-crt-", "api-ms-win-core-")) for name in names), names
        env = {**os.environ, "PATH": str(Path(os.environ["SystemRoot"]) / "System32")}
        linkage = f"{len(names)} Windows system DLL imports"
    else:
        headers = run("readelf", "-l", "-d", exe, cwd=empty)
        assert "INTERP" not in headers and "NEEDED" not in headers, headers
        env = {"PATH": ""}
        linkage = "fully static ELF"
    assert run(exe, cwd=empty, env=env).strip() == EXPECTED
    assert list(empty.iterdir()) == [exe]
    print(EXPECTED)
    print(f"{exe.stat().st_size:,} bytes, {linkage}")
