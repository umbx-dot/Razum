const ERROR_LINE = /^([^:\r\n]+):(\d+):(\d+): error: (.*)$/;

async function post(path, body) {
    const response = await fetch(path, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body),
    });
    const data = await response.json().catch(() => ({ error: `The server answered ${response.status}.` }));
    if (!response.ok && !data.error) {
        data.error = `The server answered ${response.status}.`;
    }
    return data;
}

export function runCode(code, input) {
    return post("/api/run", { code, input: input || "" });
}

export function checkCode(code) {
    return post("/api/check", { code });
}

export async function serverOnline() {
    try {
        const response = await fetch("/api/health", { cache: "no-store" });
        return response.ok;
    } catch {
        return false;
    }
}

export function parseErrors(output) {
    const errors = [];
    for (const line of (output || "").split(/\r?\n/)) {
        const match = ERROR_LINE.exec(line);
        if (match) {
            errors.push({ file: match[1], line: Number(match[2]), column: Number(match[3]), message: match[4] });
        } else if (line.trim() !== "") {
            errors.push({ file: "", line: 0, column: 0, message: line.trim() });
        }
    }
    return errors;
}

export function normalizeOutput(text) {
    return (text || "").replace(/\r/g, "").split("\n").map((line) => line.replace(/\s+$/, "")).join("\n").replace(/\n+$/, "");
}
