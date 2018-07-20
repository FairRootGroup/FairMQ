<!-- {#mainpage} -->
# FairMQ

C++ Message passing framework

| Branch | Version | Docs | Status |
| :---: | :--- | :--- | :--- |
| `master` | [![release](https://img.shields.io/github/release/FairRootGroup/FairMQ.svg)](https://github.com/FairRootGroup/FairMQ/releases/latest) | [API](https://fairrootgroup.github.io/FairMQ/latest), [Book](https://github.com/FairRootGroup/FairMQ/blob/master/README.md#documentation) | [![build status master branch](https://alfa-ci.gsi.de/buildStatus/icon?job=FairRootGroup/FairMQ/master)](https://alfa-ci.gsi.de/blue/organizations/jenkins/FairRootGroup%2FFairMQ/branches) [![test coverage master branch](https://codecov.io/gh/FairRootGroup/FairMQ/branch/master/graph/badge.svg)](https://codecov.io/gh/FairRootGroup/FairMQ/branch/master) |
| `dev` | [![dev tag](https://img.shields.io/github/tag/FairRootGroup/FairMQ.svg)](https://github.com/FairRootGroup/FairMQ/tags) | [Book](https://github.com/FairRootGroup/FairMQ/blob/dev/README.md#documentation) | [![build status dev branch](https://alfa-ci.gsi.de/buildStatus/icon?job=FairRootGroup/FairMQ/dev)](https://alfa-ci.gsi.de/blue/organizations/jenkins/FairRootGroup%2FFairMQ/branches) [![test coverage dev branch](https://codecov.io/gh/FairRootGroup/FairMQ/branch/dev/graph/badge.svg)](https://codecov.io/gh/FairRootGroup/FairMQ/branch/dev) |

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

Find all FairMQ releases and development tags [here](https://github.com/FairRootGroup/FairMQ/releases).

## Dependencies

  * [**Boost**](https://www.boost.org/) (PUBLIC)
  * [**FairLogger**](https://github.com/FairRootGroup/FairLogger) (PUBLIC)
  * [CMake](https://cmake.org/) (BUILD)
  * [GTest](https://github.com/google/googletest) (BUILD, optional, `tests`)
  * [Doxygen](http://www.doxygen.org/) (BUILD, optional, `docs`)
  * [ZeroMQ](http://zeromq.org/) (PRIVATE)
  * [Msgpack](https://msgpack.org/index.html) (PRIVATE, optional, `nanomsg_transport`)
  * [nanomsg](http://nanomsg.org/) (PRIVATE, optional, `nanomsg_transport`)
  * [OFI](https://ofiwg.github.io/libfabric/) (PRIVATE, optional, `ofi_transport`)
  * [Protobuf](https://developers.google.com/protocol-buffers/) (PRIVATE, optional, `ofi_transport`)
  * [DDS](http://dds.gsi.de) (PRIVATE, optional, `dds_plugin`)

  Supported platforms: Linux and MacOS.

## Installation from Source

```bash
git clone https://github.com/FairRootGroup/FairMQ fairmq
mkdir fairmq_build && cd fairmq_build
cmake -DCMAKE_INSTALL_PREFIX=./fairmq_install ../fairmq
cmake --build . --target install
```

If dependencies are not installed in standard system directories, you can hint the installation location via `-DCMAKE_PREFIX_PATH=...` or per dependency via `-D{DEPENDENCY}_ROOT=...`. `{DEPENDENCY}` can be `GTEST`, `BOOST`, `FAIRLOGGER`, `ZEROMQ`, `MSGPACK`, `NANOMSG`, `OFI`, `PROTOBUF`, or `DDS` (`*_ROOT` variables can also be environment variables).

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
  find_package(Boost ${FairMQ_Boost_VERSION} COMPONENTS ${FairMQ_BOOST_COMPONENTS})
endif()
```

Of course, feel free to customize the above commands to your needs.

Optionally, you can require certain FairMQ package components and a minimum version:

```cmake
find_package(FairMQ 1.1.0 COMPONENTS nanomsg_transport dds_plugin)
if(FairMQ_FOUND)
  find_package(FairLogger ${FairMQ_FairLogger_VERSION})
  find_package(Boost ${FairMQ_Boost_VERSION} COMPONENTS ${FairMQ_BOOST_COMPONENTS})
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
| `${FairMQ_ROOT}` | the actual installation prefix, notice the difference to the hint variable `FAIRMQ_ROOT` |
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

## License

GNU Lesser General Public Licence (LGPL) version 3, see [LICENSE](LICENSE).

Copyright (C) 2013-2018 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH
