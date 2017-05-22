# FairMQ

Standard FairRoot is running all the different analysis tasks within one process. FairMQ ([Message Queue](http://en.wikipedia.org/wiki/Message_queue)) allows starting tasks on different processes and provides the communication layer between these processes.

1. [Device](docs/Device.md#1-device)
   1. [Topology](docs/Device.md#11-topology)
   2. [Communication Patterns](docs/Device.md#12-communication-patterns)
   3. [State Machine](docs/Device.md#13-state-machine)
2. [Transport Interface](docs/Transport.md#2-transport-interface)
   1. [Message](docs/Transport.md#21-message)
      1. [Ownership](docs/Transport.md#211-ownership)
   2. [Channel](docs/Transport.md#22-channel)
   3. [Poller](docs/Transport.md#23-poller)
3. [Development](docs/Development.md#3-development)
   1. [Testing](docs/Development.md#31-testing)
4. [Examples](docs/Examples.md#4-examples)
