QC
==

A topology consisting of 4 devices - Sampler, QCDispatcher, QCTask and Sink. The data flows from Sampler through QCDispatcher to Sink. On demand - by setting the corresponding configuration property - the QCDispatcher device will duplicate the data to the QCTask device. The property is set by the topology controller, in this example this is the `fairmq-dds-command-ui` utility.
