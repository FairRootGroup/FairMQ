# FairMQ Request-Reply Example

This example demonstrates usage of the request-reply pattern together with FairMQ. Two processes - example_client and example_server communicate. The client sends a text string and the server respondes by returning the string back to the client. The communication happens over a **single** REP-REP socket. Works both with ZeroMQ and with nanomsg transport.
