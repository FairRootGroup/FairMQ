← [Back](../README.md)

# 6. Examples

A collection of simple examples in `FairRoot/examples/MQ` directory demonstrates some common usage patterns of FairMQ.

A number of devices to handle the data from the Tutorial3 FairTestDetector of FairRoot are provided as an example and can be found in `FairRoot/base/MQ` directory. The implementation of the tasks run by these devices can be found `FairRoot/examples/advanced/Tutorial3`. The implementation includes sending raw binary data as well as serializing the data with either [Boost Serialization](http://www.boost.org/doc/libs/release/libs/serialization/), [Google Protocol Buffers](https://developers.google.com/protocol-buffers/) or [Root TMessage](http://root.cern.ch/root/html/TMessage.html). Following the examples you can implement your own devices to transport arbitrary data.

← [Back](../README.md)
