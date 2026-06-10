import os
import shutil

from spack_repo.builtin.build_systems.generic import Package

from spack.package import *


class LibstdcxxTsan(Package):
    """ThreadSanitizer-instrumented build of GCC's libstdc++.

    GCC ships no supported way to produce a tsan-instrumented libstdc++ (and
    spack's gcc package offers no hook into the target-library flags), so this
    package builds only the libstdc++-v3 subtree from the GCC release tarball
    matching the toolchain, with -fsanitize=thread. The result is a drop-in
    runtime replacement for the compiler's own libstdc++ (same soname and
    symbol versions) that lets tsan observe the synchronization inside the
    C++ standard library (e.g. std::locale/std::regex lazy initialization).

    Select it via LD_LIBRARY_PATH in the environment of the instrumented
    executables only. Like any shared library built with gcc
    -fsanitize=thread it carries unresolved __tsan_* symbols satisfied by the
    executable's libtsan, so loading it into an uninstrumented executable
    fails.

    Recipe modeled on https://iree.dev/developers/debugging/sanitizers/.
    """

    homepage = "https://gcc.gnu.org/onlinedocs/libstdc++/"
    url = "https://ftp.gnu.org/gnu/gcc/gcc-15.2.0/gcc-15.2.0.tar.xz"

    # Keep in lockstep with the gcc version used by the tsan CI job: a
    # libstdc++ older than the compiler may lack GLIBCXX symbol versions
    # that binaries built by that compiler reference. Checksums match the
    # version() directives in spack's gcc package.
    version("15.2.0", sha256="438fd996826b0c82485a29da03a72d71d6e3541a83ec702df4271f6fe025d24e")
    version("14.3.0", sha256="e0dc77297625631ac8e50fa92fffefe899a4eb702592da5c32ef04e2293aca3a")

    depends_on("c", type="build")
    depends_on("cxx", type="build")
    depends_on("gmake", type="build")

    def install(self, spec, prefix):
        # gthr-default.h only materializes in a configured libgcc build dir;
        # on POSIX targets it is a verbatim copy of gthr-posix.h. Providing it
        # up front spares building libgcc (and the in-tree compiler).
        gthr_include = join_path(self.stage.source_path, "spack-gthr")
        mkdirp(gthr_include)
        copy(
            join_path(self.stage.source_path, "libgcc", "gthr-posix.h"),
            join_path(gthr_include, "gthr-default.h"),
        )

        flags = f"-I{gthr_include} -O2 -g -fno-omit-frame-pointer -fsanitize=thread"
        build_dir = join_path(self.stage.source_path, "spack-build")
        mkdirp(build_dir)
        with working_dir(build_dir):
            configure = Executable(
                join_path(self.stage.source_path, "libstdc++-v3", "configure")
            )
            configure(
                f"--prefix={prefix}",
                f"--libdir={prefix.lib}",
                "--disable-multilib",
                "--disable-libstdcxx-pch",
                "--enable-libstdcxx-threads=yes",
                "--with-default-libstdcxx-abi=new",
                f"CFLAGS={flags}",
                f"CXXFLAGS={flags}",
                "LDFLAGS=-fsanitize=thread",
            )
            make()
            make("install")

        # The runtime libraries land in the multilib os dir (lib64 on
        # x86_64), --libdir notwithstanding (--with-toolexeclibdir only
        # applies to cross builds); normalize to lib/ for the consumer.
        if os.path.isdir(prefix.lib64):
            mkdirp(prefix.lib)
            for entry in os.listdir(prefix.lib64):
                shutil.move(join_path(prefix.lib64, entry), join_path(prefix.lib, entry))
            os.rmdir(prefix.lib64)

        # Only the runtime library is consumed -- compilation uses the
        # compiler's own (identical) headers. Drop the rest to keep the
        # buildcache entry small.
        for directory in ("include", "share"):
            path = join_path(prefix, directory)
            if os.path.isdir(path):
                shutil.rmtree(path)
