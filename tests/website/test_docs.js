import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { pathToFileURL } from "node:url";

const root = process.cwd();
const texts = {
    language: fs.readFileSync(path.join(root, "docs", "language.md"), "utf8"),
    libraries: fs.readFileSync(path.join(root, "docs", "libraries.md"), "utf8"),
    projects: fs.readFileSync(path.join(root, "docs", "projects.md"), "utf8"),
    discord: fs.readFileSync(path.join(root, "docs", "discord.md"), "utf8"),
};

globalThis.fetch = async (url) => {
    const name = String(url).replace(/^docs\//, "").replace(/\.md$/, "");
    return { ok: name in texts, status: name in texts ? 200 : 404, text: async () => texts[name] };
};
globalThis.document = { getElementById: () => null, title: "" };
globalThis.window = { scrollTo() {} };
globalThis.requestAnimationFrame = (fn) => fn();
globalThis.location = { replace() {} };

const { renderDocs } = await import(pathToFileURL(path.join(root, "website", "public", "js", "docs.js")).href);
const { slugify } = await import(pathToFileURL(path.join(root, "website", "public", "js", "markdown.js")).href);

function container() {
    const nodes = {
        input: { value: "", addEventListener(type, fn) { listeners[type] = fn; } },
        list: { innerHTML: "", querySelector() { return null; } },
    };
    const listeners = {};
    return {
        isConnected: true,
        className: "",
        innerHTML: "",
        offsetWidth: 0,
        classList: { remove() {}, add() {} },
        nodes,
        listeners,
        querySelector(selector) {
            if (selector === ".docs-search input") {
                return nodes.input;
            }
            if (selector === ".docs-side-list") {
                return nodes.list;
            }
            return null;
        },
    };
}

async function render(source, section, anchor) {
    const view = container();
    await renderDocs(view, source, section, anchor);
    return view;
}

function topics(text) {
    const sections = [];
    let fence = false;
    for (const line of text.split("\n")) {
        if (line.startsWith("```")) {
            fence = !fence;
        } else if (!fence && line.startsWith("## ")) {
            const title = line.slice(3).trim();
            sections.push({ title, slug: slugify(title), anchors: [slugify(title)] });
        } else if (!fence && line.startsWith("### ") && sections.length > 0) {
            sections[sections.length - 1].anchors.push(slugify(line.slice(4).trim()));
        }
    }
    return sections;
}

const index = {};
for (const [id, text] of Object.entries(texts)) {
    index[id] = topics(text);
}
assert.ok(index.language.length >= 23, "language sections");
const libraryFolders = fs.readdirSync(path.join(root, "libraries"), { withFileTypes: true })
    .filter((entry) => entry.isDirectory())
    .map((entry) => entry.name);
for (const name of libraryFolders) {
    assert.ok(index.libraries.some((item) => item.slug === name), `missing library page: ${name}`);
}

function valid(href, current) {
    if (!href.startsWith("#/docs")) {
        return true;
    }
    const [route, anchor] = href.slice(2).split("#");
    const [, sourceId, sectionSlug] = route.split("/");
    const source = sourceId || current;
    const sections = index[source];
    if (!sections) {
        return false;
    }
    const section = sections.find((item) => item.slug === sectionSlug);
    if (!sectionSlug) {
        return true;
    }
    return Boolean(section) && (!anchor || section.anchors.includes(anchor));
}

const pages = [[undefined, undefined]];
for (const sourceId of Object.keys(texts)) {
    pages.push([sourceId, undefined]);
    for (const section of index[sourceId]) {
        pages.push([sourceId, section.slug]);
    }
}
let checked = 0;
for (const [sourceId, sectionSlug] of pages) {
    const page = await render(sourceId, sectionSlug);
    assert.ok(page.innerHTML.includes("docs-side"), "sidebar");
    assert.ok(page.innerHTML.includes("lesson-body") || page.innerHTML.includes("docs-grid"), "content");
    assert.ok(!page.innerHTML.includes(">undefined<"), "undefined text");
    const ids = Array.from(page.innerHTML.matchAll(/\bid="([^"]+)"/g), (match) => match[1]);
    assert.equal(ids.length, new Set(ids).size, `duplicate id on ${sourceId}/${sectionSlug}`);
    const markup = page.innerHTML + page.nodes.list.innerHTML;
    for (const match of markup.matchAll(/href="(#[^"]*)"/g)) {
        assert.ok(valid(match[1], sourceId), `broken link ${match[1]} on ${sourceId}/${sectionSlug || "overview"}`);
        checked++;
    }
}

const language = await render("language");
assert.ok(language.innerHTML.includes('href="#/learn"'), "tutorial links to the course");
const modules = await render("language", "modules-and-import");
assert.match(modules.innerHTML, /href="#\/docs\/projects">the project manifest/);
const operators = await render("language", "operators");
assert.ok(operators.innerHTML.replace(/<\/?span[^>]*>/g, "").includes("<code>a || b</code>"), "escaped pipes in tables");
const statements = await render("language", "statements");
assert.match(statements.innerHTML, /<td><code><span class="tok-type">int<\/span> x /, "table examples use syntax colors");
assert.match(statements.innerHTML, /<span class="tok-number">5<\/span>/, "table literal uses syntax colors");
const libraries = await render("libraries");
assert.ok(!libraries.innerHTML.includes("]("), "code-labelled links are rendered");
const http = await render("libraries", "http");
assert.match(http.innerHTML, /href="#\/docs\/libraries\/threads">threads<\/a>/, "HTTP links to the threads library");

const crypt = await render("libraries", "crypt");
crypt.nodes.input.value = "hmac";
crypt.listeners.input();
assert.ok(crypt.nodes.list.innerHTML.includes("crypt.hmac"), "search finds an h3");
assert.ok(crypt.nodes.list.innerHTML.includes("#/docs/libraries/crypt#crypthmac"), "search links to the h3 anchor");

for (const [query, section, anchor] of [
    ["aes_gcm_decrypt", "crypt", "api-aes_gcm_decrypt"],
    ["http_listen()", "http", "api-http_listen"],
    ["http_https_get()", "http", "api-http_https_get"],
    ["net_tcp_connect_for", "net", "api-net_tcp_connect_for"],
    ["write_long", "stdio", "api-write_bool"],
    ["WSACleanup", "winsock", "api-wsastartup"],
    ["environment_value", "linux", "api-environment_value"],
]) {
    crypt.nodes.input.value = query;
    crypt.listeners.input();
    const first = crypt.nodes.list.innerHTML.match(/href="([^"]+)"/)?.[1];
    assert.equal(first, `#/docs/libraries/${section}#${anchor}`, `first result for ${query}`);
    const result = await render("libraries", section, anchor);
    assert.ok(result.innerHTML.includes(`<tr id="${anchor}">`), `missing function row for ${query}`);
}

console.log(`website docs: ${pages.length} pages, ${libraryFolders.length} libraries, ${checked} links, search ok`);
