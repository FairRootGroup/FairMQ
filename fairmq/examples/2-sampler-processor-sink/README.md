Example 2: Sampler -> Processor -> Sink
===============

A simple topology of three devices - **Sampler**, **Processor** and **Sink**. **Sampler** sends data to one or more **Processor**s, who modify the data and send it to one **Sink**. Transport with the **PUSH-PULL** pattern.

In this example the Sampler is configured to **bind** its output and the Sink is configured to also **bind** its input. This allows us run any number of processors with the same configuration, because they all connect to same Sampler and Sink addresses. Furthermore, it allows adding of processors dynamically during run-time. The PUSH and PULL sockets will handle the data distribution to/from the new devices according to their distribution strategies ([Round-robin output for PUSH](http://api.zeromq.org/4-0:zmq-socket#toc14) and [Fair-queued input for PULL](http://api.zeromq.org/4-0:zmq-socket#toc15)).

The Sampler sends out a simple text string (its content configurable with `--text` command line parameter, defaul is "Hello"). Each Processor modifies the string by appending its ID to it and send it to the Sink.

The provided configuration file contains two Processors. To add more Processors, you can either extend the configuration file, or create a separate file only for new processors.
