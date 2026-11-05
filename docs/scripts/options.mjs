// Reads the command-line options of FairMQ from the C++ sources:
// the `add_options()` chains of boost::program_options.
import { readFileSync } from 'node:fs';
import path from 'node:path';

/** Splits `s` at top-level commas (outside parentheses, brackets and strings). */
function splitTop(s) {
  const out = [];
  let depth = 0;
  let cur = '';
  let str = false;
  for (let i = 0; i < s.length; i++) {
    const c = s[i];
    if (str) {
      cur += c;
      if (c === '\\') cur += s[++i];
      else if (c === '"') str = false;
      continue;
    }
    if (c === '"') {
      str = true;
      cur += c;
      continue;
    }
    if (c === '(' || c === '[' || c === '{') depth++;
    if (c === ')' || c === ']' || c === '}') depth--;
    if (c === ',' && depth === 0) {
      out.push(cur.trim());
      cur = '';
    } else cur += c;
  }
  if (cur.trim()) out.push(cur.trim());
  return out;
}

/** Concatenates adjacent C++ string literals: "a" "b" -> ab. */
function cString(s) {
  const parts = [...s.matchAll(/"((?:[^"\\]|\\.)*)"/g)].map((m) =>
    m[1].replace(/\\n/g, '\n').replace(/\\"/g, '"').replace(/\\t/g, '\t').replace(/\\\\/g, '\\'),
  );
  return parts.join('');
}

/** The groups `("...", ...)` of an add_options() chain that starts at `start`. */
function groupsAfter(text, start) {
  const groups = [];
  let i = text.indexOf('add_options()', start) + 'add_options()'.length;
  for (;;) {
    while (i < text.length && /\s/.test(text[i])) i++;
    if (text[i] !== '(') break;
    let depth = 0;
    let str = false;
    let j = i;
    for (; j < text.length; j++) {
      const c = text[j];
      if (str) {
        if (c === '\\') j++;
        else if (c === '"') str = false;
        continue;
      }
      if (c === '"') str = true;
      else if (c === '/' && text[j + 1] === '/') {
        j = text.indexOf('\n', j);
        continue;
      } else if (c === '(') depth++;
      else if (c === ')') {
        depth--;
        if (depth === 0) break;
      }
    }
    groups.push({ text: text.slice(i + 1, j), line: text.slice(0, i).split('\n').length });
    i = j + 1;
  }
  return groups;
}

function parseValue(spec) {
  const out = { type: '', default: undefined, implicit: undefined, multitoken: false, composing: false, required: false, switch: false };
  const t = /po::value\s*<\s*(.+?)\s*>\s*\(/.exec(spec) ?? /value\s*<\s*(.+?)\s*>\s*\(/.exec(spec);
  if (t) out.type = t[1].replace(/\s+/g, ' ').replace(/^std::/, '');
  if (/po::bool_switch|bool_switch\(/.test(spec)) {
    out.type = 'bool';
    out.switch = true;
  }
  const d = /->\s*default_value\((.*?)\)(?=\s*(->|$))/s.exec(spec);
  if (d) out.default = d[1].trim();
  const im = /->\s*implicit_value\((.*?)\)(?=\s*(->|$))/s.exec(spec);
  if (im) out.implicit = im[1].trim();
  out.multitoken = /->\s*multitoken\(\)/.test(spec);
  out.composing = /->\s*composing\(\)/.test(spec);
  out.required = /->\s*required\(\)/.test(spec);
  return out;
}

/** Makes a C++ default value readable: "2ULL << 30" -> "2147483648", "\"zeromq\"" -> "zeromq". */
export function displayValue(v) {
  if (v === undefined) return undefined;
  let s = v.trim();
  const shift = /^(\d+)U?L*L*\s*<<\s*(\d+)$/.exec(s);
  if (shift) return String(BigInt(shift[1]) << BigInt(shift[2]));
  if (/^".*"$/.test(s)) return cString(s);
  if (/^-?\d+\.$/.test(s)) return s.slice(0, -1);
  if (/^-?\d+\.0+$/.test(s)) return s.replace(/\.0+$/, '');
  return s;
}

/** Reads the option chains of the given files. `files` maps a path to a group label. */
export function readOptions(repoRoot, files) {
  const options = [];
  for (const { file, group, source } of files) {
    const text = readFileSync(path.join(repoRoot, file), 'utf8');
    let at = 0;
    while ((at = text.indexOf('add_options()', at)) >= 0) {
      for (const g of groupsAfter(text, at)) {
        const parts = splitTop(g.text);
        const names = cString(parts[0]);
        const [long, short] = names.split(',');
        const rest = parts.slice(1);
        let spec = '';
        let desc = '';
        if (rest.length === 1) desc = cString(rest[0]);
        else if (rest.length >= 2) {
          spec = rest[0];
          desc = cString(rest.slice(1).join(','));
        }
        const value = spec ? parseValue(spec) : { type: '', switch: true };
        const grp = typeof group === 'function' ? group(long) : group;
        options.push({
          name: long,
          short: short || '',
          type: value.type,
          default: displayValue(value.default),
          implicit: displayValue(value.implicit),
          multitoken: !!value.multitoken,
          composing: !!value.composing,
          flag: !spec,
          description: desc.trim(),
          group: grp,
          source,
          file,
          line: g.line,
        });
      }
      at += 'add_options()'.length;
    }
  }
  return options;
}

/** The options that every device made with runDevice.h has. */
export function deviceOptions(repoRoot) {
  const logging = new Set(['severity', 'file-severity', 'verbosity', 'color', 'log-to-file']);
  return readOptions(repoRoot, [
    { file: 'fairmq/ProgOptions.cxx', source: 'ProgOptions', group: (n) => (logging.has(n) ? 'Logging' : 'General') },
    { file: 'fairmq/plugins/config/Config.cxx', source: 'config plugin', group: (n) => (/^(shm|bad-alloc|shmid)/.test(n) ? 'Shared memory' : 'Device') },
    { file: 'fairmq/plugins/control/Control.cxx', source: 'control plugin', group: 'Control' },
    { file: 'fairmq/PluginManager.cxx', source: 'PluginManager', group: 'Plugins' },
  ]);
}
