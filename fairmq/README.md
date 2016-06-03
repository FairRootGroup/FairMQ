# FairMQ

The standard FairRoot is running all the different analysis tasks within one process. The FairMQ ([Message Queue](http://en.wikipedia.org/wiki/Message_queue)) allows starting tasks on different processes and provides the communication layer between these processes.

## Devices

The components encapsulating the tasks are called **devices** and derive from the common base class `FairMQDevice`. FairMQ provides ready to use devices to organize the dataflow between the components (without touching the contents of a message), providing functionality like merging and splitting of the data stream (see subdirectory `devices`).

## Topology

Devices are arranged into **topologies** where each device has a defined number of data inputs and outputs.

Example of a simple FairMQ topology:

![example of FairMQ topology](../docs/images/fairmq-example-topology.png?raw=true "Example of possible FairMQ topology")

Within a topology each device needs a unique id (given to it via required command line option `--id`).

Topology configuration is currently happening via setup scripts. This is very rudimentary and a much more flexible system is now in development. For now, example setup scripts can be found in directory `FairRoot/example/Tutorial3/` along with some additional documentation.

## Communication Patterns

FairMQ devices communicate via the communication patterns offered by ZeroMQ (or nanomsg): PUSH-PULL, PUB-SUB, REQ-REP, PAIR, [more info here](http://api.zeromq.org/4-0:zmq-socket). Each transport may provide further patterns.

## Messages

Devices transport data between each other in form of `FairMQMessage`s. These can be filled with arbitrary content. Message can be initialized in three different ways:
- **with no parameters**: Initializes an empty message (typically used for receiving).
- **given message size**: Initializes message body with a given size. Fill the created contents via buffer pointer.
- **given existing buffer and a size**: Initialize the message from an existing buffer. In case of ZeroMQ this is a zero-copy operation.

After sending the message, the transport takes over control over the message body and will free it with `free()` after it is no longer used. A callback can be given to the message object, to be called instead of the destruction with `free()` (for initialization via buffer+size).

## Transport Interface

The communication layer is available through an interface. Three interface implementations are currently available. Main implementation uses the [ZeroMQ](http://zeromq.org) library. Alternative implementation relies on the [nanomsg](http://nanomsg.org) library. Third transport implementation is using shared memory via boost::interprocess & ZeroMQ combination.

Here is an overview to give an idea how interface is implemented:

![FairMQ transport interface](../docs/images/fairmq-transport-interface.png?raw=true "FairMQ transport interface")

Currently, the transports have been tested to work with these communication patterns:

|               | ZeroMQ | nanomsg | Shared Memory |
| ------------- |--------| ------- | ------------- |
| PAIR          | yes    | yes     | yes           |
| PUSH/PULL     | yes    | yes     | yes           |
| PUB/SUB       | yes    | yes     | no            |
| REQ/REP       | yes    | yes     | yes           |

## State Machine

Each FairMQ device has an internal state machine:

![FairMQ state machine](../docs/images/fairmq-states.png?raw=true "FairMQ state machine")

The state machine can be querried and controlled via `GetCurrentStateName()` and `ChangeState("<state name>")` methods. Only legal state transitions are allowed (see image above). Illegal transitions will fail with an error.

If the device is running in interactive mode (default), states can be changed via keyboard input:

 - `'h'` - help
 - `'p'` - pause
 - `'r'` - run
 - `'s'` - stop
 - `'t'` - reset task
 - `'d'` - reset device
 - `'q'` - end
 - `'j'` - init task
 - `'i'` - init device

Without the interactive mode, for example for a run in background, two other control mechanisms are available:

 - static (`--control static`) - device goes through a simple init -> run -> reset -> exit chain.
 - dds (`--control dds`) - device is controled by external command, in this case using dds commands (fairmq-dds-command-ui).

## Examples

A collection of simple examples in `FairRoot/examples/MQ` directory demonstrates some common usage patterns of FairMQ.

A number of devices to handle the data from the Tutorial3 FairTestDetector of FairRoot are provided as an example and can be found in `FairRoot/base/MQ` directory. The implementation of the tasks run by these devices can be found `FairRoot/examples/advanced/Tutorial3`. The implementation includes sending raw binary data as well as serializing the data with either [Boost Serialization](http://www.boost.org/doc/libs/release/libs/serialization/), [Google Protocol Buffers](https://developers.google.com/protocol-buffers/) or [Root TMessage](http://root.cern.ch/root/html/TMessage.html). Following the examples you can implement your own devices to transport arbitrary data.
