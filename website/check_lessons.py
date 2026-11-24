import json
import re
import sys
import urllib.request
from pathlib import Path

LESSONS = Path(__file__).parent / "public" / "lessons.txt"
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8765


def parse(text):
    lessons = []
    lesson = None
    section = None
    for line in text.replace("\r", "").split("\n"):
        if line.startswith("=== lesson "):
            identifier, chapter, title = [part.strip() for part in line[len("=== lesson "):].split("|")]
            lesson = {"id": identifier, "chapter": chapter, "title": title, "body": [], "tests": [], "requires": []}
            lessons.append(lesson)
            section = lesson["body"]
            continue
        if line.startswith("--- "):
            name = line[4:].strip()
            if name.startswith("requires "):
                pattern, message = name[len("requires "):].split(" :: ", 1)
                lesson["requires"].append((pattern, message))
                section = None
            elif name == "test":
                lesson["tests"].append({"input": [], "expected": []})
                section = lesson["tests"][-1]["input"]
            elif name == "expect":
                section = lesson["tests"][-1]["expected"]
            else:
                lesson[name] = []
                section = lesson[name]
            continue
        if section is not None:
            section.append(line)
    for lesson in lessons:
        for key in ["body", "starter", "task", "hint", "solution", "input"]:
            if key in lesson:
                lesson[key] = "\n".join(lesson[key]).strip("\n")
        for test in lesson["tests"]:
            test["input"] = "\n".join(test["input"]).strip("\n") + "\n" if any(test["input"]) else ""
            test["expected"] = "\n".join(test["expected"]).strip("\n")
    return lessons


def run(code, given):
    body = json.dumps({"code": code, "input": given}).encode("utf-8")
    request = urllib.request.Request(f"http://127.0.0.1:{PORT}/api/run", data=body, method="POST",
                                     headers={"Content-Type": "application/json", "Host": f"localhost:{PORT}"})
    with urllib.request.urlopen(request) as response:
        return json.loads(response.read().decode("utf-8"))


def normalize(text):
    lines = [line.rstrip() for line in text.replace("\r", "").split("\n")]
    return "\n".join(lines).rstrip("\n")


def main():
    lessons = parse(LESSONS.read_text(encoding="utf-8"))
    failures = 0
    for lesson in lessons:
        starter = run(lesson["starter"], lesson.get("input", "") + "\n")
        starter_ok = starter["compile"]["exit_code"] == 0
        expected_starter = lesson["id"] != "errors"
        if starter_ok != expected_starter:
            failures += 1
            print(f"FAIL {lesson['id']}: starter compiles={starter_ok}: {starter['compile']['output'][:200]}")
        if "solution" not in lesson:
            print(f"ok   {lesson['id']} (no task)")
            continue
        for pattern, message in lesson["requires"]:
            if not re.search(pattern, lesson["solution"]):
                failures += 1
                print(f"FAIL {lesson['id']}: solution misses requirement {message}")
        for number, test in enumerate(lesson["tests"], 1):
            result = run(lesson["solution"], test["input"])
            if result["compile"]["exit_code"] != 0:
                failures += 1
                print(f"FAIL {lesson['id']} test {number}: {result['compile']['output'][:300]}")
                continue
            got = normalize(result["run"]["output"])
            if got != test["expected"]:
                failures += 1
                print(f"FAIL {lesson['id']} test {number}: expected {test['expected']!r}, got {got!r}")
        print(f"done {lesson['id']}: {len(lesson['tests'])} test(s)")
    print(f"{len(lessons)} lessons, {failures} failure(s)")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
