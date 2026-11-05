// Astro integration that prepares the data for the generated pages.
//
// 1. It runs Doxygen when the XML is missing or older than a file in fairmq/.
// 2. It gives the pages a virtual module `virtual:fairmq/data` with the build
//    info (version, commit) and the absolute paths of the repository.
import { existsSync, mkdirSync, readdirSync, statSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { buildInfo } from './build-info.mjs';

const VIRTUAL_ID = 'virtual:fairmq/data';
const RESOLVED_ID = '\0' + VIRTUAL_ID;

function newestMtime(dir) {
  let newest = 0;
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, entry.name);
    if (entry.isDirectory()) newest = Math.max(newest, newestMtime(p));
    else if (/\.(h|cxx)$/.test(entry.name)) newest = Math.max(newest, statSync(p).mtimeMs);
  }
  return newest;
}

function runDoxygen({ docsDir, repoRoot, xmlDir, logger }) {
  const index = path.join(xmlDir, 'index.xml');
  const sources = path.join(repoRoot, 'fairmq');
  if (existsSync(index) && statSync(index).mtimeMs > newestMtime(sources)) {
    logger.info('Doxygen XML is up to date');
    return;
  }
  mkdirSync(path.join(docsDir, 'build', 'doxygen'), { recursive: true });
  const start = Date.now();
  try {
    execFileSync('doxygen', ['Doxyfile'], { cwd: docsDir, stdio: ['ignore', 'ignore', 'pipe'] });
  } catch (err) {
    if (err.code === 'ENOENT') {
      throw new Error('Doxygen is not installed. Install it (for example `apt install doxygen`) and build again.');
    }
    throw new Error(`Doxygen failed: ${err.stderr?.toString() ?? err.message}`);
  }
  logger.info(`Doxygen XML written in ${((Date.now() - start) / 1000).toFixed(1)} s`);
}

export default function fairmqData() {
  return {
    name: 'fairmq-data',
    hooks: {
      'astro:config:setup': ({ config, updateConfig, logger }) => {
        const docsDir = fileURLToPath(config.root);
        const repoRoot = path.resolve(docsDir, '..');
        const xmlDir = path.join(docsDir, 'build', 'doxygen', 'xml');
        runDoxygen({ docsDir, repoRoot, xmlDir, logger });
        const info = buildInfo(repoRoot);
        logger.info(`Building docs for ${info.label} (${info.branch} @ ${info.commitShort})`);
        const source =
          `export const build = ${JSON.stringify(info)};\n` +
          `export const paths = ${JSON.stringify({ docsDir, repoRoot, xmlDir })};\n`;
        updateConfig({
          vite: {
            plugins: [
              {
                name: 'fairmq-data',
                resolveId(id) {
                  if (id === VIRTUAL_ID) return RESOLVED_ID;
                },
                load(id) {
                  if (id === RESOLVED_ID) return source;
                },
              },
            ],
          },
        });
      },
    },
  };
}
