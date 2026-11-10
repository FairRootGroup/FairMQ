// Takes lines out of a file of the repository for the Snippet component and the page check.
// The options select a region, a line range, or a start line (and an end line). See Snippet.astro.
import { readFileSync, existsSync } from 'node:fs';
import path from 'node:path';

/** Returns { code, first, last } or throws an Error that names the problem. */
export function extractSnippet(repoRoot, { file, region, lines, start, end }) {
  const abs = path.join(repoRoot, file);
  if (!existsSync(abs)) throw new Error(`Snippet: file not found: ${file}`);
  const all = readFileSync(abs, 'utf8').replace(/\r\n/g, '\n').split('\n');
  let first = 1;
  let body;
  if (region) {
    const marker = new RegExp(`^\\s*(//!|#!|//) \\[${region.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}\\]\\s*$`);
    const idx = all.flatMap((l, i) => (marker.test(l) ? [i] : []));
    if (idx.length < 2) throw new Error(`Snippet: region "${region}" not found twice in ${file}`);
    body = all.slice(idx[0] + 1, idx[1]);
    first = idx[0] + 2;
  } else if (start) {
    const matcher = (pat) => {
      const re = /^\/(.+)\/$/.exec(pat);
      if (re) {
        const rx = new RegExp(re[1]);
        return (l) => rx.test(l);
      }
      return (l) => l.includes(pat);
    };
    const isStart = matcher(start);
    const a = all.findIndex(isStart);
    if (a < 0) throw new Error(`Snippet: no line with "${start}" in ${file}`);
    let b = -1;
    if (end) {
      const isEnd = end.startsWith('^') ? (l) => new RegExp(end).test(l) : matcher(end);
      b = all.findIndex((l, i) => i > a && isEnd(l));
      if (b < 0) throw new Error(`Snippet: no line with "${end}" after "${start}" in ${file}`);
    } else {
      // Up to the line that closes the first brace at or after the start line.
      let depth = 0;
      let open = false;
      for (let i = a; i < all.length && b < 0; i++) {
        const code = all[i].replace(/\/\/.*$/, '').replace(/"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'/g, '');
        for (const c of code) {
          if (c === '{') {
            depth++;
            open = true;
          } else if (c === '}') depth--;
        }
        if (open && depth <= 0) b = i;
      }
      if (b < 0) throw new Error(`Snippet: no closing brace after "${start}" in ${file}`);
    }
    body = all.slice(a, b + 1);
    first = a + 1;
  } else if (lines) {
    const m = /^(\d+)-(\d+)$/.exec(lines);
    if (!m) throw new Error(`Snippet: bad lines "${lines}" for ${file}`);
    const a = Number(m[1]);
    const b = Number(m[2]);
    if (b > all.length || a < 1 || a > b) throw new Error(`Snippet: lines ${lines} out of range for ${file} (${all.length} lines)`);
    body = all.slice(a - 1, b);
    first = a;
  } else {
    // Drop the license block at the top and the blank lines after it.
    let i = 0;
    if (/^\s*\/\*{5,}/.test(all[0] ?? '')) {
      while (i < all.length && !/\*\/\s*$/.test(all[i])) i++;
      i++;
    } else {
      while (i < all.length && /^\s*#/.test(all[i]) && /copyright|license|\*{5,}|^#\s*$/i.test(all[i])) i++;
    }
    while (i < all.length && all[i].trim() === '') i++;
    body = all.slice(i);
    first = i + 1;
  }
  // Remove nested markers and the common indentation.
  body = body.filter((l) => !/^\s*(\/\/!|#!) \[[\w-]+\]\s*$/.test(l));
  while (body.length && body[body.length - 1].trim() === '') body.pop();
  const indent = Math.min(...body.filter((l) => l.trim()).map((l) => (l.match(/^ */) ?? [''])[0].length));
  if (Number.isFinite(indent) && indent > 0) body = body.map((l) => l.slice(indent));

  return { code: body.join('\n'), first, last: first + body.length - 1 };
}
