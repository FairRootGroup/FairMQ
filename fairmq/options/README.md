## FairMQParser

The FairMQParser configures the FairMQ channels from a JSON file.

The basic structure looks like this:

```json
{
    "fairMQOptions":
    {
        "devices":
        [{
            "id": "device1",
            "channels":
            [{
                "name": "data",
                "sockets":
                [{
                    "type": "push",
                    "method": "bind",
                    "address": "tcp://127.0.0.1:5555",
                    "sndBufSize": 1000,
                    "rcvBufSize": 1000,
                    "rateLogging": 1
                }]
            }]
        }]
    }
}
```

The top level key is `fairMQOptions`, followed by one or more devices (with their IDs), each containing one or more channels (with their names), each containing one or more sockets.

The socket parameters accept following values:
- `type` (default = ""): "push"/"pull", "pub"/"sub", "req"/"rep", "pair".
- `method` (default = ""): "bind"/"connect".
- `address` (default = ""): address to bind/connect.
- `sndBufSize` (default = 1000): socket send queue size in number of messages.
- `rcvBufSize` (default = 1000): socket receive queue size in number of messages.
- `sndKernelSize"` (default = ): 
- `rcvKernelSize"` (default = ): 
- `rateLogging` (default = 1): log socket transfer rates in seconds. 0 for no logging of this socket.
- `transport"` (default = ): override the default device transport for this channel.
- `linger"` (default = ): 
- `portRangeMin"` (default = ): 
- `portRangeMax"` (default = ): 
- `autoBind"` (default = ): 
- `numSockets"` (default = ): 

If a parameter is not specified, its default value will be set.

When a channel has multiple sockets, sockets can share common parameters. In this case specify the common parameters directly on the channel, which will be applied to all sockets of the channel. For example, the following config will create 3 sockets for the data channel with same settings, except the address:

```json
{
    "fairMQOptions":
    {
        "devices":
        [{
            "id": "device1",
            "channels":
            [{
                "name": "data",
                "type": "push",
                "method": "connect",
                "sockets":
                [
                    { "address": "tcp://127.0.0.1:5555" },
                    { "address": "tcp://127.0.0.1:5556" },
                    { "address": "tcp://127.0.0.1:5557" }
                ]
            }]
        }]
    }
}
```

The device ID should be unique within a topology. It is possible to create a configuration that can be shared by multiple devices, by specifying "key" instead of "id". To use it the started device must be started with `--config-key <key>` option. For example, the following config can be applied to multiple running devices within a topology:

```json
{
    "fairMQOptions":
    {
        "devices":
        [{
            "key": "processor",
            "channels":
            [{
                "name": "data",
                "socket":
                {
                    "type": "pull",
                    "method": "connect",
                    "address": "tcp://localhost:5555"
                }
            }]
        }]
    }
}
```
