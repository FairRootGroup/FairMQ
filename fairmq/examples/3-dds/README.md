Example 3: DDS
===============

This example demonstrates usage of the Dynamic Deployment System ([DDS](http://dds.gsi.de/)) to dynamically deploy and configure a topology of devices. The topology is similar to those of Example 2, but now it can be easily distributed on different computing nodes without the need for manual reconfiguration of the devices.

The description below outlines the minimal steps needed to run the example with DDS. For more details please refer to DDS documentation on [DDS Website](http://dds.gsi.de/).

##### 1. The devices that bind their sockets need to advertise their bound addresses to DDS by writing a property.

In our example Sampler and Sink bind their sockets. The bound addresses are available after the initial validation. The following code takes the address value and gives it to DDS:

```C++
sampler.ChangeState("INIT_DEVICE");
sampler.WaitForInitialValidation();

dds::key_value::CKeyValue ddsKeyValue;
ddsKeyValue.putValue("SamplerOutputAddress", sampler.fChannels.at("data-out").at(0).GetAddress());

sampler.WaitForEndOfState("INIT_DEVICE");
```

Same approach for the Sink.

##### 2. The devices that connect their sockets need to read the addresses from DDS.

The Processors in our example need the addresses of Sampler and Sink. They receive these from DDS via properties (sent in the step above):

```C++
dds::key_value::CKeyValue ddsKeyValue;
// Sampler properties
dds::key_value::CKeyValue::valuesMap_t samplerValues;
{
    mutex keyMutex;
    condition_variable keyCondition;

    LOG(INFO) << "Subscribing and waiting for sampler output address.";
    ddsKeyValue.subscribe([&keyCondition](const string& /*_key*/, const string& /*_value*/) { keyCondition.notify_all(); });
    ddsKeyValue.getValues("SamplerOutputAddress", &samplerValues);
    while (samplerValues.empty())
    {
        unique_lock<mutex> lock(keyMutex);
        keyCondition.wait_until(lock, chrono::system_clock::now() + chrono::milliseconds(1000));
        ddsKeyValue.getValues("SamplerOutputAddress", &samplerValues);
    }
}
// Sink properties
// ... same as above, but for sinkValues ...

processor.fChannels.at("data-in").at(0).UpdateAddress(samplerValues.begin()->second);
processor.fChannels.at("data-out").at(0).UpdateAddress(sinkValues.begin()->second);
```

After this step each device will have the necessary connection information.

##### 3. Write DDS hosts file that contains a list of worker nodes to run the topology on (When deploying using the SSH plug-in).

We run this example on the local machine for simplicity. The file below defines one worker `wn0` with 12 DDS Agents (thus able to accept 12 tasks). The parameters for each worker node are:
 - user-chosen worker ID (must be unique)
 - a host name with or without a login, in a form: login@host.fqdn
 - additional SSH params (can be empty)
 - a remote working directory
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

##### 5. Start DDS server.

The DDS server is started with:

```bash
dds-server start -s
```

##### 6. Submit DDS Agents (configured in the hosts file).

Agents are submitted with:
```bash
dds-submit --rms ssh --ssh-rms-cfg ex3-dds-hosts.cfg
```
The `--rms` option defines a destination resource management system. The `--ssh-rms-cfg` specifies an SSH plug-in resource definition file. 

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

##### 10. Stop DDS server/topology.

The execution of tasks can be stopped with:
```bash
dds-topology --stop
```
Or by stopping the DDS server:
```bash
dds-server stop
```

For a more complete DDS documentation please refer to [DDS Website](http://dds.gsi.de/).
