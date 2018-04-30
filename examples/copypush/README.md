Copy & Push
===========

A topology consisting of one **Sampler** and two **Sink**s. The **Sampler** uses the `Copy` method to send the same data to both sinks with the **PUSH-PULL** pattern. In contrary to the **PUB-SUB** pattern, this ensures that all receivers are connected and no data is lost, but requires additional sockets.
