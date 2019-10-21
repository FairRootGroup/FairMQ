← [Back](../README.md)

# 8. Controller SDK

The FairMQ Controller Software Development Kit (`-DBUILD_SDK=ON`) contains a (as of today still experimental) set of C++ APIs that provide essential functionality to the implementer of a global controller.

The FairMQ core library only provides two local controllers - `static` (a fixed sequence of state transitions) and `interactive` (a read-eval-print-loop which reads keyboard commands from standard input). A local controller only knows how steer a single [FairMQ device](Device.md) - in fact, it runs in a thread within the device process.

A global controller has knowledge about the full topology of connected FairMQ devices. Its responsibility is to facilitate the lifecycle of a distributed FairMQ-based application (*executing a topology*), such as

* allocating/releasing compute resources from a resource management system,
* launching/setting up the run-time environment and the FairMQ devices,
* driving the device state machines in lock-step across the full topology,
* pushing the device configuration,
* monitoring (some aspects of the application's) operation,
* and handling/reporting (some) error cases.

The low-level hook to integrate FairMQ devices with such a global contoller is the [plugin mechanism](Plugins.md) in the FairMQ core library. The FairMQ Controller SDK provides C++ APIs that communicate to the endpoints exposed by such a FairMQ plugin.

At the moment, the Controller SDK only supports [DDS](https://dds.gsi.de) as resource manager and run-time environment. A second implementation based on [PMIx](https://pmix.org/) (targeting its implementation in [Slurm](https://slurm.schedmd.com/documentation.html) and [OpenRTE](https://www-lb.open-mpi.org/papers/euro-pvmmpi-2005-orte/)) is in development.

The following section give a short overview on the APIs provided.

## RMS and run-time environment

The classes [`fair::mq::sdk::DDSEnvironment`](../fairmq/sdk/DDSEnvironment.h), [`fair::mq::sdk::DDSSession`](../fairmq/sdk/DDSSession.h), and [`fair::mq::sdk::DDSTopology`](../fairmq/sdk/DDSTopology.h) are thin wrappers of most of the synchronous APIs exposed by DDS ([`dds::tools_api`](http://dds.gsi.de/doc/api-docs/DDS/html/namespacedds_1_1tools__api.html) and [`dds::topology_api`](http://dds.gsi.de/doc/api-docs/DDS/html/namespacedds_1_1topology__api.html)). E.g. they allow to [start a DDS session](https://github.com/FairRootGroup/FairMQ/blob/077eb0ef691940d764cfd1852bf3981dc812ddbd/main.cpp#L26-L28), [allocate resources](https://github.com/FairRootGroup/FairMQ/blob/077eb0ef691940d764cfd1852bf3981dc812ddbd/main.cpp#L34) and [launch a topology](https://github.com/FairRootGroup/FairMQ/blob/077eb0ef691940d764cfd1852bf3981dc812ddbd/main.cpp#L39) from a C++ program.

## Driving the global state machine

The class [`fair::mq::sdk::Topology`](../fairmq/sdk/Topology.h) adds a FairMQ-specific view on an existing DDS session that is executing a topology of FairMQ devices. One can e.g. [initiate a state transition on all devices in the topology simultaneously](https://github.com/FairRootGroup/FairMQ/blob/077eb0ef691940d764cfd1852bf3981dc812ddbd/main.cpp#L48-L49). This topology transition completes once a topology-wide barrier is passed (all devices completed the transition). This effectively exposes the device state machine as a topology state machine. The implementation is based on remote procedure calls over the [DDS intercom service](http://dds.gsi.de/doc/api-docs/DDS/html/namespacedds_1_1intercom__api.html) between the controller and the DDS plugin shipped with FairMQ (`-DBUILD_DDS_PLUGIN=ON`).

For future versions of the SDK new APIs are planned to inspect and modify the device configurations and also operate only on subsets of a given topology.

← [Back](../README.md)
