Example 3: DDS
===============

This example demonstrates usage of the Dynamic Deployment System ([DDS](http://dds.gsi.de/)) to dynamically deploy and configure a topology of devices. The topology is similar to those of Example 2, but now it can be easily distributed on different computing nodes without the need for manual reconfiguration of the devices.

The description below outlines the minimal steps needed to run the example. For more detailed DDS documentation please visit [DDS Website](http://dds.gsi.de/).

The topology run by DDS is defined in `ex3-dds-topology.xml` and the hosts to run it on are configured in `ex3-dds-hosts.cfg`. The topology starts one Sampler, one Sink and a group of 10 Processors. 