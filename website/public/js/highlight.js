const KEYWORDS = new Set([
    "if", "else", "while", "for", "do", "switch", "case", "default", "return", "break", "continue",
    "import", "struct", "enum", "const", "extern", "export", "new", "sizeof", "fn", "asm",
]);
const TYPES = new Set([
    "void", "bool", "char", "byte", "sbyte", "short", "ushort", "int", "uint", "long", "ulong", "float", "double", "string",
]);
const LITERALS = new Set(["true", "false", "null"]);
const BUILT_IN = new Set(["print", "printf", "free"]);
const TOKEN = /\/\/[^\n]*|\/\*[\s\S]*?(?:\*\/|$)|"(?:[^"\\\n]|\\.)*"?|'(?:[^'\\\n]|\\.)*'?|0[xX][0-9a-fA-F]+|\d+(?:\.\d*)?(?:[eE][+-]?\d+)?|\.\d+|[A-Za-z_]\w*|\s+|[\s\S]/gy;

export function escapeHtml(text) {
    return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

function classify(token, code, end) {
    const first = token[0];
    if (token.startsWith("//") || token.startsWith("/*")) {
        return "comment";
    }
    if (first === '"') {
        return "string";
    }
    if (first === "'") {
        return "char";
    }
    if (/[0-9.]/.test(first) && /\d/.test(token)) {
        return "number";
    }
    if (/[A-Za-z_]/.test(first)) {
        if (KEYWORDS.has(token)) {
            return "keyword";
        }
        if (TYPES.has(token)) {
            return "type";
        }
        if (LITERALS.has(token)) {
            return "literal";
        }
        const rest = code.slice(end).match(/^\s*(.)/);
        if (rest && rest[1] === "(") {
            return BUILT_IN.has(token) ? "builtin" : "function";
        }
        if (/^[A-Z][a-z]/.test(token)) {
            return "type-name";
        }
        if (/^[A-Z][A-Z0-9_]+$/.test(token)) {
            return "constant";
        }
        return null;
    }
    if (/[{}()[\];,.]/.test(first)) {
        return "punctuation";
    }
    if (/\S/.test(first)) {
        return "operator";
    }
    return null;
}

export function highlight(code) {
    let html = "";
    TOKEN.lastIndex = 0;
    let match;
    while (TOKEN.lastIndex < code.length && (match = TOKEN.exec(code))) {
        const token = match[0];
        const kind = classify(token, code, TOKEN.lastIndex);
        const escaped = escapeHtml(token);
        html += kind ? `<span class="tok-${kind}">${escaped}</span>` : escaped;
    }
    return html;
}
