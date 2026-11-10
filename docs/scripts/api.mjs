// Turns the Doxygen XML into the model for the API and source pages.
//
// The API pages show the types of the installed headers (FAIRMQ_PUBLIC_HEADER_FILES in
// fairmq/CMakeLists.txt). The source pages show all files in fairmq/. A Doxygen reference
// to a public symbol links to its API page; a reference to another symbol links to the
// line in the source browser.
import { readdirSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { parseXmlFile, tagOf, isText, kids, attr, children, child, find, textOf, escapeHtml } from './xml.mjs';

const ENTITIES = {
  ndash: '–', mdash: '—', nbsp: ' ', lsquo: '‘', rsquo: '’', ldquo: '“', rdquo: '”', hellip: '…',
  copy: '©', trade: '™', reg: '®', plusmn: '±', times: '×', divide: '÷', larr: '←', rarr: '→', harr: '↔',
  uarr: '↑', darr: '↓', lArr: '⇐', rArr: '⇒', hArr: '⇔', laquo: '«', raquo: '»', deg: '°', micro: 'µ',
  middot: '·', bull: '•', le: '≤', ge: '≥', ne: '≠', minus: '−', infin: '∞', sect: '§', para: '¶',
  apos: "'", quot: '"', zwj: '', zwnj: '', shy: '',
};

const KEYWORDS =
  /\b(template|typename|class|struct|union|enum|const|constexpr|static|virtual|explicit|inline|noexcept|override|final|void|bool|int|unsigned|signed|char|short|long|float|double|auto|using|typedef|friend|operator|default|delete|volatile|mutable|decltype|sizeof|true|false|nullptr)\b/g;

const OPERATOR_NAMES = {
  '=': 'assign', '==': 'eq', '!=': 'ne', '<': 'lt', '>': 'gt', '<=': 'le', '>=': 'ge', '<<': 'shl', '>>': 'shr',
  '[]': 'index', '()': 'call', '!': 'not', '+': 'plus', '-': 'minus', '*': 'deref', '&': 'and', '|': 'or',
  '->': 'arrow', '++': 'inc', '--': 'dec', '+=': 'plus-assign', '-=': 'minus-assign', '<=>': 'cmp',
  bool: 'bool',
};

/** The headers that `cmake --install` puts into include/fairmq. */
export function readPublicHeaders(repoRoot) {
  const text = readFileSync(path.join(repoRoot, 'fairmq', 'CMakeLists.txt'), 'utf8');
  const m = /set\(FAIRMQ_PUBLIC_HEADER_FILES([\s\S]*?)\)/.exec(text);
  if (!m) throw new Error('FAIRMQ_PUBLIC_HEADER_FILES not found in fairmq/CMakeLists.txt');
  return new Set(
    m[1]
      .split(/\s+/)
      .filter((s) => s && !s.startsWith('#') && !s.startsWith('$'))
      .map((s) => `fairmq/${s}`),
  );
}

/** All C++ files in fairmq/, sorted by directory, then by name. */
export function listSourceFiles(repoRoot) {
  const out = [];
  const walk = (rel) => {
    for (const e of readdirSync(path.join(repoRoot, rel), { withFileTypes: true })) {
      const p = `${rel}/${e.name}`;
      if (e.isDirectory()) walk(p);
      else if (/\.(h|cxx)$/.test(e.name)) out.push(p);
    }
  };
  walk('fairmq');
  return out.sort((a, b) => {
    const da = path.posix.dirname(a);
    const db = path.posix.dirname(b);
    if (da !== db) return da.localeCompare(db);
    return path.posix.basename(a).localeCompare(path.posix.basename(b), 'en', { sensitivity: 'base' });
  });
}

function slugPath(name) {
  return name.split('::').join('/');
}

/** A readable, stable anchor for a member name. */
export function anchorFor(name) {
  let n = name.trim();
  if (n.startsWith('~')) return 'dtor';
  const op = /^operator\s*(.*)$/.exec(n);
  if (op) {
    const sym = op[1].replace(/\s+/g, '');
    return 'operator-' + (OPERATOR_NAMES[sym] ?? sym.replace(/[^A-Za-z0-9_]/g, (c) => c.charCodeAt(0).toString(16)));
  }
  return n.replace(/[^A-Za-z0-9_-]/g, '-');
}

function normalizeTypeText(s) {
  return s
    .replace(/<\s+/g, '<')
    .replace(/\s+>/g, '>')
    .replace(/\s+([&*]+)/g, '$1')
    .replace(/\s+,/g, ',')
    .replace(/\(\s+/g, '(')
    .replace(/\s+\)/g, ')');
}

function highlightKeywords(escaped) {
  return escaped.replace(KEYWORDS, '<span class="k">$1</span>');
}

export function buildModel({ xmlDir, repoRoot, base = '' }) {
  const publicHeaders = readPublicHeaders(repoRoot);
  const sourceFiles = listSourceFiles(repoRoot);
  const sourceSet = new Set(sourceFiles);

  /** @type {Map<string, any>} */
  const defs = new Map();
  for (const f of readdirSync(xmlDir)) {
    if (!f.endsWith('.xml') || f === 'index.xml' || f === 'Doxyfile.xml') continue;
    const doc = find(parseXmlFile(path.join(xmlDir, f)), 'doxygen');
    for (const cd of children(doc, 'compounddef')) defs.set(attr(cd, 'id'), cd);
  }

  const compounds = new Map(); // id -> class/struct/union/namespace record
  const members = new Map(); // id -> member record
  const files = new Map(); // path -> file record
  const targets = new Map(); // refid -> { href, kind, name, sig, loc }

  // ---------- Pass 1: records ----------
  for (const [id, cd] of defs) {
    const kind = attr(cd, 'kind');
    const name = textOf(child(cd, 'compoundname')).trim();
    const loc = child(cd, 'location');
    const file = attr(loc, 'file');
    if (kind === 'file') {
      if (!sourceSet.has(file)) continue;
      files.set(file, { id, path: file, node: cd, defines: [] });
      continue;
    }
    if (kind === 'dir' || kind === 'page' || kind === 'example' || kind === 'group') continue;
    if (name.includes('@')) continue;
    const isType = kind === 'class' || kind === 'struct' || kind === 'union';
    if (!isType && kind !== 'namespace') continue;
    const parts = name.split('::');
    const rec = {
      id,
      kind,
      name,
      short: parts[parts.length - 1],
      scope: parts.slice(0, -1).join('::'),
      file,
      line: Number(attr(loc, 'line')) || 1,
      prot: attr(cd, 'prot') ?? 'public',
      node: cd,
      url: `${base}/api/${slugPath(name)}/`,
      public: false,
      members: [],
      inner: children(cd, 'innerclass').map((n) => attr(n, 'refid')),
      innerNamespaces: children(cd, 'innernamespace').map((n) => attr(n, 'refid')),
    };
    if (isType) {
      rec.public = !!file && publicHeaders.has(file) && name.startsWith('fair::mq') && rec.prot === 'public';
      const inc = child(cd, 'includes');
      rec.include = inc ? textOf(inc).trim() : file;
    }
    compounds.set(id, rec);
  }

  // Nested classes of a non-public class are not public either.
  for (const rec of compounds.values()) {
    if (rec.kind === 'namespace' || !rec.public) continue;
    const parent = [...compounds.values()].find((c) => c.name === rec.scope);
    if (parent && parent.kind !== 'namespace' && !parent.public) rec.public = false;
  }

  // Members. Namespace members also appear in file compounds with the same id; the
  // namespace (or class) is the owner.
  const ownerOrder = (cd) => (attr(cd, 'kind') === 'file' ? 1 : 0);
  const sortedDefs = [...defs.values()].sort((a, b) => ownerOrder(a) - ownerOrder(b));
  for (const cd of sortedDefs) {
    const ownerId = attr(cd, 'id');
    const owner = compounds.get(ownerId);
    const fileRec = attr(cd, 'kind') === 'file' ? files.get(attr(child(cd, 'location'), 'file')) : undefined;
    if (!owner && !fileRec) continue;
    for (const sec of children(cd, 'sectiondef')) {
      const secKind = attr(sec, 'kind');
      for (const md of children(sec, 'memberdef')) {
        const mid = attr(md, 'id');
        if (members.has(mid)) continue;
        const loc = child(md, 'location');
        const m = {
          id: mid,
          kind: attr(md, 'kind'),
          name: textOf(child(md, 'name')).trim(),
          qualified: textOf(child(md, 'qualifiedname')).trim(),
          prot: attr(md, 'prot'),
          static: attr(md, 'static') === 'yes',
          constexpr: attr(md, 'constexpr') === 'yes',
          explicit: attr(md, 'explicit') === 'yes',
          virt: attr(md, 'virt'),
          strong: attr(md, 'strong') === 'yes',
          section: secKind,
          // A namespace function that a .cxx file defines has the .cxx file as location and
          // the header as declfile. The declaration in the header is the API.
          file: attr(loc, 'declfile') || attr(loc, 'file'),
          line: Number(attr(loc, 'declline') || attr(loc, 'line')) || 1,
          owner: owner ? owner.id : null,
          fileOwner: fileRec ? fileRec.path : null,
          node: md,
        };
        if (!m.qualified) m.qualified = owner ? `${owner.name}::${m.name}` : m.name;
        members.set(mid, m);
        if (owner) owner.members.push(m);
      }
    }
  }

  // ---------- Pass 2: URLs and link targets ----------
  const sourceHref = (file, line) =>
    file && sourceSet.has(file) ? `${base}/source/${file}/${line ? `#L${line}` : ''}` : null;

  for (const f of files.values()) {
    targets.set(f.id, { href: `${base}/source/${f.path}/`, kind: 'file', name: f.path, sig: f.path, loc: f.path });
  }

  // Namespaces get a page when they hold public types or public members.
  for (const ns of compounds.values()) {
    if (ns.kind !== 'namespace') continue;
    const hasTypes = ns.inner.some((id) => compounds.get(id)?.public);
    // A name with :: is an explicit instantiation of a member template (extern template).
    const pubMembers = ns.members.filter((m) => publicHeaders.has(m.file) && m.kind !== 'define' && !m.name.includes('::'));
    ns.public = ns.name.startsWith('fair::mq') && (hasTypes || pubMembers.length > 0);
    // C++ has no access control in namespaces. Doxygen marks some functions of .cxx files private.
    for (const m of pubMembers) m.prot = 'public';
    ns.publicMembers = pubMembers;
  }

  // Anchors per owner, with numbers for overloads.
  const assignAnchors = (list) => {
    const used = new Map();
    for (const m of list) {
      const a = anchorFor(m.name);
      const n = (used.get(a) ?? 0) + 1;
      used.set(a, n);
      m.anchor = n === 1 ? a : `${a}-${n}`;
      m.overload = n;
    }
  };

  for (const rec of compounds.values()) {
    if (rec.kind === 'namespace') {
      if (!rec.public) continue;
      const list = rec.publicMembers;
      const enums = list.filter((m) => m.kind === 'enum');
      const rest = list.filter((m) => m.kind !== 'enum');
      assignAnchors(rest);
      for (const m of rest) m.href = `${rec.url}#${m.anchor}`;
      for (const e of enums) {
        e.anchor = anchorFor(e.name);
        e.page = `${rec.url}${e.name}/`;
        e.href = e.page;
      }
    } else if (rec.public) {
      const visible = rec.members.filter((m) => m.prot !== 'private');
      assignAnchors(visible);
      for (const m of visible) m.href = `${rec.url}#${m.anchor}`;
    }
  }

  const kindLabel = (m) => {
    if (m.kind === 'function') {
      const owner = m.owner && compounds.get(m.owner);
      if (owner && owner.kind !== 'namespace') {
        if (m.name === owner.short) return 'constructor';
        if (m.name === `~${owner.short}`) return 'destructor';
        return m.static ? 'static member function' : 'member function';
      }
      return 'function';
    }
    if (m.kind === 'variable') return m.owner && compounds.get(m.owner)?.kind !== 'namespace' ? 'data member' : 'variable';
    if (m.kind === 'typedef') return 'type alias';
    if (m.kind === 'enum') return m.strong ? 'enum class' : 'enum';
    return m.kind;
  };

  for (const rec of compounds.values()) {
    const href = rec.public ? rec.url : sourceHref(rec.file, rec.line);
    if (!href) continue;
    targets.set(rec.id, {
      href,
      kind: rec.kind,
      name: rec.name,
      sig: `${rec.kind} ${rec.name}`,
      loc: `${rec.file}:${rec.line}`,
    });
  }
  for (const m of members.values()) {
    const href = m.href ?? sourceHref(m.file, m.line);
    if (!href) continue;
    targets.set(m.id, { href, kind: kindLabel(m), name: m.qualified, sig: '', loc: `${m.file}:${m.line}`, member: m });
    if (m.kind === 'enum') {
      for (const ev of children(m.node, 'enumvalue')) {
        const evName = textOf(child(ev, 'name')).trim();
        targets.set(attr(ev, 'id'), {
          href: m.page ? `${m.page}#${anchorFor(evName)}` : m.href ? `${m.href}` : href,
          kind: 'enumerator',
          name: `${m.qualified}::${evName}`,
          sig: `${m.qualified}::${evName}${textOf(child(ev, 'initializer')) ? ' ' + textOf(child(ev, 'initializer')).trim() : ''}`,
          loc: `${m.file}:${m.line}`,
        });
      }
    }
  }

  // ---------- Rendering helpers ----------
  const link = (refid, innerHtml) => {
    const t = targets.get(refid);
    if (!t) return innerHtml;
    return `<a href="${escapeHtml(t.href)}">${innerHtml}</a>`;
  };

  /** Linked text (types, initializers): text and <ref> only. */
  const linked = (node, { type = false, keywords = true } = {}) => {
    if (!node) return '';
    const parts = [];
    for (const n of kids(node)) {
      if (isText(n)) {
        const raw = type ? normalizeTypeText(n['#text']) : n['#text'];
        const t = escapeHtml(raw);
        parts.push(keywords ? highlightKeywords(t) : t);
      } else if (tagOf(n) === 'ref') {
        parts.push(link(attr(n, 'refid'), escapeHtml(textOf(n))));
      } else if (tagOf(n) === 'sp') {
        parts.push(' ');
      } else {
        parts.push(linked(n, { type, keywords }));
      }
    }
    return parts.join('').replace(/\s+/g, ' ').trim();
  };

  const plain = (node, type = false) => {
    const t = textOf(node).replace(/\s+/g, ' ').trim();
    return type ? normalizeTypeText(t) : t;
  };

  const renderInline = (n) => {
    if (isText(n)) return escapeHtml(n['#text']);
    const t = tagOf(n);
    if (ENTITIES[t] !== undefined) return ENTITIES[t];
    switch (t) {
      case 'sp':
        return ' ';
      case 'ref':
        return link(attr(n, 'refid'), kids(n).map(renderInline).join(''));
      case 'computeroutput':
        return `<code>${kids(n).map(renderInline).join('')}</code>`;
      case 'emphasis':
        return `<em>${kids(n).map(renderInline).join('')}</em>`;
      case 'bold':
        return `<strong>${kids(n).map(renderInline).join('')}</strong>`;
      case 's':
      case 'strike':
      case 'del':
        return `<del>${kids(n).map(renderInline).join('')}</del>`;
      case 'underline':
      case 'ins':
        return `<u>${kids(n).map(renderInline).join('')}</u>`;
      case 'subscript':
        return `<sub>${kids(n).map(renderInline).join('')}</sub>`;
      case 'superscript':
        return `<sup>${kids(n).map(renderInline).join('')}</sup>`;
      case 'small':
        return `<small>${kids(n).map(renderInline).join('')}</small>`;
      case 'ulink':
        return `<a href="${escapeHtml(attr(n, 'url') ?? '#')}">${kids(n).map(renderInline).join('')}</a>`;
      case 'linebreak':
        return '<br>';
      case 'nonbreakablespace':
        return ' ';
      case 'anchor':
        return '';
      case 'formula':
        return `<code>${escapeHtml(textOf(n))}</code>`;
      case 'emoji':
        return attr(n, 'unicode') ?? '';
      case 'image':
      case 'htmlonly':
      case 'manonly':
      case 'xmlonly':
      case 'rtfonly':
      case 'latexonly':
      case 'docbookonly':
      case 'dot':
      case 'msc':
      case 'plantuml':
      case 'indexentry':
        return '';
      default:
        return kids(n).map(renderInline).join('');
    }
  };

  const codeOf = (n) =>
    children(n, 'codeline')
      .map((cl) => textOf(cl))
      .join('\n');

  const unwrapP = (html) => {
    const m = /^<p>([\s\S]*)<\/p>$/.exec(html);
    return m && !m[1].includes('<p>') ? m[1] : html;
  };

  /** Renders a description into HTML. Parameters, return values and notes go into `out`. */
  const renderBlocks = (nodes, out) => {
    let html = '';
    for (const n of nodes) {
      if (isText(n)) {
        if (n['#text'].trim()) html += `<p>${escapeHtml(n['#text'].trim())}</p>`;
        continue;
      }
      const t = tagOf(n);
      if (t === 'para') html += renderPara(n, out);
      else if (t === 'sect1' || t === 'sect2' || t === 'sect3' || t === 'internal') {
        const title = child(n, 'title');
        if (title) html += `<p class="doc-h">${kids(title).map(renderInline).join('')}</p>`;
        html += renderBlocks(
          kids(n).filter((k) => tagOf(k) !== 'title'),
          out,
        );
      } else html += renderPara({ para: [n] }, out);
    }
    return html;
  };

  const listItems = (n, out) =>
    children(n, 'listitem')
      .map((li) => `<li>${unwrapP(renderBlocks(kids(li), out).trim())}</li>`)
      .join('');

  const renderPara = (para, out) => {
    let html = '';
    let inline = [];
    const flush = () => {
      const text = inline.join('').replace(/\s+/g, ' ').trim();
      if (text) html += `<p>${text}</p>`;
      inline = [];
    };
    for (const n of kids(para)) {
      if (isText(n)) {
        inline.push(escapeHtml(n['#text']));
        continue;
      }
      const t = tagOf(n);
      switch (t) {
        case 'parameterlist': {
          flush();
          const kind = attr(n, 'kind');
          for (const item of children(n, 'parameteritem')) {
            const names = children(child(item, 'parameternamelist'), 'parametername').map((pn) => ({
              name: textOf(pn).trim(),
              dir: attr(pn, 'direction'),
            }));
            const desc = unwrapP(renderBlocks(kids(child(item, 'parameterdescription')), out).trim());
            for (const { name, dir } of names) {
              const entry = { name, dir, html: desc };
              if (kind === 'templateparam') out.tparams.push(entry);
              else if (kind === 'retval') out.retvals.push(entry);
              else if (kind === 'exception') out.throws.push(entry);
              else out.params.push(entry);
            }
          }
          break;
        }
        case 'simplesect': {
          flush();
          const kind = attr(n, 'kind');
          const body = renderBlocks(
            kids(n).filter((k) => tagOf(k) !== 'title'),
            out,
          ).trim();
          if (kind === 'return') out.returns = out.returns ? `${out.returns} ${unwrapP(body)}` : unwrapP(body);
          else if (kind === 'see') out.see.push(unwrapP(body));
          else if (kind === 'since') out.since = textOf(n).trim();
          else if (kind === 'note' || kind === 'remark' || kind === 'attention' || kind === 'important')
            html += `<div class="doc-aside note"><b>Note</b>${body}</div>`;
          else if (kind === 'warning') html += `<div class="doc-aside caution"><b>Caution</b>${body}</div>`;
          else if (kind === 'pre') html += `<p><strong>Precondition:</strong> ${unwrapP(body)}</p>`;
          else if (kind === 'post') html += `<p><strong>Postcondition:</strong> ${unwrapP(body)}</p>`;
          else if (kind === 'invariant') html += `<p><strong>Invariant:</strong> ${unwrapP(body)}</p>`;
          else if (kind === 'par') {
            const title = child(n, 'title');
            const label = title ? kids(title).map(renderInline).join('').trim() : '';
            html += `<div class="doc-aside note"><b>${label || 'Note'}</b>${body}</div>`;
          }
          // author, authors, version, date, copyright: not shown
          break;
        }
        case 'xrefsect': {
          flush();
          const title = textOf(child(n, 'xreftitle')).trim();
          const body = renderBlocks(kids(child(n, 'xrefdescription')), out).trim();
          if (/deprecated/i.test(title)) out.deprecated = unwrapP(body) || 'Deprecated.';
          else html += `<div class="doc-aside note"><b>${escapeHtml(title)}</b>${body}</div>`;
          break;
        }
        case 'itemizedlist':
          flush();
          html += `<ul>${listItems(n, out)}</ul>`;
          break;
        case 'orderedlist':
          flush();
          html += `<ol>${listItems(n, out)}</ol>`;
          break;
        case 'programlisting':
          flush();
          html += `<pre class="doc-code"><code>${escapeHtml(codeOf(n))}</code></pre>`;
          break;
        case 'verbatim':
        case 'preformatted':
          flush();
          html += `<pre class="doc-code"><code>${escapeHtml(textOf(n))}</code></pre>`;
          break;
        case 'heading':
          flush();
          html += `<p class="doc-h">${kids(n).map(renderInline).join('')}</p>`;
          break;
        case 'blockquote':
          flush();
          html += `<blockquote>${renderBlocks(kids(n), out)}</blockquote>`;
          break;
        case 'hruler':
          flush();
          html += '<hr>';
          break;
        case 'table': {
          flush();
          const rows = children(n, 'row')
            .map((r) => {
              const cells = children(r, 'entry')
                .map((e) => {
                  const tag = attr(e, 'thead') === 'yes' ? 'th' : 'td';
                  return `<${tag}>${unwrapP(renderBlocks(kids(e), out).trim())}</${tag}>`;
                })
                .join('');
              return `<tr>${cells}</tr>`;
            })
            .join('');
          html += `<div class="doc-table"><table>${rows}</table></div>`;
          break;
        }
        case 'variablelist': {
          flush();
          let dl = '';
          for (const k of kids(n)) {
            if (tagOf(k) === 'varlistentry') dl += `<dt>${kids(child(k, 'term')).map(renderInline).join('')}</dt>`;
            if (tagOf(k) === 'listitem') dl += `<dd>${unwrapP(renderBlocks(kids(k), out).trim())}</dd>`;
          }
          html += `<dl>${dl}</dl>`;
          break;
        }
        case 'para':
          flush();
          html += renderPara(n, out);
          break;
        default:
          inline.push(renderInline(n));
      }
    }
    flush();
    return html;
  };

  const newOut = () => ({ params: [], tparams: [], retvals: [], throws: [], returns: '', see: [], since: '', deprecated: '' });

  const describe = (node) => {
    const out = newOut();
    const briefNode = child(node, 'briefdescription');
    const brief = briefNode
      ? children(briefNode, 'para')
          .map((p) => unwrapP(renderPara(p, out)))
          .join(' ')
          .trim()
      : '';
    const detailNode = child(node, 'detaileddescription');
    const detail = detailNode ? renderBlocks(kids(detailNode), out).trim() : '';
    const documented =
      !!brief ||
      !!detail ||
      out.params.some((p) => p.html) ||
      !!out.returns ||
      out.retvals.length > 0 ||
      !!out.deprecated;
    return { brief, detail, ...out, documented };
  };

  const tparamList = (node) => {
    const tpl = child(node, 'templateparamlist');
    if (!tpl) return null;
    const ps = children(tpl, 'param').map((p) => {
      const type = linked(child(p, 'type'), { type: true });
      const decl = textOf(child(p, 'declname')).trim();
      const defval = child(p, 'defval');
      let s = type;
      if (decl && !new RegExp(`\\b${decl.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}$`).test(plain(child(p, 'type')))) s += ` ${escapeHtml(decl)}`;
      if (defval) s += ` = ${linked(defval, { type: true })}`;
      return s;
    });
    return `<span class="k">template</span>&lt;${ps.join(', ')}&gt;`;
  };

  /** The text after the parameter list in an argsstring, e.g. "const", "noexcept=default". */
  const qualifiersOf = (args) => {
    const s = args ?? '';
    const start = s.indexOf('(');
    if (start < 0) return s.trim();
    let depth = 0;
    for (let i = start; i < s.length; i++) {
      if (s[i] === '(') depth++;
      else if (s[i] === ')') {
        depth--;
        if (depth === 0) return s.slice(i + 1).trim();
      }
    }
    return '';
  };

  const formatQualifiers = (q) =>
    q
      .replace(/\s*=\s*(default|delete|0)\s*$/, ' = $1')
      .replace(/\s+/g, ' ')
      .trim();

  const signature = (m) => {
    const node = m.node;
    const name = `<b>${escapeHtml(m.name)}</b>`;
    const tpl = tparamList(node);
    const typeHtml = linked(child(node, 'type'), { type: true });
    const typeText = plain(child(node, 'type'), true);
    // Doxygen keeps some specifiers in the type text; do not repeat them.
    const typeWords = new Set(typeText.split(/\s+/).slice(0, 4));
    const pre = [];
    if (m.kind === 'friend' && !typeWords.has('friend')) pre.push('friend');
    if (m.static && !typeWords.has('static')) pre.push('static');
    if (m.constexpr && !typeWords.has('constexpr')) pre.push('constexpr');
    if (m.explicit && !typeWords.has('explicit')) pre.push('explicit');
    if ((m.virt === 'virtual' || m.virt === 'pure-virtual') && !typeWords.has('virtual')) pre.push('virtual');
    const preHtml = pre.map((p) => `<span class="k">${p}</span> `).join('');
    const argsstring = textOf(child(node, 'argsstring'));
    let body;
    let text;
    if (m.kind === 'function' || (m.kind === 'friend' && argsstring.includes('('))) {
      const params = children(node, 'param').map((p) => {
        const ptype = linked(child(p, 'type'), { type: true });
        const decl = textOf(child(p, 'declname')).trim();
        const arr = textOf(child(p, 'array')).trim();
        const defval = child(p, 'defval');
        let s = ptype;
        if (decl) s += `${ptype ? ' ' : ''}${escapeHtml(decl)}`;
        if (arr) s += escapeHtml(arr);
        if (defval) s += ` = ${linked(defval)}`;
        return s;
      });
      const paramText = children(node, 'param').map((p) => {
        let s = plain(child(p, 'type'), true);
        const decl = textOf(child(p, 'declname')).trim();
        if (decl) s += ` ${decl}`;
        if (child(p, 'defval')) s += ` = ${plain(child(p, 'defval'))}`;
        return s;
      });
      const qual = formatQualifiers(qualifiersOf(argsstring));
      const qualHtml = qual ? ' ' + highlightKeywords(escapeHtml(qual)) : '';
      const oneLine = `${pre.join(' ')} ${typeText} ${m.name}(${paramText.join(', ')}) ${qual}`.trim();
      const multi = oneLine.length > 78 && params.length > 1;
      const paramsHtml = multi ? `\n    ${params.join(',\n    ')}\n` : params.join(', ');
      // "void" as the only parameter type with no name means no parameters.
      body = `${preHtml}${typeHtml ? typeHtml + ' ' : ''}${name}(${paramsHtml})${qualHtml}`;
      text = `${pre.length ? pre.join(' ') + ' ' : ''}${typeText ? typeText + ' ' : ''}${m.name}(${paramText.join(', ')})${qual ? ' ' + qual : ''}`;
    } else if (m.kind === 'variable') {
      const init = child(node, 'initializer');
      const initHtml = init ? ' ' + linked(init) : '';
      body = `${preHtml}${typeHtml} ${name}${escapeHtml(argsstring)}${initHtml}`;
      text = `${pre.join(' ')} ${typeText} ${m.name}${argsstring}${init ? ' ' + plain(init) : ''}`.trim();
    } else if (m.kind === 'typedef') {
      const def = textOf(child(node, 'definition')).trim();
      if (def.startsWith('using')) {
        body = `<span class="k">using</span> ${name} = ${typeHtml}`;
        text = `using ${m.name} = ${typeText}`;
      } else {
        body = `<span class="k">typedef</span> ${typeHtml} ${name}${escapeHtml(argsstring)}`;
        text = `typedef ${typeText} ${m.name}${argsstring}`;
      }
    } else if (m.kind === 'enum') {
      const under = typeHtml ? ` : ${typeHtml}` : '';
      body = `<span class="k">enum</span>${m.strong ? ' <span class="k">class</span>' : ''} ${name}${under}`;
      text = `enum${m.strong ? ' class' : ''} ${m.name}${typeText ? ' : ' + typeText : ''}`;
    } else if (m.kind === 'friend') {
      body = `<span class="k">friend</span> ${typeHtml} ${name}`;
      text = `friend ${typeText} ${m.name}`;
    } else if (m.kind === 'define') {
      body = `<span class="k">#define</span> ${name}${escapeHtml(argsstring)}`;
      text = `#define ${m.name}${argsstring}`;
    } else {
      body = `${typeHtml} ${name}${escapeHtml(argsstring)}`;
      text = `${typeText} ${m.name}${argsstring}`;
    }
    return { html: tpl ? `${tpl}\n${body}` : body, text: text.replace(/\s+/g, ' ').trim(), template: !!tpl };
  };

  // ---------- Pass 3: render public records ----------
  for (const m of members.values()) {
    const sig = signature(m);
    m.sigHtml = sig.html;
    m.sigText = sig.text;
    m.template = sig.template;
    const t = targets.get(m.id);
    if (t) t.sig = sig.text;
  }

  const renderMember = (m) => {
    Object.assign(m, describe(m.node));
    if (m.kind === 'enum') {
      m.values = children(m.node, 'enumvalue').map((ev) => {
        const out = newOut();
        const b = child(ev, 'briefdescription');
        const d = child(ev, 'detaileddescription');
        const brief = b ? children(b, 'para').map((p) => unwrapP(renderPara(p, out))).join(' ').trim() : '';
        const detail = d ? renderBlocks(kids(d), out).trim() : '';
        return {
          name: textOf(child(ev, 'name')).trim(),
          anchor: anchorFor(textOf(child(ev, 'name')).trim()),
          init: plain(child(ev, 'initializer')).replace(/^=\s*/, ''),
          html: [brief, unwrapP(detail)].filter(Boolean).join(' '),
        };
      });
    }
  };

  for (const rec of compounds.values()) {
    if (!rec.public) continue;
    Object.assign(rec, describe(rec.node));
    rec.templ = tparamList(rec.node);
    rec.final = attr(rec.node, 'final') === 'yes';
    rec.abstract = attr(rec.node, 'abstract') === 'yes';
    rec.bases = children(rec.node, 'basecompoundref').map((b) => ({
      html: attr(b, 'refid') ? link(attr(b, 'refid'), escapeHtml(textOf(b).trim())) : escapeHtml(textOf(b).trim()),
      prot: attr(b, 'prot'),
      virt: attr(b, 'virt'),
    }));
    rec.derived = children(rec.node, 'derivedcompoundref').map((b) => ({
      html: attr(b, 'refid') ? link(attr(b, 'refid'), escapeHtml(textOf(b).trim())) : escapeHtml(textOf(b).trim()),
      id: attr(b, 'refid'),
    }));
    const list = rec.kind === 'namespace' ? rec.publicMembers : rec.members.filter((m) => m.prot !== 'private');
    for (const m of list) renderMember(m);
    rec.visible = list;
  }

  // Which types and members each file defines, for the source pages.
  for (const rec of compounds.values()) {
    if (rec.kind !== 'namespace' && files.has(rec.file)) files.get(rec.file).defines.push(rec);
  }

  return {
    base,
    publicHeaders,
    sourceFiles,
    compounds,
    members,
    files,
    targets,
    link,
    /** Public classes, structs and unions. */
    types: () => [...compounds.values()].filter((c) => c.public && c.kind !== 'namespace'),
    /** Namespaces with a page. */
    namespaces: () => [...compounds.values()].filter((c) => c.public && c.kind === 'namespace'),
    /** Enums at namespace scope, which get their own pages. */
    namespaceEnums: () =>
      [...compounds.values()]
        .filter((c) => c.public && c.kind === 'namespace')
        .flatMap((ns) => ns.visible.filter((m) => m.kind === 'enum').map((e) => ({ ns, e }))),
    compoundByName: (name) => [...compounds.values()].find((c) => c.name === name),
  };
}

let cached;
let cachedKey;
/** The model, built once per build. */
export function getModel(opts) {
  const key = JSON.stringify(opts);
  if (!cached || cachedKey !== key) {
    cached = buildModel(opts);
    cachedKey = key;
  }
  return cached;
}
