Multiple Transports example
===========================

This example demonstrates use of multiple transports (zeromq/nanomsg/shmem) within the same topology and/or device. It is a simple topology consisting of two samplers and a sink. The devices are connected via 3 channels:

![Multiple Transports example](../../../docs/images/fairmq-ex-multiple-transports.png?raw=true "Multiple Transports example")

Each device has main transport that it uses. By default it is ZeroMQ, and can be overriden via the `--transport` cmd option. The device will initialize additional transports if any of the channels have them configured (e.g. via the JSON file, see `ex-multiple-transports.json`).

In this example sampler1 and sink are started with `--transport shmem`, making shared memory their main transport, sampler2 with `--transport nanomsg`. Additionally, the ack channel is configured to use zeromq as its transport via the JSON configuration.

The main two things that a transport does is transfer of data and allocation of memory for the messages. By default, new messages are created via the main device transport. If a message has been created with one transport and is to be transferred with another, it has to be copied into a new message of the target transport. This happens automatically behind the scenes. To avoid this copy the device can create messages via `NewMessageFor(const string& channelName, int subChannelIndex, ...)` method, that creates the messages via the transport of the given channel (check sampler1 and sink for an example).
