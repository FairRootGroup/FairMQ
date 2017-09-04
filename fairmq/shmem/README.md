# Shared Memory transport

Shared memory transport for FairMQ. To try with existing devices, run the devices with `--transport shmem` option or configure channel transport in JSON (see examples/MQ/multiple-transports).

The transport manages shared memory via boost::interprocess library. The transfer of the meta data, required to locate the content in the shared memory, is done via ZeroMQ. The transport supports all communication patterns where a single message is received by a single receiver. For multiple receivers for the same message, the message has to be copied.

Devices track and cleanup shared memory on shutdown. For more information on the current shared memory segment and additional cleanup options, see following section.

## Shared memory monitor

The shared memory monitor tool, supplied with the shared memory transport can be used to monitor shared memory use and automatically cleanup shared memory in case of device crashes.

With default arguments the monitor will run indefinitely with no output, and clean up shared memory segment if it is open and no heartbeats from devices arrive within a timeout period. It can be further customized with following parameters: 

  `--segment-name <arg>`: customize the name of the shared memory segment (default is "fairmq_shmem_main").
  `--cleanup`: start monitor, perform cleanup of the memory and quit.
  `--self-destruct`: run until the memory segment is closed (either naturally via cleanup performed by devices or in case of a crash (no heartbeats within timeout)).
  `--interactive`: run interactively, with detailed segment details and user input for various shmem operations.
  `--timeout <arg>`: specifiy the timeout for the heartbeats from shmem transports in milliseconds (default 5000).

The options can be combined, with the exception of `--cleanup` option, which will invoke the described behaviour independent of other options.
Without the `--self-destruct` option, the monitor will run continously, moitoring (and cleaning up if needed) consecutive topologies.

Possible further implementation would be to run the monitor with `--self-destruct` with each topology.

The FairMQShmMonitor class can also be used independently from the supplied executable (built from `runFairMQShmMonitor.cxx`), allowing integration on any level. For example invoking the monitor could be a functionality that a device offers.

FairMQ Shared Memory currently uses following names to register shared memory on the system:

`fairmq_shmem_main` - main segment name, used for user data (this name can be overridden via `--shm-segment-name`).
`fairmq_shmem_management` - management segment name, used for storing management data.
`fairmq_shmem_control_queue` - message queue for communicating between shm transport and shm monitor (exists independent of above segments).
`fairmq_shmem_mutex` - boost::interprocess::named_mutex for management purposes (exists independent of above segments).
