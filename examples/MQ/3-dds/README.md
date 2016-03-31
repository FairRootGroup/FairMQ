Example 3: DDS
===============

This example demonstrates usage of the Dynamic Deployment System ([DDS](http://dds.gsi.de/)) to dynamically deploy and configure a topology of devices. The topology is similar to those of Example 2, but now it can be easily distributed on different computing nodes without the need for manual socket reconfiguration of the devices.

This example is compiled only if the DDS is found by CMake. Custom DDS installation location can be given to CMake like this:

```bash
cmake -DDDS_PATH="/path/to/dds/install/dir/" ..
```

The description below outlines the minimal steps needed to run the example with DDS. For more details please refer to DDS documentation on [DDS Website](http://dds.gsi.de/).

##### 1. After beginning the initialization, the device handles the socket addresses and ports distribution via DDS.

The binding channels give their bound addresses to devices interested in connecting to it and connecting sockets wait to receive these addresses. This match happens via the properties specified in the JSON file, which replace addresses in the DDS run. This is done behind the scenes after the initialization has been started and can be called with a single method call:

```C++
sampler.ChangeState("INIT_DEVICE");
HandleConfigViaDDS(sampler);
sampler.WaitForEndOfState("INIT_DEVICE");
```

In most cases a device will land on a random node and all the addresses and ports are configured dynamicaly. The JSON file does not contain any address information for a DDS run. Instead, addresses are exchanged between the devices dynamically based on the provided property names. E.g. here a processor communicates with the sampler via the *data1* channel. Sampler (binding) communicates its address to the processor(s) (connecting) via the "samplerAddr" property (see `ex3-dds.json` file).

##### 3. Write DDS hosts file that contains a list of worker nodes to run the topology on (When deploying using the SSH plug-in).

We run this example on the local machine for simplicity. The file below defines one worker `wn0` with 12 DDS Agents (thus able to accept 12 tasks). The parameters for each worker node are:
 - user-chosen worker ID (must be unique)
 - a host name with or without a login, in a form: login@host.fqdn (password-less SSH access to these hosts must be possible)
 - additional SSH params (can be empty)
 - a remote working directory (most exist on the worker nodes)
 - number of DDS Agents for this worker

```bash
@bash_begin@
echo "DBG: SSH ENV Script"
#source setup.sh
@bash_end@

wn0, username@localhost, , /tmp/, 12
```

##### 4. Write DDS topology file that describes which tasks (processes) to run and their topology and configuration.

Take a look at `ex3-dds-topology.xml`. It consists of a definition part (properties, tasks, collections and more) and execution part (main). In our example Sampler, Processor and Sink tasks are defines, containing their executables and exchanged properties. The `<main>` of the topology uses the defined tasks. Besides one Sampler and one Sink task, a group containing Processor task is defined. The group has a multiplicity of 10, meaninig 10 Processors will be executed. Each of the Processors will receive the properties with Sampler and Sink addresses.

If `eth0` network interface (default for binding) is not available on your system, specify another one in the topology file for each task. For example: `--network-interface lo0`.

##### 5. Start DDS server.

The DDS server is started with:

```bash
dds-server start -s
```

##### 6. Submit DDS Agents (configured in the hosts file).

Agents are submitted with:
```bash
dds-submit --rms ssh --config ex3-dds-hosts.cfg
```
The `--rms` option defines a destination resource management system. The `--config` specifies an SSH plug-in resource definition file. 

##### 7. Set the topology file.

Point DDS to the topology file:
```bash
dds-topology --set ex3-dds-topology.xml
```

##### 8. Activate the topology.

```bash
dds-topology --activate
```

##### 9. Run

After activation, agents will execute the defined tasks on the worker nodes. Output of the tasks will be stored in the directory that was specified in the hosts file.

##### 10. (optional) Use example command UI to check state of the devices

This example includes a simple utility to send commands to devices and receive replies from them. The code in `runDDSCommandUI.cxx` (compiled as ex3-dds-command-ui) uses the DDS intercom library to send "check-state" string to all devices, to which they reply with their ID and state they are in. The utility also allows requesting state changes from devices. This can be used as an example of sending/receiving commands or other information to devices.

To see it in action, start the ex3-dds-command-ui while the topology is running.

##### 11. Stop DDS server/topology.

The execution of tasks can be stopped with:
```bash
dds-topology --stop
```
Or by stopping the DDS server:
```bash
dds-server stop
```

For a more complete DDS documentation please refer to [DDS Website](http://dds.gsi.de/).
