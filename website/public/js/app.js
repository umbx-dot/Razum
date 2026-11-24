import { Editor } from "./editor.js";
import { renderMarkdown } from "./markdown.js";
import { runCode, checkCode, serverOnline, parseErrors, normalizeOutput } from "./api.js";
import { store } from "./store.js";
import { loadLessons } from "./lessons.js";
import { renderDocs } from "./docs.js";
import { renderBenchmarks } from "./benchmarks.js";
import { escapeHtml, highlight } from "./highlight.js";

let app = document.getElementById("app");
const PLAYGROUND_START = `import stdio;

int main() {
    print("Hello from the playground!");
    return 0;
}
`;
let course = null;
let courseRequest = null;
let activePage = "home";
let online = true;

function $(selector, root = document) {
    return root.querySelector(selector);
}

function toast(message, kind = "info") {
    const element = document.createElement("div");
    element.className = `toast toast-${kind}`;
    element.textContent = message;
    $("#toasts").appendChild(element);
    requestAnimationFrame(() => element.classList.add("visible"));
    setTimeout(() => {
        element.classList.remove("visible");
        setTimeout(() => element.remove(), 400);
    }, 2600);
}

function confirmAction(message) {
    return window.confirm(message);
}

function celebrate(origin) {
    const box = origin.getBoundingClientRect();
    const layer = document.createElement("div");
    layer.className = "celebration";
    layer.style.left = `${box.left + box.width / 2}px`;
    layer.style.top = `${box.top + box.height / 2}px`;
    for (let index = 0; index < 28; index++) {
        const piece = document.createElement("span");
        const angle = (index / 28) * Math.PI * 2;
        const distance = 70 + Math.random() * 90;
        piece.style.setProperty("--x", `${Math.cos(angle) * distance}px`);
        piece.style.setProperty("--y", `${Math.sin(angle) * distance}px`);
        piece.style.setProperty("--r", `${Math.random() * 360}deg`);
        piece.style.animationDelay = `${Math.random() * 80}ms`;
        layer.appendChild(piece);
    }
    document.body.appendChild(layer);
    setTimeout(() => layer.remove(), 1300);
}

function applyTheme() {
    const theme = store.state.theme;
    const dark = theme === "dark" || (theme === "auto" && window.matchMedia("(prefers-color-scheme: dark)").matches);
    document.documentElement.dataset.theme = dark ? "dark" : "light";
    $("#theme-toggle").title = `Theme: ${theme}`;
    $("#theme-toggle").textContent = theme === "auto" ? "◐" : dark ? "☾" : "☀";
}

function updateProgress() {
    const done = course.lessons.filter((lesson) => store.isDone(lesson.id)).length;
    const total = course.lessons.length;
    $("#progress-fill").style.width = `${(done / total) * 100}%`;
    $("#progress-text").textContent = `${done} / ${total}`;
}

function setActiveNav(name) {
    const titles = { home: "Razum programming language", learn: "Learn · Razum", playground: "Playground · Razum", benchmarks: "Benchmarks · Razum" };
    document.title = titles[name] || "Razum";
    activePage = name;
    $("#course-progress").hidden = name !== "learn" || course === null;
    $("#offline").hidden = online || (name !== "learn" && name !== "playground");
    for (const link of document.querySelectorAll("[data-nav]")) {
        const active = link.dataset.nav === name;
        link.classList.toggle("active", active);
        if (active) {
            link.setAttribute("aria-current", "page");
        } else {
            link.removeAttribute("aria-current");
        }
    }
}

function nextUnfinished() {
    return course.lessons.find((lesson) => !store.isDone(lesson.id)) || course.lessons[course.lessons.length - 1];
}

function statusIcon(lesson) {
    if (store.isDone(lesson.id)) {
        return '<span class="status done" title="Done">✓</span>';
    }
    if (store.isSkipped(lesson.id)) {
        return '<span class="status skipped" title="Skipped">↷</span>';
    }
    return '<span class="status todo"></span>';
}

function show(html, className) {
    app.className = `view ${className}`;
    app.innerHTML = html;
    app.classList.remove("view-enter");
    void app.offsetWidth;
    app.classList.add("view-enter");
    window.scrollTo({ top: 0 });
}

function renderHome() {
    setActiveNav("home");
    show(`
        <section class="hero">
            <div class="hero-glow" aria-hidden="true"></div>
            <div class="hero-text">
                <h1><span class="accent">Razum</span><br>A small language for native programs.</h1>
                <p class="lead">Write C-like code and compile it to native x86-64 executables for Windows or Linux. Razum ships with standard libraries and emits readable assembly from your source.</p>
                <div class="hero-actions">
                    <a class="button primary large" href="#/playground">Try Razum</a>
                    <a class="button large" href="#/docs">Read the docs</a>
                </div>
            </div>
            <pre class="hero-code code-block rz"><code>${highlight(`import stdio;

int main() {
    for (int i = 1; i <= 4; i++) {
        print(i * i);
    }
    return 0;
}`)}</code></pre>
        </section>
        <section aria-labelledby="explore-razum">
            <h2 id="explore-razum">Explore Razum</h2>
            <div class="features">
                <a class="feature" href="#/docs"><h3>Docs</h3><p>Look up language syntax, types, and the standard library.</p></a>
                <a class="feature" href="#/playground"><h3>Playground</h3><p>Write and run a native Razum program in your browser.</p></a>
                <a class="feature" href="#/benchmarks"><h3>Benchmarks</h3><p>Explore measured compiler and library performance.</p></a>
                <a class="feature" href="#/learn"><h3>Learn</h3><p>Build programs step by step with examples and checked exercises.</p></a>
            </div>
        </section>
        <section class="terminal-quickstart" aria-labelledby="terminal-title">
            <div>
                <h2 id="terminal-title">Build from a terminal</h2>
                <p>Save the example as <code>hello.rz</code>. With <code>rzc</code> on your PATH, compile and run it with this command.</p>
            </div>
            <pre class="code-block"><code>rzc hello.rz --run</code></pre>
        </section>`, "home-view");
}

function renderCourse() {
    setActiveNav("learn");
    const last = course.lessons.find((item) => item.id === store.state.last);
    const resume = last && !store.isDone(last.id) ? last : nextUnfinished();
    const chapters = course.chapters.map((chapter) => {
        const done = chapter.lessons.filter((lesson) => store.isDone(lesson.id)).length;
        return `<a class="chapter-card" href="#/learn/${chapter.lessons[0].id}">
            <div class="chapter-head"><h3>${escapeHtml(chapter.title)}</h3><span>${done} / ${chapter.lessons.length}</span></div>
            <div class="meter"><span style="width:${(done / chapter.lessons.length) * 100}%"></span></div>
            <ol>${chapter.lessons.map((lesson) => `<li>${statusIcon(lesson)}${escapeHtml(lesson.title)}</li>`).join("")}</ol>
        </a>`;
    }).join("");
    show(`
        <header class="course-intro">
            <h1>Learn Razum</h1>
            <p class="lead">Read a lesson, run the examples, and check your work in the editor. Your code and progress are saved in this browser.</p>
            <a class="button primary" href="#/learn/${resume.id}">${last ? `Continue with ${escapeHtml(resume.title)}` : "Start the course"}</a>
        </header>
        <section class="chapters">${chapters}</section>
        <section class="home-footer">
            <button class="button subtle" id="reset-all">Reset all progress</button>
        </section>`, "home-view");
    $("#reset-all").addEventListener("click", () => {
        if (confirmAction("Reset all progress and saved lesson code? Playground snippets are kept.")) {
            store.resetAll();
            updateProgress();
            renderCourse();
            toast("Progress reset");
        }
    });
}

function outputHtml(result) {
    if (result.error) {
        return `<div class="out-status error">${escapeHtml(result.error)}</div>`;
    }
    const compile = result.compile;
    if (compile.exit_code !== 0 || !result.run) {
        const errors = parseErrors(compile.output);
        return `<div class="out-status error">${errors.length} error${errors.length === 1 ? "" : "s"}${compile.timed_out ? " (the compiler took too long)" : ""}</div>
            <ul class="error-list">${errors.map((error) => `<li data-line="${error.line}" data-column="${error.column}">
                ${error.line ? `<span class="where">line ${error.line}</span>` : ""}${escapeHtml(error.message)}</li>`).join("")}</ul>`;
    }
    const run = result.run;
    const notes = [];
    if (run.timed_out) {
        notes.push("stopped after 5 seconds");
    }
    if (run.truncated) {
        notes.push("output cut at 64 KB");
    }
    return `<div class="out-status ${run.exit_code === 0 && !run.timed_out ? "ok" : "warn"}">
            exit code ${run.exit_code} · compiled in ${compile.milliseconds} ms · ran in ${run.milliseconds} ms${notes.length ? ` · ${notes.join(", ")}` : ""}
        </div>
        <pre class="out-text">${escapeHtml(run.output.replace(/\r/g, "")) || '<span class="muted">(no output)</span>'}</pre>`;
}

function errorsFromResult(result) {
    if (!result || result.error || !result.compile || result.compile.exit_code === 0) {
        return [];
    }
    return parseErrors(result.compile.output).filter((error) => error.line > 0 && /main\.rz$/.test(error.file));
}

function createWorkspace(host, options) {
    host.innerHTML = `
        <div class="workspace-bar">
            <span class="file-name">${escapeHtml(options.fileName || "main.rz")}</span>
            <span class="live-status" aria-live="polite"></span>
            <span class="spacer"></span>
            ${options.extraButtons || ""}
            <button class="button primary" data-action="run" title="Run (Ctrl+Enter)" aria-label="Run (Ctrl+Enter)"><span class="run-icon">▶</span> Run</button>
        </div>
        <div class="editor-host"></div>
        <details class="input-box"${options.input ? " open" : ""}>
            <summary>Program input</summary>
            <textarea spellcheck="false" placeholder="What read_int and read_input read, one value per line"></textarea>
        </details>
        <div class="output" aria-live="polite"><div class="out-status muted">Press Run or Ctrl+Enter.</div></div>`;
    const inputBox = $(".input-box textarea", host);
    inputBox.value = options.input || "";
    inputBox.addEventListener("input", () => options.onInput && options.onInput(inputBox.value));
    const output = $(".output", host);
    const liveStatus = $(".live-status", host);
    const runButton = $('[data-action="run"]', host);
    let checkTimer = null;
    let checkVersion = 0;
    const workspace = {
        editor: null,
        output,
        get input() {
            return inputBox.value;
        },
        async run() {
            if (!online) {
                toast("The razum server is not running", "error");
                return null;
            }
            runButton.disabled = true;
            runButton.classList.add("busy");
            output.classList.add("loading");
            let result;
            try {
                result = await runCode(workspace.editor.value, inputBox.value);
            } catch {
                result = { error: "Cannot reach the razum server." };
            }
            runButton.disabled = false;
            runButton.classList.remove("busy");
            output.classList.remove("loading");
            output.innerHTML = outputHtml(result);
            workspace.editor.setDiagnostics(errorsFromResult(result));
            for (const item of output.querySelectorAll(".error-list li")) {
                item.addEventListener("click", () => workspace.editor.revealLine(Number(item.dataset.line), Number(item.dataset.column)));
            }
            output.classList.remove("flash");
            void output.offsetWidth;
            output.classList.add("flash");
            return result;
        },
        scheduleCheck() {
            clearTimeout(checkTimer);
            liveStatus.textContent = "";
            liveStatus.className = "live-status";
            checkTimer = setTimeout(async () => {
                if (!online) {
                    return;
                }
                const version = ++checkVersion;
                const code = workspace.editor.value;
                try {
                    const result = await checkCode(code);
                    if (version !== checkVersion || code !== workspace.editor.value || !result.check) {
                        return;
                    }
                    const errors = parseErrors(result.check.output).filter((error) => error.line > 0 && /main\.rz$/.test(error.file));
                    workspace.editor.setDiagnostics(errors);
                    liveStatus.textContent = errors.length ? `${errors.length} error${errors.length === 1 ? "" : "s"}` : "no errors";
                    liveStatus.className = `live-status ${errors.length ? "bad" : "good"}`;
                } catch {
                    return;
                }
            }, 700);
        },
    };
    workspace.editor = new Editor($(".editor-host", host), {
        value: options.code,
        onRun: () => workspace.run(),
        onSave: () => options.onSave && options.onSave(),
        onChange: (text) => {
            if (options.onCode) {
                options.onCode(text);
            }
            workspace.scheduleCheck();
        },
    });
    runButton.addEventListener("click", () => workspace.run());
    workspace.scheduleCheck();
    return workspace;
}

function sidebarHtml(active) {
    return course.chapters.map((chapter) => `
        <div class="side-chapter">
            <h4>${escapeHtml(chapter.title)}</h4>
            ${chapter.lessons.map((lesson) => `<a class="side-lesson${lesson.id === active.id ? " active" : ""}" href="#/learn/${lesson.id}">
                ${statusIcon(lesson)}<span class="side-number">${lesson.number}</span><span>${escapeHtml(lesson.title)}</span></a>`).join("")}
        </div>`).join("");
}

function testReport(lesson, failure) {
    return `<div class="verdict fail">
        <strong>Not yet.</strong> ${failure.message ? escapeHtml(failure.message) : ""}
        ${failure.test ? `
            ${failure.test.input ? `<div class="compare-input"><span>Input</span><pre>${escapeHtml(failure.test.input)}</pre></div>` : ""}
            <div class="compare">
                <div><span>Expected</span><pre>${escapeHtml(failure.test.expected)}</pre></div>
                <div><span>Your program</span><pre>${escapeHtml(failure.got)}</pre></div>
            </div>` : ""}
    </div>`;
}

async function checkTask(lesson, workspace, verdict, button) {
    const code = workspace.editor.value;
    for (const requirement of lesson.requires) {
        if (!requirement.pattern.test(code)) {
            verdict.innerHTML = testReport(lesson, { message: requirement.message });
            return;
        }
    }
    button.disabled = true;
    button.classList.add("busy");
    verdict.innerHTML = '<div class="verdict pending">Checking…</div>';
    let failure = null;
    for (const test of lesson.tests) {
        let result;
        try {
            result = await runCode(code, test.input);
        } catch {
            failure = { message: "Cannot reach the razum server." };
            break;
        }
        if (result.error) {
            failure = { message: result.error };
            break;
        }
        if (result.compile.exit_code !== 0 || !result.run) {
            workspace.output.innerHTML = outputHtml(result);
            workspace.editor.setDiagnostics(errorsFromResult(result));
            failure = { message: "The program does not compile yet; the errors are under the editor." };
            break;
        }
        const got = normalizeOutput(result.run.output);
        if (got !== test.expected) {
            failure = { test, got, message: result.run.timed_out ? "It ran for more than 5 seconds." : "" };
            break;
        }
    }
    button.disabled = false;
    button.classList.remove("busy");
    if (failure) {
        verdict.innerHTML = testReport(lesson, failure);
        return;
    }
    const first = !store.isDone(lesson.id);
    store.complete(lesson.id);
    updateProgress();
    $(".side-nav").innerHTML = sidebarHtml(lesson);
    const next = course.lessons[lesson.number];
    verdict.innerHTML = `<div class="verdict pass"><strong>Solved!</strong> ${lesson.tests.length > 1 ? `All ${lesson.tests.length} checks pass.` : "The output is exactly right."}
        ${next ? `<a class="button primary" href="#/learn/${next.id}">Next: ${escapeHtml(next.title)} →</a>` : '<a class="button primary" href="#/">Back to the start</a>'}</div>`;
    if (first) {
        celebrate(button);
    }
}

function renderLesson(lesson) {
    setActiveNav("learn");
    store.setLast(lesson.id);
    const previous = course.lessons[lesson.number - 2];
    const next = course.lessons[lesson.number];
    const hasTask = Boolean(lesson.task);
    show(`
        <aside class="side-nav" aria-label="Lessons">${sidebarHtml(lesson)}</aside>
        <article class="lesson">
            <p class="crumb">${escapeHtml(lesson.chapter)} · Lesson ${lesson.number} of ${course.lessons.length}</p>
            <h1>${escapeHtml(lesson.title)}</h1>
            <div class="lesson-body">${renderMarkdown(lesson.body)}</div>
            ${hasTask ? `
            <section class="task-card">
                <h2><span class="task-badge">Task</span></h2>
                <div class="task-text">${renderMarkdown(lesson.task)}</div>
                <div class="task-actions">
                    <button class="button primary" data-action="check">Check my solution</button>
                    ${lesson.hint ? '<button class="button" data-action="hint">Hint</button>' : ""}
                    ${lesson.solution ? '<button class="button subtle" data-action="solution">Show solution</button>' : ""}
                </div>
                <div class="hint-box" hidden>${renderMarkdown(lesson.hint || "")}</div>
                <div class="solution-box" hidden>
                    <pre class="code-block rz"><code>${highlight(lesson.solution || "")}</code></pre>
                    <button class="button" data-action="use-solution">Put it in the editor</button>
                </div>
                <div class="task-verdict"></div>
            </section>` : `
            <section class="task-card">
                <div class="task-text"><p>No task here: try anything you like in the editor.</p></div>
                <div class="task-actions"><button class="button primary" data-action="finish">Mark as done</button></div>
            </section>`}
            <nav class="lesson-nav">
                ${previous ? `<a class="button" href="#/learn/${previous.id}">← ${escapeHtml(previous.title)}</a>` : '<span></span>'}
                <span class="spacer"></span>
                <button class="button subtle" data-action="reset">Reset this lesson</button>
                ${next ? `<button class="button subtle" data-action="skip">Skip</button>
                <a class="button" href="#/learn/${next.id}">${escapeHtml(next.title)} →</a>` : ""}
            </nav>
        </article>
        <section class="workspace"></section>`, "lesson-view");
    const workspace = createWorkspace($(".workspace"), {
        code: store.code(lesson.id, lesson.starter || ""),
        input: store.input(lesson.id, lesson.input ? lesson.input + "\n" : ""),
        onCode: (text) => store.setCode(lesson.id, text),
        onInput: (text) => store.setInput(lesson.id, text),
        onSave: () => toast("Saved in this browser"),
    });
    const verdict = $(".task-verdict");
    app.addEventListener("click", (event) => {
        const action = event.target.closest("[data-action]")?.dataset.action;
        if (!action) {
            return;
        }
        if (action === "check") {
            checkTask(lesson, workspace, verdict, event.target.closest("button"));
        } else if (action === "hint") {
            $(".hint-box").hidden = !$(".hint-box").hidden;
        } else if (action === "solution") {
            $(".solution-box").hidden = !$(".solution-box").hidden;
        } else if (action === "use-solution") {
            if (confirmAction("Replace your code with the solution?")) {
                workspace.editor.value = lesson.solution;
                store.setCode(lesson.id, lesson.solution);
                workspace.scheduleCheck();
            }
        } else if (action === "reset") {
            if (confirmAction("Reset this lesson? Your code goes back to the start and the lesson is no longer marked done.")) {
                store.resetLesson(lesson.id);
                updateProgress();
                resetView();
                renderLesson(lesson);
                toast("Lesson reset");
            }
        } else if (action === "skip") {
            store.skip(lesson.id);
            location.hash = `#/learn/${next.id}`;
        } else if (action === "finish") {
            store.complete(lesson.id);
            updateProgress();
            $(".side-nav").innerHTML = sidebarHtml(lesson);
            celebrate(event.target);
            toast("Course complete — well done!", "success");
        }
    });
    $(".side-lesson.active")?.scrollIntoView({ block: "nearest" });
}

function renderPlayground() {
    setActiveNav("playground");
    if (store.snippets().length === 0) {
        store.saveSnippet({ id: `s${Date.now()}`, name: "hello.rz", code: PLAYGROUND_START, input: "" });
    }
    const snippets = store.snippets();
    const active = snippets.find((item) => item.id === store.state.snippet) || snippets[0];
    show(`
        <aside class="snippets">
            <div class="snippets-head"><h3>Your programs</h3><button class="button small" data-action="new">+ New</button></div>
            <ul>${snippets.map((item) => `<li class="${item.id === active.id ? "active" : ""}" data-id="${item.id}">
                <span>${escapeHtml(item.name)}</span><button class="icon-button" data-action="delete" data-id="${item.id}" title="Delete">✕</button></li>`).join("")}</ul>
            <p class="muted small">Saved in this browser.</p>
        </aside>
        <section class="workspace playground"></section>`, "playground-view");
    const workspace = createWorkspace($(".workspace"), {
        fileName: active.name,
        code: active.code,
        input: active.input,
        extraButtons: '<button class="button" data-action="download" title="Download as a .rz file">Download</button>',
        onCode: (text) => store.saveSnippet({ ...active, code: text, input: workspace.input }),
        onInput: (text) => store.saveSnippet({ ...active, input: text, code: workspace.editor.value }),
        onSave: () => toast("Saved in this browser"),
    });
    app.addEventListener("click", (event) => {
        const target = event.target.closest("[data-action], li[data-id]");
        if (!target) {
            return;
        }
        const action = target.dataset.action;
        if (action === "new") {
            const name = (window.prompt("Name of the new program", "program.rz") || "").trim();
            if (name) {
                store.saveSnippet({ id: `s${Date.now()}`, name: name.endsWith(".rz") ? name : `${name}.rz`, code: PLAYGROUND_START, input: "" });
                resetView();
                renderPlayground();
            }
        } else if (action === "delete") {
            event.stopPropagation();
            const item = snippets.find((snippet) => snippet.id === target.dataset.id);
            if (item && confirmAction(`Delete ${item.name}?`)) {
                store.deleteSnippet(item.id);
                resetView();
                renderPlayground();
            }
        } else if (action === "download") {
            const blob = new Blob([workspace.editor.value], { type: "text/plain" });
            const link = document.createElement("a");
            link.href = URL.createObjectURL(blob);
            link.download = active.name;
            link.click();
            URL.revokeObjectURL(link.href);
        } else if (!action && target.dataset.id && target.dataset.id !== active.id) {
            store.selectSnippet(target.dataset.id);
            resetView();
            renderPlayground();
        }
    });
}

async function route() {
    const raw = location.hash.replace(/^#\/?/, "");
    const [path, anchor] = raw.split("#");
    const [page, id, sub] = path.split("/");
    if (page === "learn") {
        if (course === null) {
            const view = app;
            setActiveNav("learn");
            show('<h1>Learn Razum</h1><p role="status">Loading lessons…</p>', "home-view");
            try {
                courseRequest ??= loadLessons();
                course = await courseRequest;
            } catch {
                courseRequest = null;
                if (view.isConnected) {
                    show('<h1>Learn Razum</h1><p role="status">The lessons could not be loaded. Reload to try again.</p>', "home-view");
                }
                return;
            }
            if (!view.isConnected) {
                return;
            }
        }
        updateProgress();
        const lesson = course.lessons.find((item) => item.id === id);
        if (lesson) {
            renderLesson(lesson);
        } else {
            renderCourse();
        }
    } else if (page === "docs") {
        setActiveNav("docs");
        renderDocs(app, id, sub, anchor);
    } else if (page === "benchmarks") {
        setActiveNav("benchmarks");
        renderBenchmarks(app, anchor);
    } else if (page === "playground") {
        renderPlayground();
    } else {
        renderHome();
    }
}

function resetView() {
    const fresh = app.cloneNode(false);
    app.replaceWith(fresh);
    app = fresh;
}

function start() {
    applyTheme();
    window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", applyTheme);
    $("#theme-toggle").addEventListener("click", () => {
        const order = ["auto", "light", "dark"];
        store.setTheme(order[(order.indexOf(store.state.theme) + 1) % order.length]);
        applyTheme();
    });
    serverOnline().then((available) => {
        online = available;
        $("#offline").hidden = online || (activePage !== "learn" && activePage !== "playground");
    });
    window.addEventListener("hashchange", () => {
        resetView();
        route();
    });
    route();
}

start();
