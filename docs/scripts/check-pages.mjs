// Fast checks of the docs pages, without a build: frontmatter, MDX syntax, imports, links,
// anchors of API and source pages, and <Snippet> ranges.
//   node scripts/check-pages.mjs                      all pages
//   node scripts/check-pages.mjs src/content/docs/concepts/states.mdx …
// Exit code 1 if a page has an error. Run `npm run build` and `npm run check` for the full check.
import { readFileSync, readdirSync, statSync, existsSync } from 'node:fs';
import path from 'node:path';
import { parse as parseYaml } from 'yaml';
import { mdxToJs } from 'satteri';
import GithubSlugger from 'github-slugger';
import { getModel } from './api.mjs';
import { extractSnippet } from './snippet.mjs';
import { deviceOptions, readOptions } from './options.mjs';

const docsDir = path.resolve(new URL('..', import.meta.url).pathname);
const repoRoot = path.resolve(docsDir, '..');
const contentDir = path.join(docsDir, 'src/content/docs');

const walk = (dir, out = []) => {
  for (const name of readdirSync(dir)) {
    const p = path.join(dir, name);
    if (statSync(p).isDirectory()) walk(p, out);
    else if (/\.mdx?$/.test(name)) out.push(p);
  }
  return out;
};
const allPages = walk(contentDir);
const files = process.argv.slice(2).length ? process.argv.slice(2).map((f) => path.resolve(f)) : allPages;

const model = getModel({ xmlDir: path.join(docsDir, 'build/doxygen/xml'), repoRoot, base: '' });
const api = new Map();
const addAnchors = (url, recs) => {
  if (!api.has(url)) api.set(url, new Set());
  for (const x of recs) if (x?.anchor) api.get(url).add(x.anchor);
};
for (const t of model.types()) addAnchors(t.url, t.visible ?? []);
for (const ns of model.namespaces()) addAnchors(ns.url, ns.visible ?? []);
for (const { e } of model.namespaceEnums()) addAnchors(e.page, e.values ?? []);
api.set('/api/', new Set());
const sources = new Set(model.sourceFiles);
const lineCount = (f) => readFileSync(path.join(repoRoot, f), 'utf8').replace(/\n$/, '').split('\n').length;

const pageUrl = (abs) => {
  const rel = path.relative(contentDir, abs).replace(/\.mdx?$/, '');
  return rel === 'index' ? '/' : '/' + rel.replace(/\/index$/, '') + '/';
};
const pages = new Map(allPages.map((p) => [pageUrl(p), p]));
const slugCache = new Map();
const slugsOf = (abs) => {
  if (slugCache.has(abs)) return slugCache.get(abs);
  const src = readFileSync(abs, 'utf8').replace(/```[\s\S]*?```/g, '');
  const slugger = new GithubSlugger();
  const set = new Set(['_top']);
  for (const m of src.matchAll(/^#{2,6}\s+(.+?)\s*$/gm)) set.add(slugger.slug(m[1].replace(/`/g, '').replace(/\[([^\]]+)\]\([^)]*\)/g, '$1')));
  for (const m of src.matchAll(/\sid="([^"]+)"/g)) set.add(m[1]);
  // Rows of <OptionsTable />: #<id>-<option>, id "opt" by default.
  for (const m of src.matchAll(/<OptionsTable\b([^>]*?)\/>/g)) {
    const attrs = m[1];
    const file = /\bfile="([^"]+)"/.exec(attrs)?.[1];
    const id = /\bid="([^"]+)"/.exec(attrs)?.[1] ?? 'opt';
    const groups = /\bgroups=\{\[([^\]]*)\]\}/.exec(attrs)?.[1].match(/'[^']*'|"[^"]*"/g)?.map((g) => g.slice(1, -1));
    let options = file ? readOptions(repoRoot, [{ file, source: path.basename(file), group: 'Options' }]) : deviceOptions(repoRoot);
    if (groups) options = options.filter((o) => groups.includes(o.group));
    for (const o of options) set.add(`${id}-${o.name}`);
  }
  slugCache.set(abs, set);
  return set;
};

let errors = 0;
let warnings = 0;
const report = (file, line, msg, warn = false) => {
  if (warn) warnings++;
  else errors++;
  console.log(`${path.relative(docsDir, file)}:${line}: ${warn ? 'warning' : 'error'}: ${msg}`);
};
const lineOf = (src, index) => src.slice(0, index).split('\n').length;

function checkLink(file, src, index, url) {
  const line = lineOf(src, index);
  if (url.startsWith('/FairMQ/')) return report(file, line, `remove the base /FairMQ from the link ${url}`);
  const [p, hash] = url.split('#');
  if (!p.endsWith('/')) return report(file, line, `the link ${url} must end with a slash before the anchor`);
  if (p.startsWith('/api/')) {
    if (!api.has(p)) return report(file, line, `no API page ${p} (see node scripts/print-api.mjs)`);
    if (hash && !api.get(p).has(hash)) report(file, line, `no anchor #${hash} on ${p} (see node scripts/print-api.mjs <Name> --members)`);
    return;
  }
  if (p.startsWith('/source/')) {
    const f = p.slice('/source/'.length, -1);
    if (!f) return;
    if (!sources.has(f)) return report(file, line, `no source page for ${f}`);
    if (hash) {
      const m = /^L(\d+)(?:-L(\d+))?$/.exec(hash);
      if (!m) return report(file, line, `use #L<n> or #L<a>-L<b> on source pages, not #${hash}`);
      const n = lineCount(f);
      if (Number(m[2] ?? m[1]) > n) report(file, line, `${f} has only ${n} lines`);
    }
    return;
  }
  const target = pages.get(p);
  if (!target) return report(file, line, `no page ${p}`);
  if (hash && !slugsOf(target).has(hash)) report(file, line, `no heading #${hash} on ${p}`);
}

for (const file of files) {
  if (!existsSync(file)) {
    report(file, 0, 'file not found');
    continue;
  }
  const src = readFileSync(file, 'utf8');
  const fmMatch = /^---\n([\s\S]*?)\n---\n/.exec(src);
  if (!fmMatch) {
    report(file, 1, 'no frontmatter');
    continue;
  }
  let fm;
  try {
    fm = parseYaml(fmMatch[1]) ?? {};
  } catch (e) {
    report(file, 1, `frontmatter: ${e.message}`);
    continue;
  }
  if (!fm.title) report(file, 1, 'frontmatter: no title');
  if (!fm.description) report(file, 1, 'frontmatter: no description');
  for (const c of fm.code ?? []) if (!sources.has(c)) report(file, 1, `frontmatter code: ${c} is not a page of the source browser (fairmq/**/*.h or .cxx)`);
  for (const g of fm.guides ?? []) if (!g?.label || !g?.link) report(file, 1, 'frontmatter guides: each entry needs label and link');
  for (const g of fm.guides ?? []) if (g?.link) checkLink(file, src, 0, g.link);
  if (fm.hero?.actions) for (const a of fm.hero.actions) if (a.link?.startsWith('/FairMQ/')) checkLink(file, src, 0, a.link.slice('/FairMQ'.length));

  const fmLines = fmMatch[0].split('\n').length - 1;
  const body = src.slice(fmMatch[0].length);
  if (file.endsWith('.mdx')) {
    try {
      await mdxToJs(body);
    } catch (e) {
      const m = /^(\d+):(\d+)/.exec(e.message);
      report(file, m ? Number(m[1]) + fmLines : fmLines, `MDX: ${e.message}`);
    }
  }
  for (const m of src.matchAll(/^import\s+[\s\S]*?\s+from\s+['"]([^'"]+)['"]/gm)) {
    const spec = m[1];
    if (spec.startsWith('.') && !existsSync(path.resolve(path.dirname(file), spec))) report(file, lineOf(src, m.index), `import not found: ${spec}`);
  }
  const noCode = src.replace(/```[\s\S]*?```/g, (s) => s.replace(/[^\n]/g, ' ')).replace(/`[^`\n]*`/g, (s) => ' '.repeat(s.length));
  if (file.endsWith('.mdx')) {
    // MDX fails at render time for a component without an import.
    const imported = new Set();
    for (const m of src.matchAll(/^import\s+([\s\S]*?)\s+from\s+['"][^'"]+['"]/gm)) {
      for (const n of m[1].matchAll(/[A-Za-z_$][\w$]*/g)) imported.add(n[0]);
    }
    const missing = new Set();
    for (const m of noCode.matchAll(/<([A-Z][\w$]*)[\s/>]/g)) {
      if (!imported.has(m[1]) && !missing.has(m[1])) {
        missing.add(m[1]);
        report(file, lineOf(src, m.index), `component <${m[1]}> has no import`);
      }
    }
  }
  for (const m of noCode.matchAll(/\]\(([^)\s]+)(?:\s+"[^"]*")?\)/g)) {
    const url = m[1];
    if (/^(https?:|mailto:|#)/.test(url)) continue;
    if (!url.startsWith('/')) report(file, lineOf(src, m.index), `use a root-relative link like /concepts/states/, not ${url}`);
    else checkLink(file, src, m.index, url);
  }
  for (const m of noCode.matchAll(/\shref=["'](\/[^"']*)["']/g)) checkLink(file, src, m.index, m[1]);
  for (const m of src.matchAll(/<Snippet\b([\s\S]*?)\/>/g)) {
    const attrs = Object.fromEntries([...m[1].matchAll(/(\w+)=(?:"([^"]*)"|'([^']*)')/g)].map((a) => [a[1], a[2] ?? a[3]]));
    if (/\{/.test(m[1].replace(/(\w+)=(?:"[^"]*"|'[^']*')/g, ''))) continue; // props with expressions: checked by the build
    try {
      extractSnippet(repoRoot, attrs);
    } catch (e) {
      report(file, lineOf(src, m.index), e.message);
    }
  }
  for (const m of noCode.matchAll(/\b(simply|just|easily|obviously|basically|very|really|please)\b/gi))
    report(file, lineOf(src, m.index), `style: avoid "${m[1]}"`, true);
  if (/This page is a stub\./.test(src)) report(file, 1, 'the page is still a stub', true);
}
console.log(`${files.length} pages: ${errors} errors, ${warnings} warnings.`);
process.exit(errors ? 1 : 0);
