# Generic Devices

With FairMQ several generic devices are provided:

- **BenchmarkSampler**: generates random data of configurable size and at configurable rate and sends it out on an output channel.
- **Sink**: receives messages on the input channel and simply discards them.
- **Merger**: receives data from multiple input channels and forwards it to a single output channel.
- **Splitter**: receives messages on a single input channels and round-robins them among multiple output channels (which can have different socket types).
- **Multiplier**: receives data from a single input channel and multiplies (copies) it to two or more output channels.
- **Proxy**: connects input channel to output channel, where both can have different socket types and multiple peers.
