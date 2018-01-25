← [Back](../README.md)

# 4. Development

## 4.1 Testing

For unit testing it is often not feasible to boot up a full-blown distributed system with dozens of processes.

In some scenarios it is useful to not even instantiate a `FairMQDevice` at all. Please see [this example](../test/protocols/_push_pull_multipart.cxx) for single and multi threaded unit test without a device instance. If you store your transport factories and channels on the heap, pls make sure, you destroy the channels before you destroy the related transport factory for proper shutdown. Channels provide all the `Send/Receive` and `New*Message/New*Poller` APIs provided by the device too.

TODO Multiple devices in one process.

← [Back](../README.md)
