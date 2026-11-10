declare module 'virtual:fairmq/data' {
  /** Version and commit of the FairMQ checkout that the site is built from. */
  export const build: {
    commit: string;
    commitShort: string;
    date: string;
    branch: string;
    release: string;
    ahead: number;
    label: string;
    unreleased: boolean;
    repo: string;
  };
  /** Absolute paths, resolved when the site build starts. */
  export const paths: {
    docsDir: string;
    repoRoot: string;
    xmlDir: string;
  };
}
