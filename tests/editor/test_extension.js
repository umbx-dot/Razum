
const assert = require("assert");
const fs = require("fs");
const { execFileSync } = require("child_process");
const { Position } = require("vscode");
const extension = require("../../editors/vscode/extension.js");
const file = process.argv[2];
const text = fs.readFileSync(file, "utf8");
const lines = text.split(/\r?\n/);
const document = {
    fileName: file,
    lineAt: (line) => ({ text: lines[line] }),
    getText: (range) => range ? lines[range.start.line].slice(range.start.character, range.end.character) : text,
    getWordRangeAtPosition: (position, pattern) => {
        const line = lines[position.line];
        const regex = /[A-Za-z_]\w*/g;
        let match;
        while ((match = regex.exec(line))) {
            if (match.index <= position.character && position.character <= match.index + match[0].length) {
                return { start: new Position(position.line, match.index), end: new Position(position.line, match.index + match[0].length) };
            }
        }
        return undefined;
    },
};
const output = execFileSync(process.env.RZC || "build/rzc.exe", [file, "--symbols"]).toString();
const symbols = extension.parseSymbols(output);
function at(lineText, word, occurrence = 0) {
    const line = lines.findIndex((candidate) => candidate.includes(lineText));
    let column = -1;
    for (let index = 0; index <= occurrence; index++) {
        column = lines[line].indexOf(word, column + 1);
    }
    const found = extension.resolveSymbol(symbols, document, new Position(line, column + 1));
    return found.map((symbol) => `${symbol.kind}: ${symbol.detail} @${symbol.line + 1}`).join(" | ");
}
const checks = [
    ["print(SIZE);", "SIZE", "const: const int SIZE = 4 @3"],
    ["color = next(color);", "next", "function: Color next(Color color) @47"],
    ["color = next(color);", "color", "local: Color color @82"],
    ["return color == Color.WHITE", "color", "parameter: Color color @47"],
    ["return color == Color.WHITE", "WHITE", "member: Color.WHITE = 3 @20"],
    ["pixel.color = Color.GREEN;", "color", "field: Color Pixel.color @26"],
    ["Pixel pixel;", "Pixel", "struct: struct Pixel { Color color; char[5] label; } @25"],
    ["print(SEPARATOR);", "print", "builtin: void print(any value): writes the value and a line end; needs import stdio @0"],
    ["write_char(SEPARATOR);", "write_char", "function: void write_char(char value) @1"],
    ["int values[LOCAL];", "LOCAL", "const: const int LOCAL = 15 @66"],
];
let failures = 0;
for (const [line, word, expected] of checks) {
    const actual = at(line, word);
    const ok = actual === expected;
    if (!ok) failures++;
    console.log(`${ok ? "ok  " : "FAIL"} ${word} in "${line}" -> ${actual}${ok ? "" : "   expected " + expected}`);
}
const diagnostics = extension.toDiagnostics(document, `${file}:3:5: error: first\n${file}:7:1: error: second\n`);
console.log(diagnostics.length === 2 ? "ok   two diagnostics from two error lines" : "FAIL diagnostics " + diagnostics.length);
const vscode = require("vscode");
vscode.workspace.getWorkspaceFolder = () => ({ uri: { fsPath: "project" } });
const args = extension.sourceArgs({ fileName: "project/ui/show.rz", uri: {} }, "--check").join(" ");
const argsOk = args === "project/ui/show.rz -I project --stdin --check";
if (!argsOk) failures++;
console.log(argsOk ? "ok   the workspace folder is passed as -I" : "FAIL source arguments: " + args);
process.exit(failures);
