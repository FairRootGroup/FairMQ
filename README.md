# FairMQ

[![license](https://alfa-ci.gsi.de/shields/badge/license-LGPL--3.0-orange.svg)](COPYRIGHT)
[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.1689985.svg)](https://doi.org/10.5281/zenodo.1689985)
[![OpenSSF Best Practices](https://bestpractices.coreinfrastructure.org/projects/6915/badge)](https://bestpractices.coreinfrastructure.org/projects/6915)
[![fair-software.eu](https://img.shields.io/badge/fair--software.eu-%E2%97%8F%20%20%E2%97%8F%20%20%E2%97%8B%20%20%E2%97%8F%20%20%E2%97%8F-yellow)](https://github.com/FairRootGroup/FairMQ/actions/workflows/fair-software.yml)
[![Spack package](https://repology.org/badge/version-for-repo/spack/fairmq.svg)](https://repology.org/project/fairmq/versions)

C++ message queuing library and framework for large-scale data processing.

**Documentation: <https://fairrootgroup.github.io/FairMQ/>**

FairMQ helps to build data processing workflows of particle physics experiments
from many small processes. Each process is a *device* with a state machine. Devices
send messages to each other through *channels* with the socket patterns of ZeroMQ
(push/pull, pub/sub, req/rep, pair). Each channel uses a *transport*:

* `zeromq` sends the messages through ZeroMQ sockets, also to other hosts.
* `shmem` puts the payload into shared memory and sends only a small header, so
  devices on one host exchange data without a copy.

Plugins configure and control the devices. FairMQ was developed in the context of
[FairRoot](https://github.com/FairRootGroup/FairRoot).

## Install

With [Spack](https://spack.readthedocs.io/en/latest/getting_started.html):

```bash
spack install fairmq
```

From source:

```bash
git clone https://github.com/FairRootGroup/FairMQ fairmq_source
cmake -S fairmq_source -B fairmq_build -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build fairmq_build
cmake --install fairmq_build --prefix $(pwd)/fairmq_install
```

The [install page](https://fairrootgroup.github.io/FairMQ/start/install/) lists the
dependencies, the build options and how to run the tests.

## Get started

* [Your first device](https://fairrootgroup.github.io/FairMQ/start/first-device/): write, build and run a sampler and a sink.
* [Use FairMQ in a CMake project](https://fairrootgroup.github.io/FairMQ/start/cmake/): `find_package(FairMQ)` and the target `FairMQ::FairMQ`.
* [Examples](https://fairrootgroup.github.io/FairMQ/examples/1-1/): the programs in [`examples/`](examples), with their topologies.
* [API reference](https://fairrootgroup.github.io/FairMQ/api/) and [source browser](https://fairrootgroup.github.io/FairMQ/source/).

## Releases and support

Find all releases [on GitHub](https://github.com/FairRootGroup/FairMQ/releases) and the
changes in the [release notes](https://fairrootgroup.github.io/FairMQ/project/release-notes/).
Report bugs and ask questions in the [issues](https://github.com/FairRootGroup/FairMQ/issues).
Read [Contributing](https://fairrootgroup.github.io/FairMQ/project/contributing/) before you
open a pull request.

## License

FairMQ is free software under the [LGPL-3.0](LICENSE). See [COPYRIGHT](COPYRIGHT) and
[how to cite FairMQ](https://fairrootgroup.github.io/FairMQ/project/license/).
