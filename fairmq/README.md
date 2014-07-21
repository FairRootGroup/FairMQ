# FairMQ

The standard FairRoot is running all the different analysis tasks within one process. The FairMQ ([Message Queue](http://en.wikipedia.org/wiki/Message_queue)) allows starting tasks on different processes and provides the communication layer between these processes.

## Devices

The components encapsulating the tasks are called **devices** and derive from the common base class `FairMQDevice`.  FairMQ provides ready to use devices to organize the dataflow between the components (without touching the contents of a message), providing functionality like merging and splitting of the data stream (see subdirectory `devices`).

A number of devices to handle the data from the Tutorial3 detector of FairRoot are provided as an example and can be found in `FairRoot/base/MQ` directory. The implementation of the tasks run by these devices can be found `FairRoot/example/Tutorial3`. The implementation includes sending raw binary data as well as serializing the data with either [Boost Serialization](www.boost.org/doc/libs/release/libs/serialization/), [Google Protocol Buffers](https://developers.google.com/protocol-buffers/) or [Root TMessage](http://root.cern.ch/root/html/TMessage.html). Following the examples you can implement your own devices to transport arbitrary data.

## Topology

Devices are arranged into **topologies** where each device has a defined number of data inputs and outputs.

Example of a simple FairMQ topology:

![example of FairMQ topology](../docs/images/fairmq-example-topology.png?raw=true "Example of possible FairMQ topology")

Topology configuration is currently happening via setup scripts. This is very rudimentary and a much more flexible system is now in development. For now, example setup scripts can be found in directory `FairRoot/example/Tutorial3/` along with some additional documentation.

## Messages

Devices transport data between each other in form of `FairMQMessage`s. These can be filled with arbitrary content and transport either raw data or serialized data as described above. Message can be initialized in three different ways:
 - **with no parameters**: This is usefull for receiving a message, since neither size nor contents are yet known.
 - **given message size**: Initialize message body with a size and fill the contents later, either with `memcpy` or by writing directly into message memory.
 - **given message size and buffer**: initialize the message given an existing buffer. This is a zero-copy operation.

After sending the message, the queueing system takes over control over the message body and will free it with `free()` after it is no longer used. A callback can be given to the message object, to be called instead of the destruction with `free()`.

## Transport Interface

The communication layer is available through an interface. Two interface implementations are currently available. Main implementation uses the [ZeroMQ](http://zeromq.org) library. Alternative implementation relies on the [nanomsg](http://nanomsg.org) library. Here is an overview to give an idea how interface is implemented:

![FairMQ transport interface](../docs/images/fairmq-transport-interface.png?raw=true "FairMQ transport interface")

