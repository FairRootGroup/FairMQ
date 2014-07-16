fairmq
========

The standard FairRoot is running all the different analysis tasks within one process. The FairMQ ([Message Queue](http://en.wikipedia.org/wiki/Message_queue)) allows starting tasks on different processes and provides the communication layer between these processes. 

The underlying communication layer in the FairMQ is now provided by:

- [ZeroMQ](http://zeromq.org);
- [NanoMSG](http://nanomsg.org).