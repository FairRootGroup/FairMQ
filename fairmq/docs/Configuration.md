← [Back](../README.md)

# 3. Configuration

## 3.1 Device Configuration

Devices receive configuration primarily via provided command line options (that can be extended per device).

## 3.2 Communication Channels Configuration

The communication channels can be configured via configuration parsers. The parser system is extendable, so if provided parsers do not suit your style, you can write your own and plug them in the configuration system.

The provided parsers are:

### 3.2.1 JSON Parser

This parser reads channel configuration from a JSON file. Example:

```JSON
{
    "fairMQOptions": {
        "devices": [
            {
                "id": "sampler1",
                "channels": [
                    {
                        "name": "data",
                        "sockets": [
                            {
                                "type": "push",
                                "method": "bind",
                                "address": "tcp://*:5555",
                                "sndBufSize": 1000,
                                "rcvBufSize": 1000,
                                "rateLogging": 1
                            }
                        ]
                    }
                ]
            },
            {
                "id": "sink1",
                "channels": [
                    {
                        "name": "data",
                        "sockets": [
                            {
                                "type": "pull",
                                "method": "connect",
                                "address": "tcp://localhost:5555",
                                "sndBufSize": 1000,
                                "rcvBufSize": 1000,
                                "rateLogging": 1
                            }
                        ]
                    }
                ]
            }
        ]
    }
}
```

The JSON file can contain configuration for multiple devices.

- The mapping between device and configuration happens via the device ID (the `--id` parameter of the launched device and the `"id"` entry in the config).
- Instead of `"id"`, JSON file may contain device configurations under `"key"`, which allows launched devices to share configuration, e.g.: `my-device-executable --id <device-id> --config-key <config-key>`.
- Socket options must contain at least *type*, *method* and *address*, the rest of the values are optional and will get default values of the channel.
- If a channel has multiple sub-channels, common properties can be defined under channel directly, and will be shared by all sub-channels, e.g.:

```JSON
"channels": [{
    "name": "data",
    "type": "push",
    "method": "bind",
    "sockets": [{
        "address": "tcp://*:5555",
        "address": "tcp://*:5556",
        "address": "tcp://*:5557"
    }]
}]
```

### 3.2.2 SuboptParser

This parser configures channels directly from the command line.
The parser handles a comma separated key=value list format by using the getsubopt function of the standard library.
The option key `--channel-config` can be used with the list of key/value pairs, e.g.:

```
--channel-config name=output,type=push,method=bind,address=tcp://127.0.0.1:5555
```

← [Back](../README.md)
