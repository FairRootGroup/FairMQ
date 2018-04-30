Sending Multipart messages
==========================

A topology of two devices - Sampler and Sink, communicating with PUSH-PULL pattern.

The Sampler sends a multipart message to the Sink, consisting of two message parts - header and body.

Each message part is a regular FairMQMessage. To combine them into a multi-part message use `FairMQParts`. Add messages to `FairMQParts` with `AddPart` method.

All parts are guaranteed to be delivered together. The Receive call in the sink will recive the entire parts structure.

The header contains a simple data structure with one integer. The integer in this structure is used as a stop flag for the sink. As long as its value is 0, the Sink will keep processing the data. Once its value is 1, the data handler method of the Sink will return false, which will exit the RUNNING state of the device.
