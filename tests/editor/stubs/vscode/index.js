
class Position { constructor(line, character) { this.line = line; this.character = character; } translate(l, c) { return new Position(this.line + l, this.character + c); } }
class Range { constructor(a, b, c, d) { if (a instanceof Position) { this.start = a; this.end = b; } else { this.start = new Position(a, b); this.end = new Position(c, d); } } }
class Diagnostic { constructor(range, message) { this.range = range; this.message = message; } }
module.exports = { Position, Range, Diagnostic, DiagnosticSeverity: { Error: 0 }, Uri: {}, Location: class {}, workspace: {}, languages: {}, window: {}, commands: {} };
