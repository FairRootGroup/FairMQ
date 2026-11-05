// Small helpers around fast-xml-parser in `preserveOrder` mode.
// A node is either { '#text': string } or { tag: [children], ':@': { attributes } }.
import { readFileSync } from 'node:fs';
import { XMLParser } from 'fast-xml-parser';

const parser = new XMLParser({
  preserveOrder: true,
  ignoreAttributes: false,
  attributeNamePrefix: '',
  trimValues: false,
  parseTagValue: false,
  parseAttributeValue: false,
  processEntities: true,
  htmlEntities: true,
});

export function parseXmlFile(file) {
  return parser.parse(readFileSync(file, 'utf8'));
}

export function tagOf(node) {
  for (const k of Object.keys(node)) if (k !== ':@') return k;
  return undefined;
}

export function isText(node) {
  return Object.prototype.hasOwnProperty.call(node, '#text');
}

export function kids(node) {
  if (!node) return [];
  const t = tagOf(node);
  return t && t !== '#text' && Array.isArray(node[t]) ? node[t] : [];
}

export function attr(node, name) {
  return node && node[':@'] ? node[':@'][name] : undefined;
}

export function children(node, name) {
  return kids(node).filter((n) => tagOf(n) === name);
}

export function child(node, name) {
  return kids(node).find((n) => tagOf(n) === name);
}

export function find(nodes, name) {
  for (const n of nodes) if (tagOf(n) === name) return n;
  return undefined;
}

/** Plain text of a node, with Doxygen's <sp/> as spaces. */
export function textOf(node) {
  if (!node) return '';
  if (isText(node)) return String(node['#text']);
  const t = tagOf(node);
  if (t === 'sp') return ' ';
  return kids(node).map(textOf).join('');
}

export function escapeHtml(s) {
  return String(s)
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;');
}
