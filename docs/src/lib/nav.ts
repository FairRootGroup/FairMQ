// Sections of the site, for the header and the page titles.

const labels: Record<string, string> = {
  start: 'Start',
  concepts: 'Concepts',
  guides: 'Guides',
  shmem: 'Shared memory',
  tools: 'Tools',
  reference: 'Reference',
  examples: 'Examples',
  project: 'Project',
};

/** The sidebar section of a docs page, from its id (for example "concepts/channels"). */
export function sectionLabel(id: string): string | undefined {
  return labels[id.split('/')[0]];
}

const generated = /^\/(api|source|reference)\//;

export const sections = [
  {
    label: 'Docs',
    href: '/start/overview/',
    match: (path: string, base: string) => {
      const p = path.slice(base.length);
      return p !== '/' && p !== '' && !generated.test(p);
    },
  },
  {
    label: 'Reference',
    href: '/reference/options/',
    match: (path: string, base: string) => path.startsWith(`${base}/reference/`),
  },
  { label: 'API', href: '/api/', match: (path: string, base: string) => path.startsWith(`${base}/api/`) },
  {
    label: 'Source',
    href: '/source/',
    match: (path: string, base: string) => path.startsWith(`${base}/source/`),
  },
];
