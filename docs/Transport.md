← [Back](../README.md)

# 2. Transport Interface

The communication layer is available through the transport interface. Three interface implementations are currently available. Main implementation uses the [ZeroMQ](http://zeromq.org) library. Second transport implementation is using shared memory via boost::interprocess & ZeroMQ combination.

Here is an overview to give an idea how the interface is implemented:

![FairMQ transport interface](images/transport_interface.png?raw=true "FairMQ transport interface")

Currently, the transports have been tested to work with these communication patterns:

|               | zeromq | shmem |
| ------------- |--------| ----- |
| PAIR          | yes    | yes   |
| PUSH/PULL     | yes    | yes   |
| PUB/SUB       | yes    | no    |
| REQ/REP       | yes    | yes   |

The next table shows the supported address types for each transport implementation:

|             | zeromq | shmem | comment                                       |
| ----------- | ------ | ----- | --------------------------------------------- |
| `inproc://` | yes    | yes   | in process: useful for unit testing           |
| `ipc://`    | yes    | yes   | inter process comm: useful on single machine  |
| `tcp://`    | yes    | yes   | useful for any communication, local or remote |

## 2.1 Message

Devices transport data between each other in form of `fair::mq::Message`s. These can be filled with arbitrary content. Message can be initialized in three different ways by calling `NewMessage()`:

```cpp
fair::mq::MessagePtr NewMessage() const;
```
**with no parameters**: Initializes an empty message (typically used for receiving).

```cpp
fair::mq::MessagePtr NewMessage(const size_t size) const;
```
**given message size**: Initializes message body with a given size. Fill the created contents via buffer pointer.

```cpp
using fairmq_free_fn = void(void* data, void* hint);
fair::mq::MessagePtr NewMessage(void* data, const size_t size, fairmq_free_fn* ffn, void* hint = nullptr) const;
```
**given existing buffer and a size**: Initialize the message from an existing buffer. In case of ZeroMQ this is a zero-copy operation.

Additionally, FairMQ provides two more message factories for convenience:
```cpp
template<typename T>
fair::mq::MessagePtr NewSimpleMessage(const T& data) const
```
**copy and own**: Copy the `data` argument into the returned message and take ownership (free memory after message is sent). This interface is useful for small, [trivially copyable](http://en.cppreference.com/w/cpp/concept/TriviallyCopyable) data.

```cpp
template<typename T>
fair::mq::MessagePtr NewStaticMessage(const T& data) const
```
**point to existing memory**: The returned message will point to the `data` argument, but not take ownership (someone else must destruct this variable). Make sure that `data` lives long enough to be successfully sent. This interface is most useful for third party managed, contiguous memory (Be aware of shallow types with internal pointer references! These will not be sent.)

## 2.1.1 Ownership

The component of a program, that is reponsible for the allocation or destruction of data in memory, is taking ownership over this data. Ownership may be passed along to another component. It is also possible that multiple components share ownership of data. In this case, some strategy must be in place to determine the last user of the data and assign her the responsibility of destruction.

After queuing a message for sending in FairMQ, the transport takes ownership over the message body and will free it with `free()` after it is no longer used. A callback can be passed to the message object, to be called instead of the destruction with `free()` (for initialization via buffer+size).

```cpp
static void fair::mq::NoCleanup(void* /*data*/, void* /*obj*/) {}

template<typename T>
static void fair::mq::SimpleMsgCleanup(void* /*data*/, void* obj) { delete static_cast<T*>(obj); }
```
For convenience, two common deleter callbacks are already defined in the `fair::mq::TransportFactory` class to aid the user in controlling ownership of the data.

## 2.2 Channel

A channel represents a communication endpoint in FairMQ. Usage is similar to a traditional Unix network socket. A device usually contains a number of channels that can either listen for incoming connections from channels of other devices or they can connect to other listening channels. Channels are organized by a channel name and a subchannel index.

```cpp
const fair::mq::Channel& GetChannel(const std::string& channelName, const int index = 0) const;
```

All subchannels with a common channel name need to be of the same transport type.

## 2.3 Poller

A poller allows to wait on multiple channels either to receive or send a message.

```cpp
fair::mq::PollerPtr NewPoller(const std::vector<const fair::mq::Channel*>& channels)
```
**list channels**: This poller waits on all supplied channels. Currently, it is limited to channels of the same transport type only.

← [Back](../README.md)
