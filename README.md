<!-- {#mainpage} -->
# FairMQ [![license](https://alfa-ci.gsi.de/shields/badge/license-LGPL--3.0-orange.svg)](COPYRIGHT) [![build status](https://alfa-ci.gsi.de/buildStatus/icon?job=FairRootGroup/FairMQ/master)](https://alfa-ci.gsi.de/blue/organizations/jenkins/FairRootGroup%2FFairMQ/branches) [![test coverage master branch](https://codecov.io/gh/FairRootGroup/FairMQ/branch/master/graph/badge.svg)](https://codecov.io/gh/FairRootGroup/FairMQ/branch/master) [![Coverity Badge](https://alfa-ci.gsi.de/shields/coverity/scan/fairrootgroup-fairmq.svg)](https://scan.coverity.com/projects/fairrootgroup-fairmq) [![Codacy Badge](https://api.codacy.com/project/badge/Grade/6b648d95d68d4c4eae833b84f84d299c)](https://www.codacy.com/app/dennisklein/FairMQ?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=FairRootGroup/FairMQ&amp;utm_campaign=Badge_Grade)

C++ Message Queuing Library and Framework

| Release | Version | Docs |
| :---: | :--- | :--- |
| `stable` | [![release](https://alfa-ci.gsi.de/shields/github/release/FairRootGroup/FairMQ.svg)](https://github.com/FairRootGroup/FairMQ/releases/latest) | [API](https://fairrootgroup.github.io/FairMQ/latest), [Book](https://github.com/FairRootGroup/FairMQ/blob/master/README.md#documentation) |
| `testing` | [![dev tag](https://alfa-ci.gsi.de/shields/github/tag/FairRootGroup/FairMQ.svg)](https://github.com/FairRootGroup/FairMQ/tags) | [Book](https://github.com/FairRootGroup/FairMQ/blob/dev/README.md#documentation) |

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
e.g. `zeromq`, `shmem`, `nanomsg`, and `ofi` (in development)) to cover a variety of use cases
(e.g. inter-thread, inter-process, inter-node communication) and machines (e.g. Ethernet, Infiniband).
In addition to this core functionality FairMQ provides a framework for creating "devices" - actors which
are communicating through message passing. FairMQ does not only allow the user to use different transport but also to mix them; i.e: A Device can communicate using different transport on different channels at the same time. Device execution is modelled as a simple state machine that
shapes the integration points for the user task. Devices also incorporate a plugin system for runtime configuration and control.
Next to the provided devices and plugins (e.g. [DDS](https://github.com/FairRootGroup/DDS))
the user can extend FairMQ by developing his own plugins to integrate his devices with external
configuration and control services.

FairMQ has been developed in the context of its mother project [FairRoot](https://github.com/FairRootGroup/FairRoot) -
a simulation, reconstruction and analysis framework.

## Dependencies

  * PUBLIC: [**Boost**](https://www.boost.org/), [**FairLogger**](https://github.com/FairRootGroup/FairLogger)
  * BUILD: [CMake](https://cmake.org/), [GTest](https://github.com/google/googletest), [Doxygen](http://www.doxygen.org/)
  * PRIVATE: [ZeroMQ](http://zeromq.org/), [Msgpack](https://msgpack.org/index.html), [nanomsg](http://nanomsg.org/),
[asiofi](https://github.com/FairRootGroup/asiofi), [DDS](http://dds.gsi.de), [PMIx](https://pmix.org/)

  Supported platforms: Linux and MacOS.

## Installation from Source

```bash
git clone https://github.com/FairRootGroup/FairMQ fairmq
mkdir fairmq_build && cd fairmq_build
cmake -DCMAKE_INSTALL_PREFIX=./fairmq_install ../fairmq
cmake --build . --target install
```

If dependencies are not installed in standard system directories, you can hint the installation location via `-DCMAKE_PREFIX_PATH=...` or per dependency via `-D{DEPENDENCY}_ROOT=...`. `{DEPENDENCY}` can be `GTEST`, `BOOST`, `FAIRLOGGER`, `ZEROMQ`, `MSGPACK`, `NANOMSG`, `OFI`, `PMIX`, `ASIOFI` or `DDS` (`*_ROOT` variables can also be environment variables).

## Usage

FairMQ ships as a CMake package, so in your `CMakeLists.txt` you can discover it like this:

```cmake
find_package(FairMQ)
```

If FairMQ is not installed in system directories, you can hint the installation:

```cmake
set(CMAKE_PREFIX_PATH /path/to/FairMQ_install_prefix ${CMAKE_PREFIX_PATH})
find_package(FairMQ)
```

`find_package(FairMQ)` will define an imported target `FairMQ::FairMQ`.

In order to succesfully compile and link against the `FairMQ::FairMQ` target, you need to discover its public package dependencies, too.

```cmake
find_package(FairMQ)
if(FairMQ_FOUND)
  find_package(FairLogger ${FairMQ_FairLogger_VERSION})
  find_package(Boost ${FairMQ_Boost_VERSION} COMPONENTS ${FairMQ_Boost_COMPONENTS})
endif()
```

Of course, feel free to customize the above commands to your needs.

Optionally, you can require certain FairMQ package components and a minimum version:

```cmake
find_package(FairMQ 1.1.0 COMPONENTS nanomsg_transport dds_plugin)
if(FairMQ_FOUND)
  find_package(FairLogger ${FairMQ_FairLogger_VERSION})
  find_package(Boost ${FairMQ_Boost_VERSION} COMPONENTS ${FairMQ_Boost_COMPONENTS})
endif()
```

When building FairMQ, CMake will print a summary table of all available package components.

## CMake options

On command line:

  * `-DDISABLE_COLOR=ON` disables coloured console output.
  * `-DBUILD_TESTING=OFF` disables building of tests.
  * `-DBUILD_EXAMPLES=OFF` disables building of examples.
  * `-DBUILD_NANOMSG_TRANSPORT=ON` enables building of nanomsg transport.
  * `-DBUILD_OFI_TRANSPORT=ON` enables building of the experimental OFI transport.
  * `-DBUILD_DDS_PLUGIN=ON` enables building of the DDS plugin.
  * `-DBUILD_PMIX_PLUGIN=ON` enables building of the PMIx plugin.
  * `-DBUILD_DOCS=ON` enables building of API docs.
  * You can hint non-system installations for dependent packages, see the #Installation section above

After the `find_package(FairMQ)` call the following CMake variables are defined:

| Variable | Info |
| --- | --- |
| `${FairMQ_PACKAGE_DEPENDENCIES}` | the list of public package dependencies |
| `${FairMQ_Boost_VERSION}` | the minimum Boost version FairMQ requires |
| `${FairMQ_Boost_COMPONENTS}` | the list of Boost components FairMQ depends on |
| `${FairMQ_FairLogger_VERSION}` | the minimum FairLogger version FairMQ requires |
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
| `${FairMQ_CXX_STANDARD_REQUIRED}` | the value of `CMAKE_CXX_STANDARD_REQUIRED` at built-time |
| `${FairMQ_CXX_STANDARD}` | the value of `CMAKE_CXX_STANDARD` at built-time |
| `${FairMQ_CXX_EXTENSIONS}` | the values of `CMAKE_CXX_EXTENSIONS` at built-time |

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
5. [Logging](docs/Logging.md#5-logging)
   1. [Log severity](docs/Logging.md#51-log-severity)
   2. [Log verbosity](docs/Logging.md#52-log-verbosity)
   3. [Color for console output](docs/Logging.md#53-color)
   4. [File output](docs/Logging.md#54-file-output)
   5. [Custom sinks](docs/Logging.md#55-custom-sinks)
6. [Examples](docs/Examples.md#6-examples)
