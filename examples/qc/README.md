QC
==

A topology consisting of 4 devices - Sampler, QCProducer, QCConsumer and Sink. The data flows from Sampler through QCProducer to Sink. On demand - by setting the corresponding configuration property - the QCProducer device will duplicate the data to the QCConsumer device. The property is set by the topology controller, in this example this is the `fairmq-dds-command-ui` utility.
