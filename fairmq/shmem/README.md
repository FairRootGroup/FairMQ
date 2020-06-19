# Shared Memory transport

Shared memory transport for FairMQ. To try with existing devices, run the devices with `--transport shmem` option or configure channel transport in JSON (see examples/MQ/multiple-transports).

The transport manages shared memory via boost::interprocess library. The transfer of the meta data, required to locate the content in the shared memory, is done via ZeroMQ. The transport supports all communication patterns where a single message is received by a single receiver. For multiple receivers for the same message, the message has to be copied.

Devices track and cleanup shared memory on shutdown. For more information on the current shared memory segment and additional cleanup options, see following section.

# Shared Memory objects / files

FairMQ Shared Memory currently uses the following names to register shared memory on the system:

| name                      | info                                           | created by         | used by                        |
| ------------------------- | ---------------------------------------------- | ------------------ | ------------------------------ |
| `fmq_<shmId>_main`        | main segment (user data)                       | one of the devices | devices                        |
| `fmq_<shmId>_mng`         | management segment (management data)           | one of the devices | devices                        |
| `fmq_<shmId>_mtx`         | mutex                                          | one of the devices | devices                        |
| `fmq_<shmId>_cv`          | condition variable                             | one of the devices | devices with unmanaged regions |
| `fmq_<shmId>_rg_<index>`  | unmanaged region(s)                            | one of the devices | devices with unmanaged regions |
| `fmq_<shmId>_rgq_<index>` | unmanaged region queue(s)                      | one of the devices | devices with unmanaged regions |
| `fmq_<shmId>_ms`          | shmmonitor status                              | shmmonitor         | devices, shmmonitor            |
| `fmq_<shmId>_cq`          | message queue between transport and shmmonitor | shmmonitor         | devices, shmmonitor            |

The shmId is generated out of session id and user id.

## Shared memory monitor

The shared memory monitor tool, supplied with the shared memory transport can be used to monitor shared memory use and automatically cleanup shared memory in case of device crashes.

With default arguments the monitor will run indefinitely with no output, and clean up shared memory segment if it is open and no heartbeats from devices arrive within a timeout period. It can be further customized with following parameters:

  `--session <arg>`: for which session to run the monitor (default is "default"). The actual ressource names will be built out of session id, user id (hashed and truncated).
  `--cleanup`: start monitor, perform cleanup of the memory and quit.
  `--shmid <arg>`: if provided, this shmem id will be used instead of the one generated from session id. Use this if you know the name of the shared memory ressource, but do not have the used session id.
  `--self-destruct`: run until the memory segment is closed (either naturally via cleanup performed by devices or in case of a crash (no heartbeats within timeout)).
  `--interactive`: run interactively, with detailed segment details and user input for various shmem operations.
  `--timeout <arg>`: specifiy the timeout for the heartbeats from shmem transports in milliseconds (default 5000).

The options can be combined, with the exception of `--cleanup` option, which will invoke the described behaviour independent of other options.
Without the `--self-destruct` option, the monitor will run continuously, moitoring (and cleaning up if needed) consecutive topologies.

Possible further implementation would be to run the monitor with `--self-destruct` with each topology.

The Monitor class can also be used independently from the supplied executable (built from `runMonitor.cxx`), allowing integration on any level. For example invoking the monitor could be a functionality that a device offers.
