const vscode = require("vscode");
const path = require("path");
const os = require("os");
const fs = require("fs");
const { spawn } = require("child_process");

const CHECK_DELAY_MS = 250;
const ERROR_LINE = /^(.*?):(\d+):(\d+): error: (.*)$/;
const LIBRARY_PREFIX = "<rzc>/";
const LIBRARY_SCHEME = "razum-library";

function compilerPath(context) {
    const configured = vscode.workspace.getConfiguration("razum").get("compilerPath");
    return configured || path.join(context.extensionPath, "bin", process.platform === "win32" ? "rzc.exe" : "rzc");
}

function samePath(left, right) {
    return path.resolve(left).toLowerCase() === path.resolve(right).toLowerCase();
}

function runCompiler(context, args, input) {
    return new Promise((resolve) => {
        const child = spawn(compilerPath(context), args, { windowsHide: true });
        let output = "";
        let errors = "";
        child.stdout.on("data", (chunk) => { output += chunk; });
        child.stderr.on("data", (chunk) => { errors += chunk; });
        child.on("error", (error) => resolve({ code: -1, output, errors: `cannot start rzc (${error.message}); set razum.compilerPath` }));
        child.on("close", (code) => resolve({ code, output, errors }));
        child.stdin.on("error", () => {});
        child.stdin.end(input === undefined ? "" : input);
    });
}

function toDiagnostics(document, output) {
    const diagnostics = [];
    for (const line of output.split(/\r?\n/)) {
        if (line.trim() === "") {
            continue;
        }
        const match = ERROR_LINE.exec(line);
        if (!match) {
            diagnostics.push(new vscode.Diagnostic(new vscode.Range(0, 0, 0, 1), line.trim(), vscode.DiagnosticSeverity.Error));
            continue;
        }
        const [, file, row, column, message] = match;
        if (samePath(file, document.fileName)) {
            const position = new vscode.Position(Number(row) - 1, Math.max(0, Number(column) - 1));
            const range = document.getWordRangeAtPosition(position) || new vscode.Range(position, position.translate(0, 1));
            diagnostics.push(new vscode.Diagnostic(range, message, vscode.DiagnosticSeverity.Error));
        } else {
            diagnostics.push(new vscode.Diagnostic(new vscode.Range(0, 0, 0, 1), `${file}:${row}:${column}: ${message}`,
                vscode.DiagnosticSeverity.Error));
        }
    }
    return diagnostics;
}

function sourceArgs(document, ...rest) {
    const folder = vscode.workspace.getWorkspaceFolder && vscode.workspace.getWorkspaceFolder(document.uri);
    return [document.fileName, ...(folder ? ["-I", folder.uri.fsPath] : []), "--stdin", ...rest];
}

async function checkDocument(context, diagnostics, document) {
    if (document.languageId !== "razum" || document.uri.scheme !== "file") {
        return;
    }
    const version = document.version;
    const result = await runCompiler(context, sourceArgs(document, "--check"), document.getText());
    if (document.version !== version) {
        return;
    }
    diagnostics.set(document.uri, result.code === 0 ? [] : toDiagnostics(document, result.errors));
}

const symbolCache = new Map();

function parseSymbols(output) {
    const symbols = [];
    for (const line of output.split(/\r?\n/)) {
        const fields = line.split("\t");
        if (fields.length < 8) {
            continue;
        }
        const [kind, name, detail, file, row, column, scopeStart, scopeEnd] = fields;
        symbols.push({ kind, name, detail, file, line: Number(row) - 1, column: Number(column) - 1,
            scopeStart: Number(scopeStart) - 1, scopeEnd: Number(scopeEnd) - 1 });
    }
    return symbols;
}

async function symbolsOf(context, document) {
    const key = document.uri.toString();
    const cached = symbolCache.get(key);
    if (cached && cached.version === document.version) {
        return cached.symbols;
    }
    const result = await runCompiler(context, sourceArgs(document, "--symbols"), document.getText());
    const symbols = parseSymbols(result.output);
    symbolCache.set(key, { version: document.version, symbols });
    return symbols;
}

function isHere(symbol, document) {
    return !symbol.file.startsWith(LIBRARY_PREFIX) && samePath(symbol.file, document.fileName);
}

function resolveSymbol(symbols, document, position) {
    const range = document.getWordRangeAtPosition(position, /[A-Za-z_]\w*/);
    if (!range) {
        return [];
    }
    const word = document.getText(range);
    const before = document.lineAt(range.start.line).text.slice(0, range.start.character);
    const owner = /([A-Za-z_]\w*)\s*(?:\.|->)\s*$/.exec(before);
    if (owner) {
        const members = symbols.filter((symbol) => symbol.kind === "member" && symbol.name === word && symbol.detail.startsWith(`${owner[1]}.`));
        if (members.length > 0) {
            return members;
        }
        return symbols.filter((symbol) => symbol.kind === "field" && symbol.name === word);
    }
    const scoped = symbols.filter((symbol) => symbol.name === word && symbol.scopeEnd >= 0 && isHere(symbol, document) &&
        symbol.scopeStart <= position.line && position.line <= symbol.scopeEnd && symbol.line <= position.line);
    if (scoped.length > 0) {
        scoped.sort((left, right) => right.line - left.line || right.column - left.column);
        return [scoped[0]];
    }
    const global = symbols.filter((symbol) => symbol.name === word && symbol.scopeEnd < 0 && symbol.kind !== "member" && symbol.kind !== "field");
    global.sort((left, right) => Number(isHere(right, document)) - Number(isHere(left, document)));
    return global.slice(0, 1);
}

function locationOf(symbol) {
    const uri = symbol.file.startsWith(LIBRARY_PREFIX)
        ? vscode.Uri.from({ scheme: LIBRARY_SCHEME, path: `/${symbol.file.slice(LIBRARY_PREFIX.length)}` })
        : vscode.Uri.file(path.resolve(symbol.file));
    return new vscode.Location(uri, new vscode.Position(Math.max(0, symbol.line), Math.max(0, symbol.column)));
}

function describeOrigin(symbol, document) {
    if (symbol.kind === "builtin") {
        return "built into the language";
    }
    if (symbol.file.startsWith(LIBRARY_PREFIX)) {
        return `library \`${symbol.file.slice(LIBRARY_PREFIX.length)}\`, line ${symbol.line + 1}`;
    }
    if (isHere(symbol, document)) {
        return `line ${symbol.line + 1}`;
    }
    return `\`${path.basename(symbol.file)}\`, line ${symbol.line + 1}`;
}

function hoverProvider(context) {
    return {
        async provideHover(document, position) {
            const found = resolveSymbol(await symbolsOf(context, document), document, position);
            if (found.length === 0) {
                return null;
            }
            const markdown = new vscode.MarkdownString();
            for (const symbol of found) {
                markdown.appendCodeblock(symbol.detail, "razum");
                markdown.appendMarkdown(`*${symbol.kind}* · ${describeOrigin(symbol, document)}\n\n`);
            }
            return new vscode.Hover(markdown);
        },
    };
}

function definitionProvider(context) {
    return {
        async provideDefinition(document, position) {
            return resolveSymbol(await symbolsOf(context, document), document, position)
                .filter((symbol) => symbol.kind !== "builtin")
                .map(locationOf);
        },
    };
}

function libraryContentProvider(context) {
    return {
        async provideTextDocumentContent(uri) {
            const result = await runCompiler(context, ["--source", `${LIBRARY_PREFIX}${uri.path.replace(/^\//, "")}`]);
            return result.code === 0 ? result.output : result.errors;
        },
    };
}

function quote(text) {
    return `'${text.replace(/'/g, "''")}'`;
}

async function runFile(context) {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== "razum") {
        return;
    }
    await editor.document.save();
    const terminal = vscode.window.terminals.find((existing) => existing.name === "Razum") || vscode.window.createTerminal({ name: "Razum", shellPath: "powershell.exe" });
    terminal.show(true);
    terminal.sendText(`& ${quote(compilerPath(context))} ${quote(editor.document.fileName)} --run; "exit code: $LASTEXITCODE"`);
}

async function showAsm(context) {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== "razum") {
        return;
    }
    const folder = fs.mkdtempSync(path.join(os.tmpdir(), "razum-"));
    const name = path.basename(editor.document.fileName, ".rz");
    const exePath = path.join(folder, `${name}.exe`);
    const result = await runCompiler(context, sourceArgs(editor.document, "-S", "-o", exePath), editor.document.getText());
    if (result.code !== 0) {
        vscode.window.showErrorMessage(result.errors.trim() || `rzc failed with exit code ${result.code}`);
        return;
    }
    const asmDocument = await vscode.workspace.openTextDocument(path.join(folder, `${name}.asm`));
    await vscode.window.showTextDocument(asmDocument, vscode.ViewColumn.Beside, true);
}

function activate(context) {
    const diagnostics = vscode.languages.createDiagnosticCollection("razum");
    const timers = new Map();
    const schedule = (document) => {
        const key = document.uri.toString();
        clearTimeout(timers.get(key));
        timers.set(key, setTimeout(() => {
            timers.delete(key);
            checkDocument(context, diagnostics, document);
        }, CHECK_DELAY_MS));
    };
    const selector = [{ language: "razum", scheme: "file" }, { language: "razum", scheme: LIBRARY_SCHEME }];
    context.subscriptions.push(
        diagnostics,
        vscode.workspace.onDidOpenTextDocument(schedule),
        vscode.workspace.onDidChangeTextDocument((event) => schedule(event.document)),
        vscode.workspace.onDidCloseTextDocument((document) => {
            diagnostics.delete(document.uri);
            symbolCache.delete(document.uri.toString());
        }),
        vscode.languages.registerHoverProvider(selector, hoverProvider(context)),
        vscode.languages.registerDefinitionProvider(selector, definitionProvider(context)),
        vscode.workspace.registerTextDocumentContentProvider(LIBRARY_SCHEME, libraryContentProvider(context)),
        vscode.commands.registerCommand("razum.run", () => runFile(context)),
        vscode.commands.registerCommand("razum.showAsm", () => showAsm(context)),
    );
    vscode.workspace.textDocuments.forEach(schedule);
}

function deactivate() {}

module.exports = { activate, deactivate, parseSymbols, resolveSymbol, sourceArgs, toDiagnostics };
