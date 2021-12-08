FairMQ in a nutshell

Next-generation Particle Physics Experiments at FAIR and CERN are facing unprecedented data processing challenges. High detector data readout rates alone already require a non-trivial amount of distributed compute resources in the order of hundreds of servers per experiment. To achieve sufficient data compression rates in order to stay within the limits of economically feasible data storage capacity requirements, the complexity of tasks that need to be performed during the synchronous (online) data processing is significantly increased as well compared to previous generations of experiments.

The FairMQ C++ library is designed to aid the implementation of such large-scale online data processing workflows by
* providing an asynchronous message passing abstraction that integrates different existing data transport technologies (no need to re-invent the wheel),
* providing a reasonably efficient data transport service (zero-copy, high throughput - TCP, SHMEM, and RDMA implementations available),
* being data format agnostic (suitable data formats are usually very experiment-specific), and
* providing other basic building blocks such as a simple state machine based execution framework and a plugin mechanism to integrate with external config/control systems.

![Screenshot of AliceO2 Debug GUI showing the data processing workflow of a single event processing node](./AliceO2DebugGUIScreenshotEPN.png)

The screenshot shows a visualization of the data processing workflow on a single Alice event processing node (The O2DebugGUI tool in the screenshot is not part of FairMQ). Data continuously flows along the yellow edges (in this case via the FairMQ shmem data transport) through the various processing stages of which some are implemented as GPU and others as CPU algorithms.

(TODO Although FairMQ was initially designed for the synchronous (online) data processing, it has been used also to parallelize asynchronous (offline) simulation and analysis tasks. references?)
