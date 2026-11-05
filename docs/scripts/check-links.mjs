// Checks all internal links and anchors in the built site (dist/).
//   node scripts/check-links.mjs [distDir]
// Exit code 1 if a link points to a missing page or a missing anchor.
import { readFileSync, readdirSync, statSync, existsSync } from 'node:fs';
import path from 'node:path';

const dist = path.resolve(process.argv[2] ?? new URL('../dist', import.meta.url).pathname);
const base = '/FairMQ';

function walk(dir, out = []) {
  for (const name of readdirSync(dir)) {
    const p = path.join(dir, name);
    if (statSync(p).isDirectory()) walk(p, out);
    else if (name.endsWith('.html')) out.push(p);
  }
  return out;
}

const pages = walk(dist);
const ids = new Map();
const links = [];
const idRe = /\sid="([^"]+)"/g;
const linkRe = /\s(href|src)="([^"]+)"/g;
for (const file of pages) {
  const html = readFileSync(file, 'utf8');
  ids.set(file, new Set([...html.matchAll(idRe)].map((m) => m[1])));
  // Skip the content of <script> blocks: their strings are not links.
  const text = html.replace(/<script\b[^>]*>[\s\S]*?<\/script>/g, '');
  for (const m of text.matchAll(linkRe)) links.push({ file, attr: m[1], url: m[2].replace(/&amp;/g, '&') });
}

const resolve = (url) => {
  const p = decodeURIComponent(url.slice(base.length)) || '/';
  const candidates = [path.join(dist, p), path.join(dist, p, 'index.html'), path.join(dist, p + '.html')];
  return candidates.find((c) => existsSync(c) && statSync(c).isFile());
};

const bad = [];
for (const { file, url } of links) {
  if (/^(https?:|mailto:|data:|javascript:)/.test(url) || url.startsWith('//')) continue;
  const from = '/' + path.relative(dist, file);
  let [target, hash] = url.split('#');
  if (!target) {
    if (hash && !ids.get(file).has(hash)) bad.push(`${from}: missing anchor #${hash}`);
    continue;
  }
  if (!target.startsWith('/')) {
    bad.push(`${from}: relative link ${url}`);
    continue;
  }
  if (!target.startsWith(base + '/') && target !== base) {
    bad.push(`${from}: link without the base ${url}`);
    continue;
  }
  const t = resolve(target.split('?')[0]);
  if (!t) {
    bad.push(`${from}: missing page ${url}`);
    continue;
  }
  if (hash && t.endsWith('.html') && !ids.get(t)?.has(hash) && !/^L\d+(-L\d+)?$/.test(hash)) bad.push(`${from}: missing anchor ${url}`);
  if (hash && /^L(\d+)(?:-L(\d+))?$/.test(hash)) {
    const [, a, b] = /^L(\d+)(?:-L(\d+))?$/.exec(hash);
    for (const n of [a, b].filter(Boolean)) if (!ids.get(t)?.has('L' + n)) bad.push(`${from}: missing line ${url}`);
  }
}

const unique = [...new Set(bad)];
console.log(`Checked ${links.length} links in ${pages.length} pages.`);
if (unique.length) {
  console.log(unique.slice(0, 200).join('\n'));
  if (unique.length > 200) console.log(`… and ${unique.length - 200} more`);
  console.log(`${unique.length} broken links.`);
  process.exit(1);
}
console.log('All links are valid.');
