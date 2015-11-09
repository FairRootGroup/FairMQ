Example 8: Sending Multipart messages
===============

A topology of two devices - Sampler and Sink, communicating with PUSH-PULL pattern.

The Sampler sends a multipart message to the Sink, consisting of two message parts - header and body.

Each message part is a regular FairMQMessage. To combine them into a multi-part message, simply send all but the last part with `SendPart()` and the last part with `Send()` as shown in the example.

The ZeroMQ transport guarantees delivery of both parts together. Meaning that when the Receive call of the Sink receives the first part, following parts have arrived too.

The header contains a simple data structure with one integer. The integer in this structure is used as a stop flag for the sink. As long as its value is 0, the Sink will keep processing the data. Once its value is 1, the Sink will exit its `Run()` method.

