# Shared Memory transport

First version of the shared memory transport. To try with existing devices, run the devices with `--transport shmem` option.

The transport manages shared memory via boost::interprocess library. The transfer of the meta data, required to locate the content in the share memory, is done via ZeroMQ. The transport supports all communication patterns where a single message is received by a single receiver. For multiple receivers for the same message, the message has to be copied.

Under development:
- Cleanup of the shared memory segment in case all devices crash. Currently at least one device has to stop properly for a cleanup.
