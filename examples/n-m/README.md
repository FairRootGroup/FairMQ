N-M
==========================

A topology consisting of three layers of devices: synchronizer -> sender(s) -> receiver(s). Senders distribute data to receivers based on the data id contained in the message from the synchronizer (same id goes to the same receiver from every sender). The senders send the data in a non-blocking fashion - if queue is full or receiver is down, data is discarded. Two configurations are provided - one using push/pull channels between senders/receivers, another using pair channels. In push/pull case there is only one receiving channel on the receiver device. In pair case there are as many receiver (sub-)channels as there are senders.
