import { renderMarkdown, slugify, tableApi } from "./markdown.js";
import { escapeHtml } from "./highlight.js";

const SOURCES = [
    {
        id: "language",
        title: "Language reference",
        group: "Language",
        file: "language.md",
        blurb: "Syntax and examples for types, operators, control flow, functions, modules, pointers and inline assembly.",
    },
    {
        id: "libraries",
        title: "Library reference",
        group: "Libraries",
        file: "libraries.md",
        blurb: "Imports, API signatures, examples and platform support for the libraries that ship with razum.",
    },
    {
        id: "projects",
        title: "Projects and packages",
        group: "Projects",
        file: "projects.md",
        blurb: "Define build targets, install source packages and link Razum with C, C++, Rust and Go.",
    },
    {
        id: "discord",
        title: "Discord bots",
        group: "Projects",
        file: "discord.md",
        blurb: "Build a Discord bot with slash commands, Gateway events, file uploads and signed interactions.",
    },
];

let loading = null;
let renderVersion = 0;
const TITLE = "Razum";

const SUMMARY = {
    language: {
        "a-first-program": "Write a program, run it, and see how main and print fit together.",
        "program-structure": "The items a file may hold, in any order: imports, structs, enums, constants, globals, functions and the program kind.",
        "names-and-keywords": "What a name may contain, the reserved words, and why assembly words such as add stay free.",
        "types": "Sizes, ranges, defaults and literal forms for every type, from bool to fn(...) -> T.",
        "literals": "Decimal and hexadecimal integers, floating-point, character, boolean and string literals.",
        "conversions": "How values convert, automatically and with casts, between the numeric types and bool.",
        "operators": "Arithmetic, comparisons, bit operations, and which operators are evaluated first.",
        "statements": "Declarations, assignments, calls, control flow, asm blocks and scoped blocks.",
        "control-flow": "if / else, while, for, do / while, switch, break, continue and return.",
        "constants-and-enums": "const values folded into the code, and enums with scoped members and checked switch cases.",
        "making-a-dll": "Build a Windows DLL, run code on load and export functions or shared data.",
        "asm-blocks": "Use inline x86-64 assembly and follow the calling convention.",
    },
    libraries: {
        stdio: "Read from standard input and write strings, numbers and formatted values to standard output.",
        array: "Find, transform, sort and reduce arrays of numbers or text.",
        text: "Compare, search, split, build and convert UTF-8 text.",
        math: "Integer arithmetic, floating-point functions and random numbers.",
        json: "Parse JSON, read values and write JSON into a buffer.",
        time: "Read clocks, work with UTC timestamps and measure elapsed time.",
        filesystem: "Open files, read and write data, manipulate paths and list directories.",
        windows: "Call the Windows API and declare functions from Windows DLLs.",
        winsock: "Call Windows socket APIs for TCP, binary addresses and overlapped I/O.",
        linux: "Call the Linux kernel and read the process environment.",
        gui: "Make windows, draw graphics and use widgets on Windows, Wayland or X11.",
        threads: "Start workers and coordinate them with synchronization and atomic operations.",
        net: "Use buffered TCP streams, wait on multiple sockets, send datagrams and work with raw packets.",
        http: "Serve routes and static files or fetch HTTPS responses with certificate verification.",
        crypt: "Hash, authenticate and encrypt data and work with keys and certificates.",
        process: "Run programs with input, output and resource limits.",
        program: "Receive command-line arguments and manage memory with the bundled program runtime.",
        "assembly-routines": "The hand-written rz@ routines that library razum files call, and how an asm block reaches them.",
    },
};

function stripMarkdown(text) {
    return text
        .replace(/```[\s\S]*?```/g, " ")
        .replace(/`([^`]*)`/g, "$1")
        .replace(/\[([^\]]+)\]\([^)]*\)/g, "$1")
        .replace(/^\s{0,3}#{1,6}\s+/gm, "")
        .replace(/^\s*>\s?/gm, "")
        .replace(/^\s*[-*+]\s+/gm, "")
        .replace(/^\s*\d+\.\s+/gm, "")
        .replace(/[*|]/g, " ")
        .replace(/\s+/g, " ")
        .trim();
}

function firstSentence(text, limit = 150) {
    if (!text) {
        return "";
    }
    const end = text.search(/[.!?](\s|$)/);
    if (end >= 0 && end < limit) {
        return text.slice(0, end + 1);
    }
    if (text.length <= limit) {
        return text;
    }
    const boundary = text.lastIndexOf(" ", limit);
    return text.slice(0, boundary > 0 ? boundary : limit) + "…";
}

function parseSource(text, source) {
    const lines = text.replace(/\r/g, "").split("\n");
    const intro = [];
    const sections = [];
    let section = null;
    let sub = null;
    let fence = false;
    for (const line of lines) {
        if (line.startsWith("```")) {
            fence = !fence;
        }
        if (!fence && line.startsWith("# ")) {
            continue;
        }
        if (!fence && line.startsWith("## ")) {
            const title = line.slice(3).trim();
            section = { title, slug: slugify(title), body: [], subs: [], api: new Map() };
            sections.push(section);
            sub = null;
            continue;
        }
        if (!fence && line.startsWith("### ") && section) {
            const title = line.slice(4).trim();
            sub = { title, slug: slugify(title), body: [] };
            section.subs.push(sub);
        }
        if (section) {
            section.body.push(line);
            if (sub) {
                sub.body.push(line);
            }
            if (!fence && source.id === "libraries") {
                const api = tableApi(line);
                for (const name of api?.names ?? []) {
                    if (!section.api.has(name)) {
                        section.api.set(name, api.anchor);
                    }
                }
            }
        } else {
            intro.push(line);
        }
    }
    source.intro = intro
        .filter((line) => !/^(Contents|Built in):/.test(line))
        .join("\n")
        .trim();
    source.anchors = new Map();
    for (const item of sections) {
        item.plain = stripMarkdown(item.body.join("\n"));
        item.summary = SUMMARY[source.id]?.[item.slug] || firstSentence(item.plain);
        for (const child of item.subs) {
            child.plain = stripMarkdown(child.body.join("\n"));
        }
        for (const child of item.subs) {
            source.anchors.set(child.slug, { section: item.slug, anchor: child.slug });
        }
    }
    for (const item of sections) {
        source.anchors.set(item.slug, { section: item.slug, anchor: null });
    }
    source.sections = sections;
    return source;
}

async function load() {
    if (!loading) {
        loading = Promise.all(SOURCES.map(async (source) => {
            const response = await fetch(`docs/${source.file}`, { cache: "no-store" });
            if (!response.ok) {
                throw new Error(`Cannot load ${source.file} (the server answered ${response.status}).`);
            }
            return parseSource(await response.text(), { ...source });
        })).then((sources) => {
            const index = [];
            for (const source of sources) {
                for (const section of source.sections) {
                    index.push({ source, section, anchor: null, title: section.title, text: section.plain });
                    for (const sub of section.subs) {
                        index.push({ source, section, anchor: sub.slug, title: sub.title, text: sub.plain });
                    }
                    for (const [name, anchor] of section.api) {
                        index.push({ source, section, anchor, title: name, text: section.title });
                    }
                }
            }
            return { sources, byId: new Map(sources.map((source) => [source.id, source])), index };
        }).catch((error) => {
            loading = null;
            throw error;
        });
    }
    return loading;
}

function sectionHref(source, section, anchor) {
    return `#/docs/${source.id}/${section.slug}${anchor ? `#${anchor}` : ""}`;
}

function targetHref(source, target) {
    return `#/docs/${source.id}/${target.section}${target.anchor ? `#${target.anchor}` : ""}`;
}

function linkResolver(source, docs) {
    return (url) => {
        if (url.startsWith("#")) {
            const target = source.anchors.get(url.slice(1));
            return target ? targetHref(source, target) : null;
        }
        if (/^(https?:|mailto:)/.test(url)) {
            return url;
        }
        const [file, anchor] = url.split("#");
        if (file === "tutorial.md") {
            return "#/learn";
        }
        const targetSource = docs.sources.find((item) => item.file === file);
        if (!targetSource) {
            return file === "cmake.md" ? `/docs/${url}` : null;
        }
        if (!anchor) {
            return `#/docs/${targetSource.id}`;
        }
        const target = targetSource.anchors.get(anchor);
        return target ? targetHref(targetSource, target) : `#/docs/${targetSource.id}`;
    };
}

function sideNav(docs, activeSource, activeSection) {
    return docs.sources.map((source) => {
        const active = source.id === activeSource;
        const overview = `<a class="side-lesson${active && !activeSection ? ' active" aria-current="page' : ""}" href="#/docs/${source.id}">Overview</a>`;
        const links = source.sections.map((section) => {
            const current = active && section.slug === activeSection;
            return `<a class="side-lesson${current ? ' active" aria-current="page' : ""}" href="#/docs/${source.id}/${section.slug}">${escapeHtml(section.title)}</a>`;
        }).join("");
        return `<div class="side-chapter"><h4>${escapeHtml(source.title)}</h4>${overview}${links}</div>`;
    }).join("");
}

function searchResults(docs, query) {
    const normalized = query.toLowerCase().replace(/(\w)\(\)$/, "$1");
    const words = normalized.split(/\s+/);
    const matches = [];
    for (const item of docs.index) {
        const title = item.title.toLowerCase();
        const text = `${title} ${item.text.toLowerCase()}`;
        if (words.every((word) => text.includes(word))) {
            let rank = 3;
            if (title === normalized) {
                rank = 0;
            } else if (title.startsWith(normalized)) {
                rank = 1;
            } else if (title.includes(words[0])) {
                rank = 2;
            }
            matches.push({ item, rank });
        }
    }
    matches.sort((a, b) => a.rank - b.rank);
    if (matches.length === 0) {
        return '<p class="muted small docs-no-match">No matches.</p>';
    }
    return matches.slice(0, 30).map(({ item }) => {
        const group = item.anchor ? item.section.title : item.source.group;
        return `<a class="side-lesson docs-result" href="${sectionHref(item.source, item.section, item.anchor)}">
            <span>${escapeHtml(item.title)}</span><small>${escapeHtml(group)}</small></a>`;
    }).join("");
}

function setView(container, html, className) {
    container.className = `view ${className}`;
    container.innerHTML = html;
    container.classList.remove("view-enter");
    void container.offsetWidth;
    container.classList.add("view-enter");
}

function mount(container, docs, html, sourceId, sectionSlug, anchor, title) {
    if (!container.isConnected) {
        return;
    }
    setView(container, `
        <aside class="side-nav docs-side" aria-label="Documentation navigation">
            <div class="docs-search">
                <input type="search" placeholder="Search the docs" aria-label="Search the documentation" autocomplete="off" spellcheck="false">
            </div>
            <div class="docs-side-list"></div>
        </aside>
        <article class="lesson docs-article">${html}</article>`, "docs-view");
    document.title = `${title} · ${TITLE}`;
    const input = container.querySelector(".docs-search input");
    const list = container.querySelector(".docs-side-list");
    const showTopics = () => {
        list.innerHTML = sideNav(docs, sourceId, sectionSlug);
        const current = list.querySelector("[aria-current]");
        if (current) {
            const active = current.getBoundingClientRect();
            const bounds = list.getBoundingClientRect();
            if (active.bottom > bounds.bottom) {
                list.scrollTop += active.top - bounds.top - 32;
            }
        }
    };
    showTopics();
    input.addEventListener("input", () => {
        const query = input.value.trim();
        if (query) {
            list.innerHTML = searchResults(docs, query);
            list.scrollTop = 0;
        } else {
            showTopics();
        }
    });
    input.addEventListener("keydown", (event) => {
        if (event.key === "Enter") {
            const first = list.querySelector("a");
            if (first) {
                location.hash = first.getAttribute("href");
            }
        } else if (event.key === "Escape") {
            input.value = "";
            showTopics();
            input.blur();
        }
    });
    window.scrollTo({ top: 0 });
    if (anchor) {
        requestAnimationFrame(() => {
            if (!container.isConnected) {
                return;
            }
            const target = document.getElementById(anchor);
            if (target) {
                target.scrollIntoView({ block: "start" });
            }
        });
    }
}

function overview(docs) {
    return `
        <h1>Documentation</h1>
        <p class="lead">Look up razum syntax, library functions and examples.</p>
        <div class="docs-grid docs-overview">
            ${docs.sources.map((source) => `<a class="docs-card" href="#/docs/${source.id}">
                <h2>${escapeHtml(source.title)}</h2>
                <p>${escapeHtml(source.blurb)}</p></a>`).join("")}
        </div>
        <div class="docs-callout">
            <div><h2>New to razum?</h2><p>Write and run your first program in the interactive course.</p></div>
            <a class="button" href="#/learn">Open the course</a>
        </div>`;
}

function sourcePage(docs, source) {
    const resolver = linkResolver(source, docs);
    const cards = source.sections.map((section) => `<a class="docs-card" href="#/docs/${source.id}/${section.slug}">
        <h3>${escapeHtml(section.title)}</h3>
        <p>${escapeHtml(section.summary)}</p></a>`).join("");
    return `
        <p class="crumb"><a href="#/docs">Documentation</a></p>
        <h1>${escapeHtml(source.title)}</h1>
        <div class="lesson-body">${renderMarkdown(source.intro, { link: resolver })}</div>
        <div class="docs-grid">${cards}</div>`;
}

function sectionPage(docs, source, section) {
    const resolver = linkResolver(source, docs);
    const index = source.sections.indexOf(section);
    const previous = source.sections[index - 1];
    const next = source.sections[index + 1];
    const toc = section.subs.length ? `<nav class="docs-toc" aria-label="On this page">
        <span>On this page</span>
        ${section.subs.map((sub) => `<a href="${sectionHref(source, section, sub.slug)}">${escapeHtml(sub.title)}</a>`).join("")}
    </nav>` : "";
    const before = previous
        ? `<a class="button" href="#/docs/${source.id}/${previous.slug}">${escapeHtml(previous.title)}</a>`
        : `<a class="button" href="#/docs/${source.id}">Overview</a>`;
    const after = next
        ? `<a class="button" href="#/docs/${source.id}/${next.slug}">${escapeHtml(next.title)}</a>`
        : "";
    return `
        <p class="crumb"><a href="#/docs">Documentation</a> · <a href="#/docs/${source.id}">${escapeHtml(source.title)}</a></p>
        <h1>${escapeHtml(section.title)}</h1>
        ${toc}
        <div class="lesson-body">${renderMarkdown(section.body.join("\n"), { link: resolver, tableAnchors: source.id === "libraries" })}</div>
        <nav class="lesson-nav" aria-label="Adjacent topics">${before}<span class="spacer"></span>${after}</nav>`;
}

export async function renderDocs(container, sourceId, sectionSlug, anchor) {
    const version = ++renderVersion;
    setView(container, '<article class="lesson docs-article"><p class="muted">Loading documentation…</p></article>', "docs-view");
    let docs;
    try {
        docs = await load();
    } catch (error) {
        if (version === renderVersion && container.isConnected) {
            setView(container, `<article class="lesson docs-article">
                <h1>The documentation is not available</h1>
                <div class="note">${escapeHtml(error.message)} The browser loads the reference from the razum website server; start it with <code>website\\start.ps1</code> or <code>website\\razum-learn.exe</code> and reload.</div>
            </article>`, "docs-view");
            document.title = `Documentation · ${TITLE}`;
        }
        return;
    }
    if (version !== renderVersion || !container.isConnected) {
        return;
    }
    if (!sourceId) {
        mount(container, docs, overview(docs), null, null, null, "Documentation");
        return;
    }
    const source = docs.byId.get(sourceId);
    if (!source) {
        location.replace("#/docs");
        return;
    }
    if (!sectionSlug) {
        mount(container, docs, sourcePage(docs, source), source.id, null, null, source.title);
        return;
    }
    const section = source.sections.find((item) => item.slug === sectionSlug);
    if (!section) {
        location.replace(`#/docs/${source.id}`);
        return;
    }
    mount(container, docs, sectionPage(docs, source, section), source.id, section.slug, anchor, `${section.title} · ${source.title}`);
}
