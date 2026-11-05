// Sätteri hast plugin: marks short inline code with the class `nw`, so that the browser does not
// break it at a hyphen. A line break in `--print-options` or `fair::mq::Device` misleads the reader.
// Long inline code can still wrap, so that it fits on a phone.

const MAX = 32;

const textOf = (node) =>
  (node.children ?? []).map((c) => (c.type === 'text' ? c.value : textOf(c))).join('');

export default function nowrapCode() {
  return {
    name: 'fairmq-nowrap-code',
    element: {
      filter: ['code'],
      visit(node, ctx) {
        if (ctx.parent?.(node)?.tagName === 'pre') return;
        const cls = node.properties?.className ?? [];
        if ([].concat(cls).some((c) => String(c).startsWith('language-'))) return;
        if (textOf(node).length > MAX) return;
        ctx.setProperty(node, 'className', [...[].concat(cls), 'nw']);
      },
    },
  };
}
