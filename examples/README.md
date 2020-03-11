# FairMQ Examples

Set of FairMQ examples. More examples that combine FairMQ with FairRoot can be found in the [FairRoot repository](https://github.com/FairRootGroup/FairRoot/tree/dev/examples/).

## 1-1

A simple topology of two devices - **Sampler** and **Sink**. **Sampler** sends data to **Sink** with the **PUSH-PULL** pattern.

## 1-n-1

A simple topology of three device types - **Sampler**, **Processor** and **Sink**. **Sampler** sends data to one or more **Processor**s, who modify the data and send it to one **Sink**. Transport with the **PUSH-PULL** pattern. The example also shows the configuration via JSON files, as oposed to `--channel-config` that is used by other examples.

## Built-in devices

Usage of generic devies provided with FairMQ.

## Copy & Push

A topology consisting of one **Sampler** and two **Sink**s. The **Sampler** uses the `Copy` method to send the same data to both sinks with the **PUSH-PULL** pattern. In countrary to the **PUB-SUB** pattern, this ensures that all receivers are connected and no data is lost, but requires additional channels to be configured.

## DDS

This example demonstrates usage of the Dynamic Deployment System ([DDS](http://dds.gsi.de/)) to dynamically deploy and configure a topology of devices. The topology is similar to those of Example 2, but now it can be easily distributed on different computing nodes without the need for manual reconfiguration of the devices.

## Multipart

This example shows how to send a multipart message from one device to the other. (two parts message parts - header and body).

## Multiple Channels

This example demonstrates how to work with multiple channels and multiplex between them.

## Multiple Transports

This examples shows how to combine different channel transports (zeromq/nanomsg/shmem) inside of one device and/or topology.

## n-m

A topology consisting of three layers of devices: synchronizer -> n * senders -> m * receivers.

## QC

A topology consisting of 4 devices - Sampler, QCProducer, QCConsumer and Sink. The data flows from Sampler through QCProducer to Sink. On demand - by setting the corresponding configuration property - the QCProducer device will duplicate the data to the QCConsumer device. The property is set by the topology controller, in this example this is the `fairmq-dds-command-ui` utility.

## Readout

Two example topologies of setups to be distributed to two kinds of nodes - detector readout node and processing node. Detector readout node contains readout process, data builder and data sender (and optionally an additional processor), while processing node contains data receiver devices. communication within readout nodes is done via unmanaged region through shared memory transport.

## Region

This example demonstrates the use of a more advanced feature - UnmanagedRegion, that can be used to create a buffer through one of FairMQ transports. The contents of this buffer are managed by the user, who can also create messages out of sub-buffers of the created buffer. Such feature can be interesting in environments that have special requirements by the hardware that writes the data, to keep the transfer efficient (e.g. shared memory).

## Request & Reply

This topology contains two devices that communicate with each other via the **REQ-REP** pettern. Bidirectional communication via a single socket.