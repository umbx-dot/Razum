const KEY = "razum-learn-v1";

function fresh() {
    return { completed: {}, skipped: {}, code: {}, inputs: {}, last: null, theme: "auto", snippets: [], snippet: null };
}

function load() {
    try {
        return { ...fresh(), ...JSON.parse(localStorage.getItem(KEY) || "{}") };
    } catch {
        return fresh();
    }
}

let state = load();

function save() {
    try {
        localStorage.setItem(KEY, JSON.stringify(state));
    } catch {
        return;
    }
}

export const store = {
    get state() {
        return state;
    },
    isDone(id) {
        return Boolean(state.completed[id]);
    },
    isSkipped(id) {
        return Boolean(state.skipped[id]) && !state.completed[id];
    },
    complete(id) {
        state.completed[id] = Date.now();
        delete state.skipped[id];
        save();
    },
    skip(id) {
        if (!state.completed[id]) {
            state.skipped[id] = Date.now();
            save();
        }
    },
    code(id, fallback) {
        return state.code[id] ?? fallback;
    },
    setCode(id, text) {
        state.code[id] = text;
        save();
    },
    input(id, fallback) {
        return state.inputs[id] ?? fallback;
    },
    setInput(id, text) {
        state.inputs[id] = text;
        save();
    },
    resetLesson(id) {
        delete state.code[id];
        delete state.inputs[id];
        delete state.completed[id];
        delete state.skipped[id];
        save();
    },
    resetAll() {
        const { theme, snippets, snippet } = state;
        state = { ...fresh(), theme, snippets, snippet };
        save();
    },
    setLast(id) {
        state.last = id;
        save();
    },
    setTheme(theme) {
        state.theme = theme;
        save();
    },
    snippets() {
        return state.snippets;
    },
    saveSnippet(snippet) {
        const index = state.snippets.findIndex((item) => item.id === snippet.id);
        if (index >= 0) {
            state.snippets[index] = snippet;
        } else {
            state.snippets.push(snippet);
        }
        state.snippet = snippet.id;
        save();
    },
    deleteSnippet(id) {
        state.snippets = state.snippets.filter((item) => item.id !== id);
        if (state.snippet === id) {
            state.snippet = state.snippets[0]?.id ?? null;
        }
        save();
    },
    selectSnippet(id) {
        state.snippet = id;
        save();
    },
};
