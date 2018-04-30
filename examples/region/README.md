Region example
==============

This example demonstrates the use of a more advanced feature - UnmanagedRegion, that can be used to create a buffer through one of FairMQ transports. The contents of this buffer are managed by the user, who can also create messages out of sub-buffers of the created buffer. Such feature can be interesting in environments that have special requirements by the hardware that writes the data, to keep the transfer efficient (e.g. shared memory).

