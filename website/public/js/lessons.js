function finish(lesson) {
    for (const key of ["body", "starter", "task", "hint", "solution", "input"]) {
        if (Array.isArray(lesson[key])) {
            lesson[key] = lesson[key].join("\n").replace(/^\n+|\n+$/g, "");
        }
    }
    for (const test of lesson.tests) {
        test.input = test.input.some((line) => line !== "") ? test.input.join("\n").replace(/^\n+|\n+$/g, "") + "\n" : "";
        test.expected = test.expected.join("\n").replace(/^\n+|\n+$/g, "");
    }
    return lesson;
}

export function parseLessons(text) {
    const lessons = [];
    let lesson = null;
    let section = null;
    for (const line of text.replace(/\r/g, "").split("\n")) {
        if (line.startsWith("=== lesson ")) {
            const [id, chapter, title] = line.slice("=== lesson ".length).split("|").map((part) => part.trim());
            lesson = { id, chapter, title, body: [], tests: [], requires: [] };
            lessons.push(lesson);
            section = lesson.body;
            continue;
        }
        if (line.startsWith("--- ") && lesson) {
            const name = line.slice(4).trim();
            if (name.startsWith("requires ")) {
                const [pattern, message] = name.slice("requires ".length).split(" :: ");
                lesson.requires.push({ pattern: new RegExp(pattern), message });
                section = null;
            } else if (name === "test") {
                lesson.tests.push({ input: [], expected: [] });
                section = lesson.tests[lesson.tests.length - 1].input;
            } else if (name === "expect") {
                section = lesson.tests[lesson.tests.length - 1].expected;
            } else {
                lesson[name] = [];
                section = lesson[name];
            }
            continue;
        }
        if (section) {
            section.push(line);
        }
    }
    return lessons.map(finish);
}

export async function loadLessons() {
    const response = await fetch("lessons.txt", { cache: "no-store" });
    if (!response.ok) {
        throw new Error("Lessons are unavailable");
    }
    const lessons = parseLessons(await response.text());
    if (lessons.length === 0) {
        throw new Error("Lessons are unavailable");
    }
    const chapters = [];
    for (const lesson of lessons) {
        let chapter = chapters.find((item) => item.title === lesson.chapter);
        if (!chapter) {
            chapter = { title: lesson.chapter, lessons: [] };
            chapters.push(chapter);
        }
        chapter.lessons.push(lesson);
        lesson.number = lessons.indexOf(lesson) + 1;
    }
    return { lessons, chapters };
}
