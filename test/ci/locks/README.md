# Committed spack lockfiles

`setup-deps` installs from `<env>-gcc<N>.lock` here when a matching file exists,
skipping concretization so the resolved hashes match the binaries the buildcache
pushed (guaranteed cache hits). When no lockfile is present it falls back to
concretizing from `test/ci/spack-<env>.yaml`.

## Regenerating

Lockfiles pin host externals (glibc, system gcc), so they must be concretized on
the same runner image CI uses (`ubuntu-24.04`) — do **not** generate them on a
local machine.

1. Run the `Spack Buildcache` workflow (push to `dev`/`master` touching the spack
   configs, or `workflow_dispatch`). It concretizes fresh (`fresh: 'true'`),
   pushes binaries, and uploads each env's `spack.lock` as a `lock-<env>-gcc<N>`
   artifact.
2. Download the artifacts and commit them here as `<env>-gcc<N>.lock`.

Regenerate whenever the specs in `test/ci/spack-*.yaml` change or the runner
image is bumped.
