// Groups the members of a class by their name, for the filter on the API pages.

export const FAMILY_ORDER = [
  'Construction',
  'Types',
  'Get',
  'Set',
  'Transfer',
  'Create',
  'Events',
  'Other',
  'Operators',
  'Constants',
  'Data',
  'Friends',
];

export function family(m, owner) {
  const n = m.name;
  if (m.kind === 'friend') return 'Friends';
  if (m.kind === 'typedef' || m.kind === 'enum') return 'Types';
  if (m.kind === 'variable') return m.static ? 'Constants' : 'Data';
  if (m.kind === 'function') {
    if (n === owner.short || n === `~${owner.short}` || /^operator\s*=$/.test(n)) return 'Construction';
    if (/^operator/.test(n)) return 'Operators';
    if (/^(Get|Is|Has|Are|Can)[A-Z_]/.test(n)) return 'Get';
    if (/^(Set|Update|Change|Reset|Add|Remove)[A-Z_]/.test(n)) return 'Set';
    if (/^(Send|Receive)/.test(n)) return 'Transfer';
    if (/^(New|Create|Make)[A-Z_]/.test(n)) return 'Create';
    if (/^(Subscribe|Unsubscribe|On[A-Z])/.test(n)) return 'Events';
  }
  return 'Other';
}
