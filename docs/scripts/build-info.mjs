// Version and commit of the FairMQ checkout that the site is built from.
import { execFileSync } from 'node:child_process';

function git(cwd, ...args) {
  try {
    return execFileSync('git', args, { cwd, encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }).trim();
  } catch {
    return '';
  }
}

export function buildInfo(cwd = process.cwd()) {
  const commit = process.env.GITHUB_SHA || git(cwd, 'rev-parse', 'HEAD') || 'unknown';
  const commitShort = commit.slice(0, 7);
  const date = git(cwd, 'show', '-s', '--format=%cs', 'HEAD');
  const branch = process.env.GITHUB_HEAD_REF || process.env.GITHUB_REF_NAME || git(cwd, 'rev-parse', '--abbrev-ref', 'HEAD');
  // `git describe` gives e.g. v1.11.0-23-ge366c40; a checkout without tags gives nothing.
  const describe = git(cwd, 'describe', '--tags', '--match', 'v[0-9]*', '--long');
  const m = /^v(\d+)\.(\d+)\.(\d+)-(\d+)-g[0-9a-f]+$/.exec(describe);
  const release = m ? `v${m[1]}.${m[2]}.${m[3]}` : '';
  const ahead = m ? Number(m[4]) : -1;
  let label;
  if (m && ahead === 0) label = release;
  else if (m) label = `v${m[1]}.${m[2]}`;
  else label = 'dev';
  return {
    commit,
    commitShort,
    date,
    branch,
    release,
    ahead,
    label,
    unreleased: ahead !== 0,
    repo: 'https://github.com/FairRootGroup/FairMQ',
  };
}
