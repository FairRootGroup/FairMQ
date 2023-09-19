## Shared Memory transport

Shared memory transport for FairMQ. To try with existing devices, run the devices with `--transport shmem` option or configure channel transport in JSON (see examples/MQ/multiple-transports).

The transport manages shared memory via boost::interprocess library. The transfer of the meta data, required to locate the content in the shared memory, is done via ZeroMQ. The transport supports all communication patterns where a single message is received by a single receiver. For multiple receivers for the same message, the message has to be copied.

Devices track and cleanup shared memory on shutdown. For more information on the current shared memory segment and additional cleanup options, see following section.

## Shared Memory objects / files

FairMQ Shared Memory currently uses the following names to register shared memory on the system:

| name                        | info                                           | created by         | used by                        |
| --------------------------- | ---------------------------------------------- | ------------------ | ------------------------------ |
| `fmq_<shmId>_m_<segmentId>` | managed segment(s) (user data)                 | one of the devices | devices                        |
| `fmq_<shmId>_mng`           | management segment (management data)           | one of the devices | devices                        |
| `fmq_<shmId>_rg_<index>`    | unmanaged region(s)                            | one of the devices | devices with unmanaged regions |
| `fmq_<shmId>_rgq_<index>`   | unmanaged region queue(s)                      | one of the devices | devices with unmanaged regions |
| `fmq_<shmId>_rrc_<index>`   | unmanaged region reference count pool(s)       | one of the devices | devices with unmanaged regions |
| `fmq_<shmId>_ms`            | shmmonitor status                              | shmmonitor         | devices, shmmonitor            |

The shmId is generated out of session id and user id.

## Shared memory monitor

The shared memory monitor tool (`fairmq-shmmonitor`) can be used to monitor and cleanup the created shared memory.

Most commands act for the specified session, identified either via session id (`--session`,`-s`) or shmid (`--shmid`).

The monitor runs in one of the following modes:

| command                     | action                                         |
| --------------------------- | ---------------------------------------------- |
| no args                     | Print segment info of the specified session/shm ID and exit. |
| `--view`,`-v`               | Print segment info of the specified session/shm ID and exit. |
| `--interactive`,`-i`        | Print segment info of the specified session/shm ID at a given interval (`--interval`), with some keyboard controls. Can be combined with `--view` for read-only access (and avoid receiving heartbeats). |
| `--monitor`,`-m`            | Monitor the session shm usage by receiving heartbeats from shmem users, cleaning it up if no heartbeats arrived within configured timeout (`--timeout`/`-t`). Only one heartbeat receiver per session is currently possible. If `--self-destruct`/`-x` is added, monitor will exit either when (a) no shm has been observed for interval * 2, (b) a cleanup due to reached timeout has been performed, or (c) shm has been observed, but is now cleaned up. |
| `--cleanup`,`-c`            | Cleanup the shm for the specified session and exit. |
| `--debug`,`-b`              | Print the list of messages in the current session and exit. Only availabe when FairMQ is compiled with `FAIRMQ_DEBUG_MODE=ON` (high performance impact). |
| `--get-shmid`               | Translate given session id and user id (`--user-id`) to a shmem id (uses current user id if none provided) and exit. |
| `--list-all`                | Print segment info for all sessions present on the system and exit. |

Additional cmd options:

| command                     | action                                         |
| --------------------------- | ---------------------------------------------- |
| `--cleanup-on-exit`         | Perform a cleanup on exit, when running in monitoring or interactive mode. |
| `--daemonize`,`-d`          | Can be combined with the monitoring mode to detach the process from the parent. |
| `--verbose`,`-d`            | When running as a daemon, store monitor output in `fairmq-shmmonitor_<timestamp>.log` |

For full option details, run with `-h`.

The Monitor class can also be used independently from the supplied executable, allowing integration on any level.

## Troubleshooting

Bus Error (SIGBUS) can occur if the transport tries to access shared memory that is not accessible. One reason could be because the used memory in the segment exceeds the capacity or available memory of the shmem filesystem (capacity is by default set to half of RAM on Linux).

## Shared Memory cleanup

On a graceful shutdown of all devices, shared memory transport removes all created shared memory files. In case of a crash however, the cleanup cannot be guaranteed. Following possibilites are available to perform cleanup in case of crashes:

- For execution with Slurm a [job_container](https://slurm.schedmd.com/job_container.conf.html) can be used, which can isolate the shm and remove them upon job finish. This is currently the recommended approach.
- a [trap](https://www.man7.org/linux/man-pages/man1/trap.1p.html) in an executing script on ERR/EXIT with a call to `fairmq-shmmonitor -c -s <sessionid>`. This would not work if the script is killed with -SIGKILL or similar fashion where it cannot call the trap.
- [CTest cleanup fixture](https://cmake.org/cmake/help/latest/prop_test/FIXTURES_CLEANUP.html) with a call to `fairmq-shmmonitor -c -s <sessionid>`. This would work for an ongoing ctest run, but would not be called if a test run is interrupted, e.g. by SIGINT.
- manual cleanup of the files listed [above](#shared-memory-objects--files).
- Launch devices with `--shm-monitor true`. This will launch a daemon. The daemon will then listen for heartbeats from devices (every 100ms) and if none are received within 2000ms, will clean the memory. This is unreliable because the daemon can also be killed by a strict enough controller. But also if for some reason there are significant delays in the heartbeats, shmem could end up being cleaned before it should be.
