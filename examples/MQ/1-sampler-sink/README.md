Example 1: Sampler -> Sink
===============

A simple topology of two devices - **Sampler** and **Sink**. **Sampler** sends data to **Sink** via the **PUSH-PULL** pattern.

`runExample1Sampler.cxx` and `runExample1Sink.cxx` configure and run the devices in their main function.

The executables take two required command line parameters: `--id` and `--mq-config`. The value of `--id` should be a unique identifier and the value for `--mq-config` a path to a config file. The config file for this example is `ex1-sampler-sink.json` and it contains configuration for the communication channels of the devices. The mapping between a specific device and the configuration (which can contain multiple devices) is done based on the **id**.

For this and the following example, all the commands needed to start the device are contained in the startFairMQExN.sh script (that can also be used for starting the example).
