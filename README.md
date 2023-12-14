<!-- {#mainpage} -->
# FairMQ

[![license](https://alfa-ci.gsi.de/shields/badge/license-LGPL--3.0-orange.svg)](COPYRIGHT)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.1689985.svg)](https://doi.org/10.5281/zenodo.1689985)
[![OpenSSF Best Practices](https://bestpractices.coreinfrastructure.org/projects/6915/badge)](https://bestpractices.coreinfrastructure.org/projects/6915)
[![fair-software.eu](https://img.shields.io/badge/fair--software.eu-%E2%97%8F%20%20%E2%97%8F%20%20%E2%97%8B%20%20%E2%97%8F%20%20%E2%97%8F-yellow)](https://github.com/FairRootGroup/FairMQ/actions/workflows/fair-software.yml)
[![Spack package](https://repology.org/badge/version-for-repo/spack/fairmq.svg)](https://repology.org/project/fairmq/versions)

C++ Message Queuing Library and Framework

Docs: [Book](https://github.com/FairRootGroup/FairMQ/blob/dev/README.md#documentation)

Find all FairMQ releases [here](https://github.com/FairRootGroup/FairMQ/releases).

## Introduction

FairMQ is designed to help implementing large-scale data processing workflows needed in next-generation Particle Physics experiments. FairMQ is written in C++ and aims to
  * provide **an asynchronous message passing abstraction** of different data transport technologies,
  * provide a reasonably **efficient data transport** service (zero-copy, high throughput),
  * be **data format agnostic**, and
  * provide **basic building blocks** that can be used to implement higher level data processing workflows.

The core of FairMQ provides an abstract asynchronous message passing API with scalability protocols
inspired by [ZeroMQ](https://github.com/zeromq/libzmq) (e.g. PUSH/PULL, PUB/SUB).
FairMQ provides multiple implementations for its API (so-called "transports",
e.g. `zeromq` and `shmem` (latest release of the `ofi` transport in v1.4.56, removed since v1.5+)) to cover
a variety of use cases
(e.g. inter-thread, inter-process, inter-node communication) and machines (e.g. Ethernet, Infiniband).
In addition to this core functionality FairMQ provides a framework for creating "devices" - actors which
are communicating through message passing. FairMQ does not only allow the user to use different transport
but also to mix them; i.e: A Device can communicate using different transport on different channels at the
same time. Device execution is modelled as a simple state machine that shapes the integration points for
the user task. Devices also incorporate a plugin system for runtime configuration and control.
Next to the provided [devices](https://github.com/FairRootGroup/FairMQ/tree/master/fairmq/devices) and
[plugins](https://github.com/FairRootGroup/FairMQ/tree/master/fairmq/plugins) the user can extend FairMQ
by developing his own plugins to integrate his devices with external configuration and control services.

FairMQ has been developed in the context of its mother project [FairRoot](https://github.com/FairRootGroup/FairRoot) -
a simulation, reconstruction and analysis framework.

## Installation from Source

Recommended:

```bash
git clone https://github.com/FairRootGroup/FairMQ fairmq_source
cmake -S fairmq_source -B fairmq_build -GNinja -DCMAKE_BUILD_TYPE=Release [-DBUILD_TESTING=ON]
cmake --build fairmq_build
[ctest --test-dir fairmq_build --output-on-failure --schedule-random -j<ncpus>] # needs -DBUILD_TESTING=ON
cmake --install fairmq_build --prefix $(pwd)/fairmq_install
```

Please consult the [manpages of your CMake version](https://cmake.org/cmake/help/latest/manual/cmake.1.html) for more options.

If dependencies are not installed in standard system directories, you can hint the installation location via
`-DCMAKE_PREFIX_PATH=...` or per dependency via `-D{DEPENDENCY}_ROOT=...` (`*_ROOT` variables can also be environment variables).

## Installation via Spack

Prerequisite: [Spack](https://spack.readthedocs.io/en/latest/getting_started.html)

```bash
spack info fairmq # inspect build options
spack install fairmq # build latest packaged version with default options
```

Build FairMQ's dependencies via Spack for development:
```bash
git clone -b dev https://github.com/FairRootGroup/FairMQ fairmq_source
spack --env fairmq_source install # installs deps declared in fairmq_source/spack.yaml
spack env activate fairmq_source # sets $CMAKE_PREFIX_PATH which is used by CMake to find FairMQ's deps
cmake -S fairmq_source -B fairmq_build -GNinja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
# develop, compile, test
spack env deactivate # at end of dev session, or simply close the shell
```

## Usage

FairMQ ships as a CMake package, so in your `CMakeLists.txt` you can discover it like this:

```cmake
find_package(FairCMakeModules 1.0 REQUIRED)
include(FairFindPackage2)
find_package2(FairMQ)
find_package2_implicit_dependencies()
```

The [`FairFindPackage2` module](https://fairrootgroup.github.io/FairCMakeModules/latest/module/FairFindPackage2.html) is part of the [`FairCMakeModules` package](https://fairrootgroup.github.io/FairCMakeModules).

If FairMQ is not installed in system directories, you can hint the installation:

```cmake
list(PREPEND CMAKE_PREFIX_PATH /path/to/fairmq_install)
```

## Dependencies

  * [Boost](https://www.boost.org/)
  * [CMake](https://cmake.org/)
  * [Doxygen](http://www.doxygen.org/)
  * [FairCMakeModules](https://github.com/FairRootGroup/FairCMakeModules) (optionally bundled)
  * [FairLogger](https://github.com/FairRootGroup/FairLogger)
  * [GTest](https://github.com/google/googletest) (optionally bundled)
  * [ZeroMQ](http://zeromq.org/)

  Which dependencies are required depends on which components are built.

  Supported platform is Linux. macOS is supported on a best-effort basis.

## CMake options

On command line:

  * `-DDISABLE_COLOR=ON` disables coloured console output.
  * `-DBUILD_TESTING=OFF` disables building of tests.
  * `-DBUILD_EXAMPLES=OFF` disables building of examples.
  * `-DBUILD_DOCS=ON` enables building of API docs.
  * `-DFAIRMQ_CHANNEL_DEFAULT_AUTOBIND=OFF` disable channel `autoBind` by default
  * You can hint non-system installations for dependent packages, see the #installation-from-source section above

After the `find_package(FairMQ)` call the following CMake variables are defined:

| Variable | Info |
| --- | --- |
| `${FairMQ_PACKAGE_DEPENDENCIES}` | the list of public package dependencies |
| `${FairMQ_<dep>_VERSION}` | the minimum `<dep>` version FairMQ requires |
| `${FairMQ_<dep>_COMPONENTS}` | the list of `<dep>` components FairMQ depends on |
| `${FairMQ_PACKAGE_COMPONENTS}` | the list of components FairMQ consists of |
| `${FairMQ_#COMPONENT#_FOUND}` | `TRUE` if this component was built |
| `${FairMQ_VERSION}` | the version in format `MAJOR.MINOR.PATCH` |
| `${FairMQ_GIT_VERSION}` | the version in the format returned by `git describe --tags --dirty --match "v*"` |
| `${FairMQ_PREFIX}` | the actual installation prefix |
| `${FairMQ_BINDIR}` | the installation bin directory |
| `${FairMQ_INCDIR}` | the installation include directory |
| `${FairMQ_LIBDIR}` | the installation lib directory |
| `${FairMQ_DATADIR}` | the installation data directory (`../share/fairmq`) |
| `${FairMQ_CMAKEMODDIR}` | the installation directory of shipped CMake find modules |
| `${FairMQ_BUILD_TYPE}` | the value of `CMAKE_BUILD_TYPE` at build-time |
| `${FairMQ_CXX_FLAGS}` | the values of `CMAKE_CXX_FLAGS` and `CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}` at build-time |

## Documentation

1. [Device](docs/Device.md#1-device)
   1. [Topology](docs/Device.md#11-topology)
   2. [Communication Patterns](docs/Device.md#12-communication-patterns)
   3. [State Machine](docs/Device.md#13-state-machine)
   4. [Multiple devices in the same process](docs/Device.md#15-multiple-devices-in-the-same-process)
2. [Transport Interface](docs/Transport.md#2-transport-interface)
   1. [Message](docs/Transport.md#21-message)
      1. [Ownership](docs/Transport.md#211-ownership)
   2. [Channel](docs/Transport.md#22-channel)
   3. [Poller](docs/Transport.md#23-poller)
3. [Configuration](docs/Configuration.md#3-configuration)
    1. [Device Configuration](docs/Configuration.md#31-device-configuration)
    2. [Communication Channels Configuration](docs/Configuration.md#32-communication-channels-configuration)
        1. [JSON Parser](docs/Configuration.md#321-json-parser)
        2. [SuboptParser](docs/Configuration.md#322-suboptparser)
    3. [Introspection](docs/Configuration.md#33-introspection)
4. [Development](docs/Development.md#4-development)
   1. [Testing](docs/Development.md#41-testing)
   2. [Static Analysis](docs/Development.md#42-static-analysis)
      1. [CMake Integration](docs/Development.md#421-cmake-integration)
      2. [Extra Compiler Arguments](docs/Development.md#422-extra-compiler-arguments)
5. [Logging](docs/Logging.md#5-logging)
   1. [Log severity](docs/Logging.md#51-log-severity)
   2. [Log verbosity](docs/Logging.md#52-log-verbosity)
   3. [Color for console output](docs/Logging.md#53-color)
   4. [File output](docs/Logging.md#54-file-output)
   5. [Custom sinks](docs/Logging.md#55-custom-sinks)
6. [Examples](docs/Examples.md#6-examples)
7. [Plugins](docs/Plugins.md#7-plugins)
   1. [Usage](docs/Plugins.md#71-usage)
   2. [Development](docs/Plugins.md#72-development)
   3. [Provided Plugins](docs/Plugins.md#73-provided-plugins)
       1. [PMIx](docs/Plugins.md#731-pmix)
