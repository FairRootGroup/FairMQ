Example 3: DDS
===============

This example demonstrates usage of the Dynamic Deployment System ([DDS](http://dds.gsi.de/)) to dynamically deploy and configure a topology of devices. The topology is similar to those of Example 2, but now it can be easily distributed on different computing nodes without the need for manual socket reconfiguration of the devices.

To make use of DDS functionality the example executables need to find the DDS plugin libraries that are compiled with FairRoot when FairRoot find DDS installed. DDS location is given to CMake as follows:

```bash
cmake -DDDS_PATH="/path/to/dds/install/dir/" ..
```

The description below outlines the minimal steps needed to run the example with DDS. For general DDS help please refer to DDS documentation on [DDS Website](http://dds.gsi.de/).

##### 1. The device handles the socket addresses and ports configuration via DDS Plugin.

It is sufficient to provide the `--config <ddsConfigPluginLibraryName>` (and `--control <ddsControlPluginLibraryName>` if state machine control is to be handled with DDS) command line arguments to let the devices be configured dynamically. No code changes in the device are necessary. See the XML topology file for example of using the command line arguments.

##### 2. Write DDS hosts file that contains a list of worker nodes to run the topology on (When deploying using the SSH plug-in).

We run this example on the local machine for simplicity. The file below defines three workers, sampler, processor and sink, with a total of 12 DDS agents (thus able to accept 12 tasks). The parameters for each worker node are:
 - user-chosen worker ID (must be unique)
 - a host name with or without a login, in a form: login@host.fqdn (password-less SSH access to these hosts must be possible)
 - additional SSH params (can be empty)
 - a remote working directory (most exist on the worker nodes)
 - number of DDS Agents for this worker

```bash
@bash_begin@
#source setup.sh
@bash_end@

sampler, username@localhost, , /path/to/dds-work-dir/, 1
processor, username@localhost, , /path/to/dds-work-dir/, 10
sink, username@localhost, , /path/to/dds-work-dir/, 1
```

##### 3. Write DDS topology file that describes which tasks (processes) to run and their topology and configuration.

Take a look at `ex3-dds-topology.xml`. It consists of a definition part (properties, tasks, collections and more) and execution part (main). In our example Sampler, Processor and Sink tasks are defines, containing their executables and exchanged properties. The `<main>` of the topology uses the defined tasks. Besides one Sampler and one Sink task, a group containing Processor task is defined. The group has a multiplicity of 10, meaninig 10 Processors will be executed. Each of the Processors will receive the properties with Sampler and Sink addresses.

The configuration of the channel connection addresses is done by the DDS plugin via the channel names. The task property names must correspond to the channel names (data1, data2), with binding channels writing the properties and connecting channel reading the properties (see the example XML and JSON files).

If `eth0` network interface (default for binding) is not available on your system, specify another one in the topology file for each task. For example: `--network-interface lo0`.

##### 4. Start DDS server.

The DDS server is started with:

```bash
dds-server start -s
```

##### 5. Submit DDS Agents (configured in the hosts file).

Agents are submitted with:
```bash
dds-submit --rms ssh --config ex3-dds-hosts.cfg
```
The `--rms` option defines a destination resource management system. The `--config` specifies an SSH plug-in resource definition file.

##### 6. Activate the topology.

```bash
dds-topology --activate ex3-dds-topology.xml
```

##### 7. Run

After activation, agents will execute the defined tasks on the worker nodes. Output of the tasks will be stored in the directory that was specified in the hosts file.

##### 8. (optional) Use example command UI to check state of the devices

A simple utility (fairmq-dds-command-ui) is included with FairRoot to send commands to devices and receive replies from them. The utility uses the DDS intercom library to send "check-state" string to all devices, to which they reply with their ID and state they are in. The utility also allows requesting state changes from devices. To let the device listen to the commands from the utility, start the device with `--control <ddsControlPluginLibraryName>` cmd option (see example XML topology).

To see it in action, start the fairmq-dds-command-ui while the topology is running.

##### 9. Stop DDS server/topology.

The execution of tasks can be stopped with:
```bash
dds-topology --stop
```
Or by stopping the DDS server:
```bash
dds-server stop
```

For general DDS documentation please refer to [DDS Website](http://dds.gsi.de/).
