// Renders a file of fairmq/ for the source browser: Shiki colours, line anchors,
// and links on the symbols that Doxygen found in the file.
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { createHighlighter } from 'shiki';
import { kids, tagOf, isText, attr, children, child, textOf, escapeHtml } from './xml.mjs';

// The colours of src/styles/code-theme.json, as classes. Unknown colours stay inline.
const COLOR_CLASS = {
  '#8d96a5': 't-c',
  '#fabe5d': 't-k',
  '#8cc8ef': 't-t',
  '#ffffff': 't-f',
  '#a8d99b': 't-s',
  '#f4a597': 't-n',
  '#e5b3d3': 't-p',
  '#c3c8d1': 't-o',
  '#e6e8ec': '',
};

let highlighter;
async function getHighlighter(theme) {
  highlighter ??= createHighlighter({ themes: [theme], langs: ['cpp'] });
  return highlighter;
}

/** Doxygen references per line: [{ start, end, refid, text }]. */
function refsByLine(fileRec) {
  const map = new Map();
  if (!fileRec) return map;
  const listing = child(fileRec.node, 'programlisting');
  for (const cl of children(listing, 'codeline')) {
    const n = Number(attr(cl, 'lineno'));
    let pos = 0;
    const refs = [];
    const walk = (node) => {
      for (const k of kids(node)) {
        if (isText(k)) pos += String(k['#text']).length;
        else if (tagOf(k) === 'sp') pos += 1;
        else if (tagOf(k) === 'ref') {
          const text = textOf(k);
          refs.push({ start: pos, end: pos + text.length, refid: attr(k, 'refid'), text });
          pos += text.length;
        } else walk(k);
      }
    };
    walk(cl);
    if (refs.length) map.set(n, refs);
  }
  return map;
}

function classFor(token) {
  const color = (token.color ?? '').toLowerCase();
  const cls = COLOR_CLASS[color];
  if (cls !== undefined) return { cls, style: '' };
  return { cls: '', style: color ? `color:${color}` : '' };
}

function span(seg) {
  const text = escapeHtml(seg.text);
  if (seg.cls) return `<span class="${seg.cls}">${text}</span>`;
  if (seg.style) return `<span style="${seg.style}">${text}</span>`;
  return text;
}

/**
 * Renders one file. Returns the rows as HTML, the number of lines, and the symbol
 * table for the hover cards: [{ k: kind, n: name, s: signature, l: location, h: href }].
 */
export async function renderSourceFile(model, filePath, { theme, repoRoot }) {
  const raw = readFileSync(path.join(repoRoot, filePath), 'utf8');
  const code = raw.replace(/\n$/, '');
  const lines = code.split('\n');
  const hl = await getHighlighter(theme);
  const tokenLines = hl.codeToTokensBase(code, { lang: 'cpp', theme: theme.name });
  const refs = refsByLine(model.files.get(filePath));

  const symbols = [];
  const symbolIndex = new Map();
  const symbolFor = (refid) => {
    if (symbolIndex.has(refid)) return symbolIndex.get(refid);
    const t = model.targets.get(refid);
    if (!t) return -1;
    const i = symbols.length;
    symbols.push({ k: t.kind, n: t.name, s: t.sig, l: t.loc, h: t.href });
    symbolIndex.set(refid, i);
    return i;
  };

  const rows = tokenLines.map((tokens, i) => {
    const n = i + 1;
    const lineText = lines[i] ?? '';
    // Keep the references whose text is at the same place in the file. Search close by
    // when Doxygen counted a character differently.
    const lineRefs = [];
    let last = 0;
    for (const r of refs.get(n) ?? []) {
      let start = lineText.slice(r.start, r.end) === r.text ? r.start : lineText.indexOf(r.text, Math.max(last, r.start - 8));
      if (start < last || start < 0) continue;
      if (symbolFor(r.refid) < 0) continue;
      lineRefs.push({ start, end: start + r.text.length, refid: r.refid });
      last = start + r.text.length;
    }

    // Split the tokens at the borders of the references.
    const segs = [];
    let pos = 0;
    for (const tok of tokens) {
      const { cls, style } = classFor(tok);
      const text = tok.content;
      const end = pos + text.length;
      const cuts = [];
      for (const r of lineRefs) {
        if (r.start > pos && r.start < end) cuts.push(r.start);
        if (r.end > pos && r.end < end) cuts.push(r.end);
      }
      cuts.sort((a, b) => a - b);
      let s0 = pos;
      for (const c of [...cuts, end]) {
        if (c > s0) segs.push({ start: s0, end: c, text: text.slice(s0 - pos, c - pos), cls, style });
        s0 = c;
      }
      pos = end;
    }

    // Emit, with an <a> around each reference and merged spans of the same colour.
    let html = '';
    let ri = 0;
    let open = null;
    let buf = null;
    const flush = () => {
      if (buf) html += span(buf);
      buf = null;
    };
    for (const seg of segs) {
      if (!open && ri < lineRefs.length && seg.start === lineRefs[ri].start) {
        flush();
        open = lineRefs[ri];
        const s = symbolFor(open.refid);
        html += `<a class="sym" href="${escapeHtml(symbols[s].h)}" data-s="${s}">`;
      }
      if (buf && buf.cls === seg.cls && buf.style === seg.style) {
        buf.text += seg.text;
        buf.end = seg.end;
      } else {
        flush();
        buf = { ...seg };
      }
      if (open && seg.end === open.end) {
        flush();
        html += '</a>';
        open = null;
        ri++;
      }
    }
    flush();
    if (open) html += '</a>';
    return `<span class="row" id="L${n}"><a class="ln" href="#L${n}" aria-label="Line ${n}">${n}</a><span class="lc">${html || ' '}</span></span>`;
  });

  return { html: rows.join(''), lines: lines.length, symbols };
}
