import { highlight, escapeHtml } from "./highlight.js";

export function slugify(text) {
    return text
        .toLowerCase()
        .replace(/`/g, "")
        .replace(/<[^>]*>/g, "")
        .replace(/[^\w\s-]/g, "")
        .trim()
        .replace(/\s+/g, "-");
}

function formatText(text, renderCode) {
    return text.split(/(`[^`]+`)/).map((part) => {
        if (part.startsWith("`") && part.endsWith("`") && part.length > 1) {
            return `<code>${renderCode(part.slice(1, -1))}</code>`;
        }
        return escapeHtml(part)
            .replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
            .replace(/\*([^*]+)\*/g, "<em>$1</em>");
    }).join("");
}

function inline(text, resolveLink, renderCode = escapeHtml) {
    const pattern = /(`[^`]+`)|\[([^\]]+)\]\(([^)\s]+)\)/g;
    let html = "";
    let last = 0;
    let match;
    while ((match = pattern.exec(text))) {
        html += formatText(text.slice(last, match.index), renderCode);
        if (match[1]) {
            html += `<code>${renderCode(match[1].slice(1, -1))}</code>`;
        } else {
            const label = formatText(match[2], renderCode);
            const target = resolveLink ? resolveLink(match[3]) : match[3];
            if (target) {
                const external = /^(https?:|mailto:)/.test(target);
                html += `<a href="${escapeHtml(target)}"${external ? ' target="_blank" rel="noopener"' : ""}>${label}</a>`;
            } else {
                html += label;
            }
        }
        last = pattern.lastIndex;
    }
    return html + formatText(text.slice(last), renderCode);
}

export function tableCells(line) {
    return line.replace(/\\\|/g, "\uE000")
        .replace(/^\||\|$/g, "")
        .split("|")
        .map((cell) => cell.replace(/\uE000/g, "|").trim());
}

export function tableApi(line) {
    if (!line.startsWith("|")) {
        return null;
    }
    const [signature] = tableCells(line);
    const names = Array.from(signature.matchAll(/`[^`]*?\b([a-zA-Z_]\w*)\s*\([^`]*`/g), (match) => match[1]);
    return names.length ? { names, anchor: `api-${names[0].toLowerCase()}` } : null;
}

function table(lines, resolveLink, tableAnchors) {
    const rows = lines.filter((line) => !/^\|\s*-/.test(line));
    if (rows.length === 0) {
        return "";
    }
    const [head, ...body] = rows;
    const headings = tableCells(head).map((cell) => `<th scope="col">${inline(cell, resolveLink, highlight)}</th>`);
    const contents = body.map((row) => {
        const anchor = tableAnchors && tableApi(row)?.anchor;
        const id = anchor && !tableAnchors.has(anchor) ? ` id="${anchor}"` : "";
        if (anchor) {
            tableAnchors.add(anchor);
        }
        const cells = tableCells(row).map((cell) => `<td>${inline(cell, resolveLink, highlight)}</td>`);
        return `<tr${id}>${cells.join("")}</tr>`;
    });
    return `<table><thead><tr>${headings.join("")}</tr></thead><tbody>${contents.join("")}</tbody></table>`;
}

export function renderMarkdown(source, options = {}) {
    const resolveLink = options.link;
    const tableAnchors = options.tableAnchors ? new Set() : null;
    const lines = source.replace(/\r/g, "").split("\n");
    const html = [];
    let index = 0;
    while (index < lines.length) {
        const line = lines[index];
        if (line.startsWith("```")) {
            const language = line.slice(3).trim();
            const code = [];
            index++;
            while (index < lines.length && !lines[index].startsWith("```")) {
                code.push(lines[index]);
                index++;
            }
            index++;
            const text = code.join("\n");
            const body = language === "c" || language === "rz" ? highlight(text) : escapeHtml(text);
            html.push(`<pre class="code-block${language === "c" || language === "rz" ? " rz" : ""}"><code>${body}</code></pre>`);
            continue;
        }
        if (/^#{1,6} /.test(line)) {
            const level = line.match(/^#+/)[0].length;
            const text = line.slice(level + 1);
            html.push(`<h${level} id="${slugify(text)}">${inline(text, resolveLink)}</h${level}>`);
            index++;
            continue;
        }
        if (line.startsWith("|")) {
            const block = [];
            while (index < lines.length && lines[index].startsWith("|")) {
                block.push(lines[index]);
                index++;
            }
            html.push(table(block, resolveLink, tableAnchors));
            continue;
        }
        if (line.startsWith("> ")) {
            const block = [];
            while (index < lines.length && lines[index].startsWith("> ")) {
                block.push(lines[index].slice(2));
                index++;
            }
            html.push(`<div class="note">${inline(block.join(" "), resolveLink)}</div>`);
            continue;
        }
        if (/^(- |\d+\. )/.test(line)) {
            const ordered = /^\d+\. /.test(line);
            const items = [];
            while (index < lines.length && /^(- |\d+\. )/.test(lines[index])) {
                items.push(`<li>${inline(lines[index].replace(/^(- |\d+\. )/, ""), resolveLink)}</li>`);
                index++;
            }
            html.push(ordered ? `<ol>${items.join("")}</ol>` : `<ul>${items.join("")}</ul>`);
            continue;
        }
        if (line.trim() === "") {
            index++;
            continue;
        }
        const paragraph = [];
        while (index < lines.length && lines[index].trim() !== "" && !/^(```|#{1,6} |\||> |- |\d+\. )/.test(lines[index])) {
            paragraph.push(lines[index]);
            index++;
        }
        html.push(`<p>${inline(paragraph.join(" "), resolveLink)}</p>`);
    }
    return html.join("\n");
}
