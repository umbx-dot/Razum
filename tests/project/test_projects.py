import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
DRIVER = Path(sys.argv[1]).resolve()
WINDOWS = os.name == "nt"
SUFFIX = ".exe" if WINDOWS else ""
PLATFORM = "windows-x64" if WINDOWS else "linux-x64"


def run(*args, cwd, env=None, error=None):
    process = subprocess.run(args, cwd=cwd, env=env, text=True, capture_output=True, timeout=40)
    output = process.stdout + process.stderr
    if error is None:
        assert process.returncode == 0, (args, output)
    else:
        assert process.returncode != 0 and error in output, (args, output)
    return process.stdout


def project_builds(temp):
    project = temp / "project with spaces"
    shutil.copytree(ROOT / "examples/project_build", project, ignore=shutil.ignore_patterns("build"))
    release = project / "build" / PLATFORM / "release"
    dev = project / "build" / PLATFORM / "dev"
    output = run(DRIVER, "build", "--offline", cwd=project / "src")
    assert "Compiling report" in output
    assert run(dev / f"report{SUFFIX}", cwd=dev).strip() == "manifest build works"
    assert (dev / "report.asm").exists()
    assert "Up to date" in run(DRIVER, "build", "--offline", cwd=project)
    run(DRIVER, "build", "--profile", "release", "--target", "report", cwd=project)
    assert not (release / "report.asm").exists()
    assert run(DRIVER, "compiler-args", "--profile", "release", "--target", "report", cwd=project).splitlines()[0] == "-O"
    source = project / "src/shared/report_text.rz"
    saved = source.read_text(encoding="utf-8")
    source.write_text(saved.replace("manifest build works", "rebuilt"), encoding="utf-8")
    assert "Compiling report" in run(DRIVER, "build", "--offline", cwd=project)
    assert run(dev / f"report{SUFFIX}", cwd=dev).strip() == "rebuilt"
    prior = (dev / f"report{SUFFIX}").read_bytes()
    source.write_text("int", encoding="utf-8")
    run(DRIVER, "build", cwd=project, error="error")
    assert (dev / f"report{SUFFIX}").read_bytes() == prior
    assert not list(dev.glob(".build-*-*"))
    source.write_text(saved, encoding="utf-8")
    if WINDOWS:
        assert run(dev / "plugin-app.exe", cwd=dev).strip() == "42"
        assert (dev / "plugin.h").is_file() and (dev / "plugin.def").is_file()
    else:
        library = ctypes.CDLL(str(dev / "plugin.so"))
        library.plugin_value.argtypes = [ctypes.c_int32]
        library.plugin_value.restype = ctypes.c_int32
        assert library.plugin_value(-20) == -19 and (dev / "plugin.h").is_file()
        ctypes.c_int32.in_dll(library, "plugin_increment").value = 7
        assert library.plugin_value(-20) == -13
        run(DRIVER, "build", "--target", "plugin-app", cwd=project, error="unavailable")
    manifest = project / "razum.toml"
    original = manifest.read_text(encoding="utf-8")
    text = original.replace('import_dirs = ["src/shared"]', 'import_dirs = ["src/shared"]\ndepends_on = ["report"]')
    manifest.write_text(text, encoding="utf-8")
    run(DRIVER, "build", cwd=project, error="cycle")
    manifest.write_text(original.replace('import_dirs =', 'improt_dirs ='), encoding="utf-8")
    run(DRIVER, "build", cwd=project, error="unknown setting")
    print("ok   project targets, profile arguments, rebuilds and prior artifacts")


def git(folder, *args):
    return run("git", *args, cwd=folder)


def package_builds(temp):
    template = ROOT / "examples/packages/csv"
    remote = temp / "csv-publisher"
    shutil.copytree(template, remote)
    git(remote, "init", "-q", "-b", "main", "--template=")
    git(remote, "add", ".")
    git(remote, "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid", "commit", "-qm", "Publish CSV")
    git(remote, "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid",
        "tag", "-a", "v0.1.0", "-m", "Version 0.1.0")
    revision = git(remote, "rev-parse", "HEAD").strip()
    bare = temp / "published.git"
    git(temp, "clone", "-q", "--bare", str(remote), str(bare))
    project = temp / "consumer"
    project.mkdir()
    (project / "shared").mkdir()
    (project / "razum.toml").write_text(
        'manifest_version = 1\n[project]\nname = "consumer"\n'
        '[[target]]\nname = "check-csv"\nkind = "exe"\nsource = "main.rz"\n'
        'import_dirs = ["shared"]\n', encoding="utf-8")
    (project / "main.rz").write_text(
        'import csv;\nimport stdio;\n'
        'int main() {\n'
        '    int cursor = 0;\n'
        '    char value[24];\n'
        '    if (csv_field("one,\\\"two,2\\\",\\\"a\\\"\\\"b\\\",", &cursor, value) != 1) { return 1; }\n'
        '    print(value);\n'
        '    if (csv_field("one,\\\"two,2\\\",\\\"a\\\"\\\"b\\\",", &cursor, value) != 1) { return 2; }\n'
        '    print(value);\n'
        '    if (csv_field("one,\\\"two,2\\\",\\\"a\\\"\\\"b\\\",", &cursor, value) != 1) { return 3; }\n'
        '    print(value);\n'
        '    return csv_field("one,\\\"two,2\\\",\\\"a\\\"\\\"b\\\",", &cursor, value);\n'
        '}\n', encoding="utf-8")
    env = {**os.environ, "RZ_CACHE": str(temp / "packages"), "GIT_TERMINAL_PROMPT": "0",
           "GIT_CONFIG_COUNT": "1", "GIT_CONFIG_KEY_0": f"url.{bare.as_uri()}.insteadOf",
           "GIT_CONFIG_VALUE_0": "https://github.com/test-owner/razum-csv"}
    run(DRIVER, "add", "test-owner/razum-csv@v0.1.0", cwd=project, env=env)
    lock = project / "razum.lock"
    assert revision in lock.read_text(encoding="utf-8")
    (remote / "csv.rz").write_text("int broken;\n", encoding="utf-8")
    git(remote, "add", "csv.rz")
    git(remote, "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid", "commit", "-qm", "New revision")
    git(bare, "fetch", "-q", str(remote), "HEAD")
    git(bare, "tag", "-f", "v0.1.0", "FETCH_HEAD")
    replacement = git(remote, "rev-parse", "HEAD").strip()
    git(bare, "tag", "v0.2.0", replacement)
    git(bare, "tag", "v0.10.0", revision)
    git(bare, "tag", "v999.0.0-rc.1", replacement)

    run(DRIVER, "update", "csv@v0.1.0", cwd=project, env=env)
    assert revision in lock.read_text(encoding="utf-8")
    package_updates(temp, env, revision, replacement)

    fresh = {**env, "RZ_CACHE": str(temp / "fresh packages")}
    run(DRIVER, "fetch", "--locked", cwd=project, env=fresh)
    assert revision in lock.read_text(encoding="utf-8")
    bare.rename(temp / "offline.git")
    run(DRIVER, "build", "--offline", cwd=project, env=fresh)
    flags = run(DRIVER, "compiler-args", "--target", "check-csv", "--offline", cwd=project, env=fresh).splitlines()
    assert flags[::2] == ["-I", "-I"] and Path(flags[1]) == project / "shared"
    executable = project / "build" / PLATFORM / "dev" / f"check-csv{SUFFIX}"
    assert run(executable, cwd=project).splitlines() == ["one", "two,2", 'a"b']
    run(DRIVER, "build", "--offline", cwd=project, env={**env, "RZ_CACHE": str(temp / "empty")},
        error="missing from cache")
    print("ok   published CSV tag, commit pin, locked refetch and offline build")


def package_updates(temp, env, revision, replacement):
    project = temp / "updates"
    project.mkdir()
    manifest = project / "razum.toml"
    lock = project / "razum.lock"
    header = b'\xef\xbb\xbfdependencies = {} # keep this\r\nmanifest_version = 1\r\n[project]\r\nname = "updates"\r\n'
    manifest.write_bytes(header)
    assert "Added csv@v0.10.0" in run(DRIVER, "add", "test-owner/razum-csv", cwd=project, env=env)
    assert revision in lock.read_text(encoding="utf-8")
    run(DRIVER, "add", "test-owner/razum-csv", cwd=project, env=env, error="use rz update csv")
    literal = b"'''\r\nv0.10.0'''"
    source = manifest.read_bytes().replace(b'"v0.10.0"', literal)
    manifest.write_bytes(source)
    assert header[header.index(b'} #'):] in source
    assert "v0.10.0 -> v0.2.0" in run(DRIVER, "update", "csv@v0.2.0", cwd=project, env=env)
    assert replacement in lock.read_text(encoding="utf-8")
    assert manifest.read_bytes() == source.replace(literal, b'"v0.2.0"')
    assert "v0.2.0 -> v0.10.0" in run(DRIVER, "update", "csv", cwd=project, env=env)
    assert revision in lock.read_text(encoding="utf-8")
    original = (manifest.read_bytes(), lock.read_bytes())
    assert "pinned to v0.10.0" in run(DRIVER, "update", "csv", cwd=project, env=env)
    run(DRIVER, "update", "csv@v99.0.0", cwd=project, env=env, error="exited with status")
    assert (manifest.read_bytes(), lock.read_bytes()) == original
    print("ok   latest stable tags, exact upgrades, inline manifests and failed update rollback")


with tempfile.TemporaryDirectory(prefix="razum-project-tests-") as folder:
    project_builds(Path(folder))
    package_builds(Path(folder))
