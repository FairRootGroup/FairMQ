# Generic Devices

With FairMQ several generic devices are provided:

- **FairMQBenchmarkSampler**: generates random data of configurable size and at configurable rate and sends it out on an output channel.
- **FairMQSink**: receives messages on the input channel and simply discards them.
- **FairMQMerger**: receives data from multiple input channels and forwards it to a single output channel.
- **FairMQSplitter**: receives messages on a single input channels and round-robins them among multiple output channels (which can have different socket types).
- **FairMQMultiplier**: receives data from a single input channel and multiplies (copies) it to two or more output channels.
- **FairMQProxy**: connects input channel to output channel, where both can have different socket types and multiple peers.
