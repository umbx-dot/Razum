import { compilerBenchmarks, libraryBenchmarks } from "./benchmark_data.js";
import { escapeHtml } from "./highlight.js";
import { renderMarkdown } from "./markdown.js";

const header = `<header class="benchmarks-intro">
    <h1>Benchmarks</h1>
    <p>Median runtime · Shorter bars are faster</p>
</header>`;

const commands = `<section aria-labelledby="reproduce-the-results">
    <h2 id="reproduce-the-results">Reproduce the results</h2>
    <p>From the repository root:</p>
    <pre class="code-block"><code>python benchmarks/run_libraries.py
python benchmarks/run.py --compare c --compare cpp --compare rust --cpu znver3 -- -O -march=znver3</code></pre>
    <p>Use <code>--baseline PATH</code> for the library before/after comparison and <code>python3</code> if needed. See <a href="/docs/optimizer.md">target flags and comparison options</a>.</p>
</section>`;

function chart(bars, maximum = Math.max(...bars.map(([, value]) => Number(value)))) {
    return `<dl class="benchmark-bars">${bars.map(([label, value]) => {
        const width = maximum ? (100 * (Number(value) / maximum)).toFixed(3) : 0;
        const highlight = label === "Razum" || label === "After";
        return `<div class="benchmark-bar${highlight ? " benchmark-bar-accent" : ""}">
            <dt>${label}</dt>
            <dd><span class="benchmark-track" aria-hidden="true"><span style="width:${width}%"></span></span><span>${value} ms</span></dd>
        </div>`;
    }).join("")}</dl>`;
}

function compilerCharts(workloads) {
    if (!workloads.length) {
        return "";
    }
    return `<section class="benchmark-section" aria-labelledby="compiler-performance">
        <div class="benchmark-section-heading">
            <h2 id="compiler-performance">Razum vs. C, C++ and Rust</h2>
            <p>Each workload has its own scale.</p>
        </div>
        <div class="benchmark-compiler-grid">
            ${workloads.map(({ name, bars, target }) => `<article class="benchmark-card">
                <div class="benchmark-card-heading"><h3>${escapeHtml(name)}</h3><code>${target}</code></div>
                ${chart(bars)}
            </article>`).join("")}
        </div>
    </section>`;
}

function libraryCharts(workload) {
    const maximum = Math.max(...workload.platforms.flatMap(({ bars }) => bars.map(([, value]) => Number(value))));
    return workload.platforms.map(({ name, bars }) => `<article class="benchmark-card">
        <h3>${name}</h3>${chart(bars, maximum)}
    </article>`).join("");
}

function librarySection(workloads) {
    if (!workloads.length) {
        return "";
    }
    const savings = workloads.map(({ platforms }) => Math.max(...platforms.map(({ bars }) =>
        Number(bars[0][1]) - Number(bars[1][1]))));
    const initial = Math.max(0, savings.indexOf(Math.max(0, ...savings)));
    return `<section class="benchmark-section" aria-labelledby="library-charts">
        <div class="benchmark-section-heading"><h2 id="library-charts">Standard libraries</h2><p>Portable <code>-O</code> · Before and after</p></div>
        <label class="benchmark-workload" for="benchmark-workload">Workload</label>
        <select id="benchmark-workload">${workloads.map(({ name }, index) =>
            `<option${index === initial ? " selected" : ""}>${escapeHtml(name)}</option>`).join("")}</select>
        <div class="benchmark-library-grid" aria-live="polite">${libraryCharts(workloads[initial])}</div>
    </section>`;
}

export async function renderBenchmarks(root, anchor) {
    root.className = "view benchmarks-page";
    root.innerHTML = `${header}<p role="status">Loading benchmark results…</p>`;
    const loadingView = root.firstElementChild;
    window.scrollTo({ top: 0 });
    try {
        const response = await fetch("/api/benchmarks", { cache: "no-store" });
        if (!response.ok) {
            throw new Error("Benchmark results are unavailable");
        }
        const markdown = await response.text();
        if (!root.isConnected || root.firstElementChild !== loadingView) {
            return;
        }
        const html = renderMarkdown(markdown, {
            link: (url) => url.startsWith("docs/optimizer.md") ? "/docs/optimizer.md" : null,
        });
        const workloads = libraryBenchmarks(markdown);
        const compilers = compilerCharts(compilerBenchmarks(markdown));
        const libraries = librarySection(workloads);
        root.innerHTML = `${header}<nav class="benchmark-jumps" aria-label="Benchmark sections">
            ${compilers ? '<a href="#/benchmarks#compiler-performance">Compiler</a>' : ""}
            ${libraries ? '<a href="#/benchmarks#library-performance">Libraries</a>' : ""}
            <a href="#/benchmarks#benchmark-report">Full results</a>
        </nav>${compilers}${libraries}<details class="benchmark-details" id="benchmark-report">
            <summary>Full results, methodology and commands</summary>
            <div class="lesson-body">${html}${commands}</div>
        </details>`;
        const select = root.querySelector("#benchmark-workload");
        const libraryGrid = root.querySelector(".benchmark-library-grid");
        select?.addEventListener("change", () => {
            libraryGrid.innerHTML = libraryCharts(workloads[select.selectedIndex]);
        });
        if (anchor) {
            const libraryAnchor = anchor === "library-performance" && libraries;
            const target = document.getElementById(libraryAnchor ? "library-charts" : anchor);
            const details = target?.closest("details");
            if (details) {
                details.open = true;
            }
            target?.scrollIntoView();
        }
    } catch {
        if (root.isConnected && root.firstElementChild === loadingView) {
            root.innerHTML = `${header}<p role="status">Benchmark results could not be loaded. Run the site from the Razum repository to view the recorded results.</p>`;
        }
    }
}
