DDS Example
===========

This example demonstrates usage of the Dynamic Deployment System ([DDS](http://dds.gsi.de/)) to dynamically deploy and configure a topology of devices. The topology is similar to the one in Example 1-n-1, but now it can be easily distributed on different computing nodes without the need for manual addresses reconfiguration of the devices.

The description below outlines the minimal steps needed to run the example with DDS. For general DDS help please refer to DDS documentation on [DDS Website](http://dds.gsi.de/).

##### 1. The device handles the channel addresses and ports configuration via DDS Plugin.

It is sufficient to provide the `-S "<@FAIRMQ_INSTALL_DIR@/lib" -P dds` (`<` prepends the following path to the default plugin search paths; put in the path which points to the library dir of your FairRoot installation) command line arguments to let the devices be configured dynamically. No code changes in the device are necessary. See the XML topology file for example of using the command line arguments.

##### 2a. Write DDS hosts file that contains a list of worker nodes to run the topology on (When deploying using the SSH plug-in).

We run this example on the local machine for simplicity. The file below defines 3 workers - sampler, processor and sink - with a total of 12 DDS agents (thus able to accept 12 tasks). The parameters for each worker node are:
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

##### 2b. Skip (When deploying using the localhost plug-in).

If you want to deploy on a single host DDS 1.6+ provides a localhost rms plug-in. You do not need a hosts file in that case.

##### 3. Write DDS topology file that describes which tasks (processes) to run and their topology and configuration.

Take a look at `ex-dds-topology.xml`. It consists of a definition part (properties, tasks, collections and more) and execution part (main). In our example Sampler, Processor and Sink tasks are defined, containing their executables and exchanged properties. The `<main>` of the topology uses the defined tasks. Besides one Sampler and one Sink task, a group containing Processor task is defined. The group has a multiplicity of 10, meaninig 10 Processors will be executed. Each of the Processors will receive the properties with Sampler and Sink addresses.

The configuration of the channel connection addresses is done by the DDS plugin via the channel names. The task property names must correspond to the channel names (data1, data2), with binding channels writing the properties and connecting channel reading the properties.

**If you chose step 2b earlier**, then modify the provided `ex-dds-topology.xml` in the top that the following lines read as following:
```xml
<declrequirement name="SamplerWorker" type="wnname" value=".*"/>
<declrequirement name="ProcessorWorker" type="wnname" value=".*"/>
<declrequirement name="SinkWorker" type="wnname" value=".*"/>
```

Note that the attributes `value` contain a different value.

##### 4. Start DDS session.

First you need to initialize DDS environment:

```bash
source DDS_env.sh  # this script is located in the DDS installation directory
```

The DDS session is started with:

```bash
dds-session start
```

##### 5. Submit DDS Agents (configured in the hosts file).

Agents are submitted with:
```bash
dds-submit --rms ssh --config ex-dds-hosts.cfg
```
The `--rms` option defines a destination resource management system. The `--config` specifies an SSH plug-in resource definition file.

**If you chose step 2b earlier**, run the following command instead:

```bash
dds-submit --rms localhost -n 12
```

##### 6. Activate the topology.

```bash
dds-topology --activate ex-dds-topology.xml
```

##### 7. Run

After activation, agents will execute the defined tasks on the worker nodes. Output of the tasks will be stored in the directory that was specified in the hosts file (or in the system temporary directory when using the localhost plugin).

##### 8. (optional) Use example command UI to check state of the devices

A simple utility (fairmq-dds-command-ui) is included with FairMQ to send commands to devices and receive replies from them. The utility uses the DDS intercom library to query state/config of devices and allows changing their state. To let the device listen to the commands from the utility, start the device with `-S "<@FAIRMQ_INSTALL_DIR@/lib" -P dds` cmd option (see example XML topology).

To see it in action, start the fairmq-dds-command-ui while the topology is running. Run the utility with `-h` to see everything that it can do.

The utility requires a session parameter to connect to appropriate DDS session. The session value is given when starting dds-session.

By default the command UI sends commands to all tasks. This can be further refined by giving a specific topology path via `-p` argument.
Given our topology file, here are some examples of valid paths:
```bash
# get state of all devices
./fairmq/plugins/DDS/fairmq-dds-command-ui -s 937ffbca-b524-44d8-9898-1d69aedc3751 -c c
# get state of sampler
./fairmq/plugins/DDS/fairmq-dds-command-ui -s 937ffbca-b524-44d8-9898-1d69aedc3751 -c c -p main/Sampler
# get state of sink
./fairmq/plugins/DDS/fairmq-dds-command-ui -s 937ffbca-b524-44d8-9898-1d69aedc3751 -c c -p main/Sink
# get state all processors
./fairmq/plugins/DDS/fairmq-dds-command-ui -s 937ffbca-b524-44d8-9898-1d69aedc3751 -c c -p main/ProcessorGroup/Processor
# get state of a specific processor
./fairmq/plugins/DDS/fairmq-dds-command-ui -s 937ffbca-b524-44d8-9898-1d69aedc3751 -c c -p main/ProcessorGroup/Processor_9
```

##### 9. Stop DDS session/topology.

The execution of tasks can be stopped with:
```bash
dds-topology --stop
```
Or by stopping the DDS session:
```bash
dds-session stop
```

For general DDS documentation please refer to [DDS Website](http://dds.gsi.de/).
