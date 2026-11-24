import { highlight, escapeHtml } from "./highlight.js";

const INDENT = "    ";
const PAIRS = { "(": ")", "[": "]", "{": "}", '"': '"', "'": "'" };
const CLOSERS = new Set([")", "]", "}", '"', "'"]);
const LINE_HEIGHT = 21;
const PADDING = 12;

export class Editor {
    constructor(host, options = {}) {
        this.options = options;
        this.host = host;
        this.host.classList.add("editor");
        this.host.innerHTML = `
            <div class="editor-gutter" aria-hidden="true"></div>
            <div class="editor-code">
                <div class="editor-marks" aria-hidden="true"></div>
                <pre class="editor-highlight" aria-hidden="true"><code></code></pre>
                <textarea class="editor-input" spellcheck="false" autocapitalize="off" autocomplete="off" wrap="off"
                    aria-label="${escapeHtml(options.label || "razum code")}"></textarea>
            </div>`;
        this.gutter = host.querySelector(".editor-gutter");
        this.marks = host.querySelector(".editor-marks");
        this.code = host.querySelector(".editor-highlight code");
        this.input = host.querySelector(".editor-input");
        this.diagnostics = [];
        this.input.value = options.value || "";
        this.input.addEventListener("input", () => this.changed());
        this.input.addEventListener("keydown", (event) => this.keydown(event));
        this.render();
    }

    get value() {
        return this.input.value;
    }

    set value(text) {
        this.input.value = text;
        this.diagnostics = [];
        this.render();
    }

    focus() {
        this.input.focus();
    }

    changed() {
        this.diagnostics = [];
        this.render();
        if (this.options.onChange) {
            this.options.onChange(this.input.value);
        }
    }

    render() {
        const text = this.input.value;
        this.code.innerHTML = highlight(text) + "\n ";
        const lines = text.split("\n").length;
        const errorLines = new Map();
        for (const diagnostic of this.diagnostics) {
            if (!errorLines.has(diagnostic.line)) {
                errorLines.set(diagnostic.line, []);
            }
            errorLines.get(diagnostic.line).push(diagnostic.message);
        }
        let gutter = "";
        for (let line = 1; line <= lines; line++) {
            const messages = errorLines.get(line);
            gutter += messages
                ? `<div class="gutter-line has-error" title="${escapeHtml(messages.join("\n"))}">${line}</div>`
                : `<div class="gutter-line">${line}</div>`;
        }
        this.gutter.innerHTML = gutter;
        let marks = "";
        for (const [line, messages] of errorLines) {
            if (line >= 1 && line <= lines) {
                marks += `<div class="mark-line" style="top:${PADDING + (line - 1) * LINE_HEIGHT}px" title="${escapeHtml(messages.join("\n"))}"></div>`;
            }
        }
        this.marks.innerHTML = marks;
    }

    setDiagnostics(diagnostics) {
        this.diagnostics = diagnostics;
        this.render();
    }

    revealLine(line, column = 1) {
        const lines = this.input.value.split("\n");
        let offset = 0;
        for (let index = 0; index < Math.min(line - 1, lines.length); index++) {
            offset += lines[index].length + 1;
        }
        offset += Math.max(0, Math.min(column - 1, (lines[line - 1] || "").length));
        this.input.focus();
        this.input.setSelectionRange(offset, offset);
        this.host.scrollTop = Math.max(0, (line - 4) * LINE_HEIGHT);
    }

    insert(text, selectStart, selectEnd) {
        const input = this.input;
        const start = input.selectionStart;
        input.focus();
        if (!document.execCommand("insertText", false, text)) {
            input.setRangeText(text, input.selectionStart, input.selectionEnd, "end");
            this.changed();
        }
        if (selectStart !== undefined) {
            input.setSelectionRange(start + selectStart, start + (selectEnd === undefined ? selectStart : selectEnd));
        }
    }

    replaceRange(from, to, text, caret) {
        this.input.setSelectionRange(from, to);
        this.insert(text);
        if (caret !== undefined) {
            this.input.setSelectionRange(caret, caret);
        }
    }

    lineStart(position) {
        return this.input.value.lastIndexOf("\n", position - 1) + 1;
    }

    keydown(event) {
        const input = this.input;
        const value = input.value;
        const start = input.selectionStart;
        const end = input.selectionEnd;
        const before = value[start - 1];
        const after = value[start];
        const modifier = event.ctrlKey || event.metaKey;
        if (modifier && event.key === "Enter") {
            event.preventDefault();
            if (this.options.onRun) {
                this.options.onRun();
            }
            return;
        }
        if (modifier && (event.key === "s" || event.key === "S")) {
            event.preventDefault();
            if (this.options.onSave) {
                this.options.onSave();
            }
            return;
        }
        if (modifier && event.key === "/") {
            event.preventDefault();
            this.toggleComment();
            return;
        }
        if (event.key === "Tab") {
            event.preventDefault();
            if (event.shiftKey || start !== end) {
                this.indentLines(event.shiftKey ? -1 : 1);
            } else {
                this.insert(INDENT);
            }
            return;
        }
        if (event.key === "Enter" && !modifier) {
            event.preventDefault();
            const lineStart = this.lineStart(start);
            const indent = value.slice(lineStart).match(/^[ \t]*/)[0];
            if (before === "{" && after === "}") {
                this.insert(`\n${indent}${INDENT}\n${indent}`, indent.length + INDENT.length + 1);
            } else if (before === "{") {
                this.insert(`\n${indent}${INDENT}`);
            } else {
                this.insert(`\n${indent}`);
            }
            return;
        }
        if (event.key === "}" && start === end && after !== "}") {
            const lineStart = this.lineStart(start);
            const prefix = value.slice(lineStart, start);
            if (/^ +$/.test(prefix) && prefix.length >= INDENT.length) {
                event.preventDefault();
                this.replaceRange(lineStart, start, prefix.slice(INDENT.length) + "}");
                return;
            }
        }
        if (CLOSERS.has(event.key) && start === end && after === event.key) {
            event.preventDefault();
            input.setSelectionRange(start + 1, start + 1);
            return;
        }
        if (PAIRS[event.key] && !modifier) {
            const quote = event.key === '"' || event.key === "'";
            if (quote && (/\w/.test(before || "") || start !== end)) {
                return;
            }
            if (!quote && /\w/.test(after || "")) {
                return;
            }
            event.preventDefault();
            const selected = value.slice(start, end);
            this.insert(event.key + selected + PAIRS[event.key], 1, 1 + selected.length);
            return;
        }
        if (event.key === "Backspace" && start === end && start > 0) {
            if (PAIRS[before] && PAIRS[before] === after) {
                event.preventDefault();
                this.replaceRange(start - 1, start + 1, "");
                return;
            }
            const lineStart = this.lineStart(start);
            const prefix = value.slice(lineStart, start);
            if (prefix.length >= INDENT.length && /^ +$/.test(prefix) && prefix.length % INDENT.length === 0) {
                event.preventDefault();
                this.replaceRange(start - INDENT.length, start, "");
            }
        }
    }

    selectedLines() {
        const value = this.input.value;
        const start = this.lineStart(this.input.selectionStart);
        let end = value.indexOf("\n", Math.max(this.input.selectionEnd - 1, start));
        if (end < 0) {
            end = value.length;
        }
        return { start, end, text: value.slice(start, end) };
    }

    indentLines(direction) {
        const block = this.selectedLines();
        const lines = block.text.split("\n").map((line) =>
            direction > 0 ? INDENT + line : line.replace(new RegExp(`^ {1,${INDENT.length}}`), ""));
        const text = lines.join("\n");
        this.input.setSelectionRange(block.start, block.end);
        this.insert(text);
        this.input.setSelectionRange(block.start, block.start + text.length);
    }

    toggleComment() {
        const block = this.selectedLines();
        const lines = block.text.split("\n");
        const commented = lines.every((line) => /^\s*\/\//.test(line) || line.trim() === "");
        const text = lines.map((line) => {
            if (line.trim() === "") {
                return line;
            }
            return commented ? line.replace(/^(\s*)\/\/ ?/, "$1") : line.replace(/^(\s*)/, "$1// ");
        }).join("\n");
        this.input.setSelectionRange(block.start, block.end);
        this.insert(text);
        this.input.setSelectionRange(block.start, block.start + text.length);
    }
}
