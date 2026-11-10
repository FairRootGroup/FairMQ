// Sätteri hast plugin: prefixes root-relative links and images in Markdown and MDX
// with the site base, so that pages can link to `/concepts/channels/`
// instead of `/FairMQ/concepts/channels/`.

function prefix(url, base) {
  if (typeof url !== 'string' || !url.startsWith('/') || url.startsWith('//')) return url;
  if (url === base || url.startsWith(base + '/')) return url;
  return base + url;
}

export default function baseLinks({ base }) {
  const b = base.replace(/\/$/, '');
  return {
    name: 'fairmq-base-links',
    element: {
      filter: ['a', 'img'],
      visit(node, ctx) {
        if (!b) return;
        const key = node.tagName === 'a' ? 'href' : 'src';
        const value = node.properties?.[key];
        const next = prefix(value, b);
        if (next !== value) ctx.setProperty(node, key, next);
      },
    },
  };
}
