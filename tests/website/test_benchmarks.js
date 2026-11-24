import assert from "node:assert/strict";
import fs from "node:fs";
import { renderBenchmarks } from "../../website/public/js/benchmarks.js";

const fixture = `## Library performance

| Workload | Windows before → after | WSL before → after |
|---|---:|---:|
| \`array\`, integer sorts | 10 → 10 | 5 → 5 |
| \`filesystem\`, 32 MiB | **50 → 20** | **40 → 10** |
| \`linux\`, environment \`A \\| B\` | Linux only | 12 → 11 |

## Masked double loops

| Build | Median elapsed time | Executable bytes |
|---|---:|---:|
| Portable \`-O\` with SSE2 | 100 ms | 2,608 |
| \`-march=znver3\` with AVX2 | 50 ms | 2,488 |
| C | 80 ms | 38,747 |
| C++ | 90 ms | 38,747 |
| Rust | 100 ms | 1,230,291 |

## Inlining and loop invariants

| Workload | Before \`-O\` | After \`-O\` | Before Zen 3 | After Zen 3 | C | C++ | Rust |
|---|---:|---:|---:|---:|---:|---:|---:|
| Mandelbrot | 200 | 200 | 200 | 200 | 200 | 200 | 200 |
| Quicksort | 300 | 300 | 300 | 310 | 400 | 375 | 270 |
| Missing peers | 1 | 2 | 3 | 4 | | | |

## Compiler optimization on Zen 3

| Workload | Old \`-O\` | New \`-O\` | New Zen 3 | C | C++ | Rust |
|---|---:|---:|---:|---:|---:|---:|
| Sieve | 240 | 220 | 200 | 180 | 170 | 130 |
`;

const libraryGrid = { innerHTML: "" };
const workload = { selectedIndex: 0, addEventListener(type, listener) { this[type] = listener; } };
const view = {
    isConnected: true,
    markup: "",
    get innerHTML() { return this.markup; },
    set innerHTML(value) { this.markup = value; this.firstElementChild = {}; },
    querySelector(selector) { return selector === "#benchmark-workload" ? workload : libraryGrid; },
};
globalThis.window = { scrollTo() {} };
let source = fixture;
globalThis.fetch = async (url, options) => {
    assert.equal(url, "/api/benchmarks");
    assert.equal(options.cache, "no-store");
    return { ok: source !== null, text: async () => source };
};

await renderBenchmarks(view);
const [charts, fullReport] = view.innerHTML.split('<details class="benchmark-details" id="benchmark-report">');
assert.match(charts, /<h1>Benchmarks<\/h1>/);
assert.match(charts, /<h3>Mandelbrot<\/h3><code>Zen 3<\/code>[\s\S]+?width:50\.000%[\s\S]+?50 ms[\s\S]+?80 ms/);
assert.match(charts, /<h3>Quicksort<\/h3>[\s\S]+?310 ms[\s\S]+?270 ms/);
assert.match(charts, /<h3>Sieve<\/h3><code>-O<\/code>[\s\S]+?220 ms/);
assert.doesNotMatch(charts, /200 ms|Missing peers/);
assert.match(charts, /<option selected>filesystem, 32 MiB<\/option>/);
assert.match(charts, /<option>linux, environment A \| B<\/option>/);
assert.match(charts, /20 ms[\s\S]+?<h3>WSL<\/h3>[\s\S]+?width:80\.000%[\s\S]+?40 ms/);
assert.match(charts, /href="#\/benchmarks#library-performance"/);
assert.match(fullReport, /<table>/);
assert.match(fullReport, /python benchmarks\/run_libraries\.py/);
workload.selectedIndex = 2;
workload.change();
assert.match(libraryGrid.innerHTML, /<h3>WSL<\/h3>[\s\S]+?12 ms[\s\S]+?11 ms/);
assert.doesNotMatch(libraryGrid.innerHTML, /<h3>Windows<\/h3>|NaN/);

source = fixture.replace("50 → 20", "50 → 15.100");
const disclosure = { open: false };
let scrolled = false;
globalThis.document = { getElementById(id) {
    assert.equal(id, "masked-double-loops");
    return { closest: () => disclosure, scrollIntoView() { scrolled = disclosure.open; } };
} };
await renderBenchmarks(view, "masked-double-loops");
assert.ok(scrolled, "report anchors open the details before scrolling");
assert.match(view.innerHTML, /15\.100 ms/, "measurement edits appear on the next visit");

source = fixture.replace("12 → 11", "60 → 11") + `
## Current compiler benchmarks

| Workload | Before Zen 3 | After Zen 3 | C | C++ | Rust |
|---|---:|---:|---:|---:|---:|
| Mandelbrot | 51 | 49 | 86 | 95 | 110 |
| Quicksort | 310 | 290 | 405 | 380 | 275 |
| Sieve | 200 | 195 | 170 | 170 | |
`;
await renderBenchmarks(view);
const [currentCharts] = view.innerHTML.split("<details");
assert.match(currentCharts, /<h3>Mandelbrot<\/h3>[\s\S]+?49 ms[\s\S]+?110 ms/);
assert.match(currentCharts, /<h3>Quicksort<\/h3>[\s\S]+?290 ms[\s\S]+?275 ms/);
assert.match(currentCharts, /<h3>Sieve<\/h3><code>-O<\/code>[\s\S]+?220 ms/);
assert.match(currentCharts, /<option selected>linux, environment A \| B<\/option>/);

source = fs.readFileSync(new URL("../../KNOWLEDGE.md", import.meta.url), "utf8");
await renderBenchmarks(view);
assert.match(view.innerHTML, /benchmark-compiler-grid/);
assert.match(view.innerHTML, /benchmark-library-grid/);

source = "## Library performance\nRecorded measurements are pending.\n";
await renderBenchmarks(view);
assert.doesNotMatch(view.innerHTML, /benchmark-compiler-grid|benchmark-library-grid|NaN/);
assert.match(view.innerHTML, /Recorded measurements are pending/);
source = null;
await renderBenchmarks(view);
assert.match(view.innerHTML, /Benchmark results could not be loaded/);

source = fixture;
const pending = renderBenchmarks(view);
view.innerHTML = "<h1>Playground</h1>";
await pending;
assert.equal(view.innerHTML, "<h1>Playground</h1>");
console.log("website benchmarks: comparison charts, workload change, refresh, links and error state ok");
