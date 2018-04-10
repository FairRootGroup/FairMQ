# FairMQ

C++ Message Queuing Library

## Dependencies

TODO

## Installation

```bash
git clone https://github.com/FairRootGroup/FairMQ
mkdir fairmq_build && cd fairmq_build
cmake -DCMAKE_INSTALL_PREFIX=./fairmq_install ../fairmq
cmake --build . --target install
```

## Usage

In your `CMakeLists.txt`:

```cmake
find_package(FairMQ)
```

If FairMQ is not installed in system directories, you can hint the installation:

```cmake
set(CMAKE_PREFIX_PATH /path/to/FairMQ/installation ${CMAKE_PREFIX_PATH})
find_package(FairMQ)
```

`find_package(FairMQ)` will define an imported target `FairMQ::FairMQ` (An alias `FairRoot::FairMQ` is also defined, but it is deprecated).

By default, `find_package(FairMQ)` will also invoke `find_package` commands for all its dependencies. You can override this behaviour though, e.g.:

```cmake
set(FairMQ_PACKAGE_DEPENDENCIES_DISABLED ON)
find_package(FairMQ)

find_package(Boost COMPONENTS ${FairMQ_BOOST_COMPONENTS})
find_package(ZeroMQ)
# ...
```

The above is useful, if you need to customize the `find_package` calls of FairMQ's dependencies. Check the next section for more CMake options.

## CMake options

TODO complete list

On command line:

  * `-DDISABLE_COLOR=ON` disables coloured console output.
  * `-DBUILD_OFI_TRANSPORT=ON` enables building of the experimental OFI transport.

In front of the `find_package(FairMQ)` call:

  * `set(BUILD_OFI_TRANSPORT ON)` enables building of the experimental OFI transport.
  * `set(FairMQ_PACKAGE_DEPENDENCIES_DISABLED ON)` disables implicit discovery of all transitive package dependencies. 

After the `find_package(FairMQ)` the following CMake variables are defined:

  * `${FairMQ_BOOST_COMPONENTS}` contains the list of Boost components FairMQ depends on.

## Documentation

Standard [FairRoot](https://github.com/FairRootGroup/FairRoot) is running all the different analysis tasks within one process. FairMQ ([Message Queue](http://en.wikipedia.org/wiki/Message_queue)) allows starting tasks on different processes and provides the communication layer between these processes.

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
