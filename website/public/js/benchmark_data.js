import { tableCells } from "./markdown.js";

function table(report, heading) {
    const section = report.split(/^## /m).find((part) => part.split("\n", 1)[0].trim() === heading);
    const lines = section?.split("\n") ?? [];
    const start = lines.findIndex((line) => line.startsWith("|"));
    if (start < 0) {
        return [];
    }
    const cells = (line) => tableCells(line.trim()).map((cell) => cell.replace(/\*\*|`/g, "").trim());
    const headings = cells(lines[start]);
    const rows = [];
    for (const line of lines.slice(start + 2)) {
        if (!line.startsWith("|")) {
            break;
        }
        const values = cells(line);
        rows.push(Object.fromEntries(headings.map((label, index) => [label, values[index]])));
    }
    return rows;
}

function milliseconds(cell) {
    return cell?.match(/^(\d+(?:\.\d+)?)(?: ms)?$/)?.[1];
}

function timings(entries) {
    return entries.map(([label, cell]) => [label, milliseconds(cell)]);
}

function complete(entries) {
    return entries.every(([, value]) => value !== undefined && Number.isFinite(Number(value)));
}

function addCompilerRun(workloads, rows, column, target) {
    for (const row of rows) {
        const bars = timings([["Razum", row[column]], ["C", row.C], ["C++", row["C++"]], ["Rust", row.Rust]]);
        if (row.Workload && complete(bars)) {
            workloads.set(row.Workload, { bars, target });
        }
    }
}

export function compilerBenchmarks(report) {
    const workloads = new Map();
    addCompilerRun(workloads, table(report, "Compiler optimization on Zen 3"), "New -O", "-O");
    addCompilerRun(workloads, table(report, "Inlining and loop invariants"), "After Zen 3", "Zen 3");

    const masked = Object.fromEntries(table(report, "Masked double loops").map((row) => [row.Build, row["Median elapsed time"]]));
    const bars = timings([["Razum", masked["-march=znver3 with AVX2"]], ["C", masked.C], ["C++", masked["C++"]], ["Rust", masked.Rust]]);
    if (complete(bars)) {
        workloads.set("Mandelbrot", { bars, target: "Zen 3" });
    }
    addCompilerRun(workloads, table(report, "Current compiler benchmarks"), "After Zen 3", "Zen 3");

    const order = ["Mandelbrot", "Integer loop", "Quicksort"];
    const names = new Set([...order, ...workloads.keys()]);
    return [...names].filter((name) => workloads.has(name)).map((name) => ({ name, ...workloads.get(name) }));
}

export function libraryBenchmarks(report) {
    return table(report, "Library performance").flatMap((row) => {
        const platforms = ["Windows", "WSL"].flatMap((name) => {
            const [before, after] = (row[`${name} before → after`] ?? "").split("→").map((cell) => cell.trim());
            const bars = timings(before === "new" ? [["Current", after]] : [["Before", before], ["After", after]]);
            return complete(bars) ? [{ name, bars }] : [];
        });
        return row.Workload && platforms.length ? [{ name: row.Workload, platforms }] : [];
    });
}

export function projectBenchmarks(report) {
    return table(report, "Project build performance").flatMap((row) => {
        const bars = timings([["Windows", row.Windows], ["WSL", row.WSL]]).filter(([, value]) => value !== undefined);
        return / (cold|warm) build$/.test(row.Workload) && bars.length ? [{ name: row.Workload, bars }] : [];
    });
}
