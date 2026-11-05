// Shared access to the API model for the generated pages.
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { paths, build } from 'virtual:fairmq/data';
import { getModel } from '../../scripts/api.mjs';

export const base = import.meta.env.BASE_URL.replace(/\/$/, '');

export function model() {
  return getModel({ xmlDir: paths.xmlDir, repoRoot: paths.repoRoot, base });
}

let theme;
export function codeTheme() {
  theme ??= JSON.parse(readFileSync(path.join(paths.docsDir, 'src/styles/code-theme.json'), 'utf8'));
  return theme;
}

/** A path without the site base, for Starlight sidebar links (Starlight adds the base). */
export const noBase = (href) => (href.startsWith(base + '/') ? href.slice(base.length) : href);

/** GitHub link to a file and line at the commit of the build. */
export function githubUrl(file, line) {
  const ref = build.commit && build.commit !== 'unknown' ? build.commit : 'dev';
  return `${build.repo}/blob/${ref}/${file}${line ? `#L${line}` : ''}`;
}

const KIND_LETTER = { class: 'C', struct: 'S', union: 'U', enum: 'E', namespace: 'N' };

/** Sidebar for the API pages: one group per namespace with its types and enums. */
export function apiSidebar() {
  const m = model();
  const groups = [];
  const namespaces = m.namespaces().sort((a, b) => a.name.localeCompare(b.name));
  for (const ns of namespaces) {
    const inNs = (t) =>
      t.name.startsWith(ns.name + '::') &&
      !namespaces.some((o) => o !== ns && o.name.length > ns.name.length && t.name.startsWith(o.name + '::'));
    const entries = [
      ...m
        .types()
        .filter(inNs)
        .map((t) => ({ rel: t.name.slice(ns.name.length + 2), link: noBase(t.url), kind: KIND_LETTER[t.kind] })),
      ...ns.visible.filter((e) => e.kind === 'enum').map((e) => ({ rel: e.name, link: noBase(e.page), kind: 'E' })),
    ];
    // Nested types follow their parent, indented.
    const key = (rel) => rel.split('::').map((s) => s.toLowerCase());
    entries.sort((a, b) => {
      const ka = key(a.rel);
      const kb = key(b.rel);
      for (let i = 0; i < Math.min(ka.length, kb.length); i++) if (ka[i] !== kb[i]) return ka[i] < kb[i] ? -1 : 1;
      return ka.length - kb.length;
    });
    const items = entries.map((e) => {
      const depth = e.rel.split('::').length - 1;
      return {
        label: e.rel.split('::').pop(),
        link: e.link,
        attrs: { 'data-kind': e.kind, class: depth ? 'fmq-mono fmq-nested' : 'fmq-mono', title: `${ns.name}::${e.rel}` },
      };
    });
    groups.push({
      label: ns.name,
      collapsed: ns.name !== 'fair::mq',
      items: [{ label: 'Namespace overview', link: noBase(ns.url), attrs: { 'data-kind': 'N' } }, ...items],
    });
  }
  return [{ label: 'API reference', link: '/api/' }, ...groups];
}

/** Sidebar for the source pages: one group per directory. */
export function sourceSidebar() {
  const m = model();
  const dirs = new Map();
  for (const f of m.sourceFiles) {
    const dir = path.posix.dirname(f);
    if (!dirs.has(dir)) dirs.set(dir, []);
    dirs.get(dir).push({ label: path.posix.basename(f), link: `/source/${f}/`, attrs: { class: 'fmq-mono' } });
  }
  return [
    { label: 'Source browser', link: '/source/' },
    ...[...dirs].map(([dir, items]) => ({ label: `${dir}/`, collapsed: dir !== 'fairmq', items })),
  ];
}

/** Docs pages that name a file in their `code` front matter. A page that names Foo.cxx also
 *  describes Foo.h, and the other way round. */
export function guidesForFile(entries, file) {
  const stem = (f) => f.replace(/\.(h|cxx)$/, '');
  return entries
    .filter((e) => (e.data.code ?? []).some((c) => c === file || stem(c) === stem(file)))
    .map((e) => ({ label: e.data.title, link: `/${e.id}/` }));
}
