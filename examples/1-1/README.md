1-1: Sampler -> Sink
====================

A simple topology of two devices - **Sampler** and **Sink**. **Sampler** sends data to **Sink** via the **PUSH-PULL** pattern.

`runSampler.cxx` and `runSink.cxx` configure and run the devices.

The executables take two command line parameters: `--id` and `--channel-config`. The value of `--id` should be a unique identifier and the value for `--channel-config` is the configuration of the communication channel. .

For this and the following example, all the commands needed to start the device are contained in the `fairmq-start-ex-*.sh` script (that can also be used for starting the example).
