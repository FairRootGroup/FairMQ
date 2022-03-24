← [Back](../README.md)

# 7. Plugins

FairMQ devices can be integrated with external configuration and control systems through its plugin system. FairMQ plugins are special dynamic libraries that can be loaded at runtime. Plugins have access to the Plugin API which includes the capability to control/monitor the device [state machine](Device.md#13-state-machine) and change/monitor configuration properties.

A simple plugin may add the feature to read configuration from a certain desired file format once at the start of a device. A more complex plugin may create a long-running thread that integrates a network client to an external API of a central experiment control system.

Because plugins are loaded dynamically, they can be developed in separate repositories/projects and also have their own set of runtime dependency that are not needed to be known at compile-time of the FairMQ device.

## 7.1 Usage

To load a plugin pass the `-P <name>[,<name>]` (or long `--plugin`) command line option. Multiple plugins can be loaded at the same time. The load order is as specified at the command line. This determines the order in which the plugins are instantiated (ctor call order) and in which order they are notified, should they subscribe to any notifications.

When passing `-h/--help` on the command line one can find more detailed information:

```
Plugin Manager:
  -S [ --plugin-search-path ] arg       List of plugin search paths.
                                        * Override default search path, e.g.
                                          -S /home/user/lib /lib
                                        * Append(>) or prepend(<) to default
                                        search path, e.g.
                                          -S >/lib </home/user/lib
                                        * If you mix the overriding and
                                        appending/prepending syntaxes, the
                                        overriding paths act as default search
                                        path, e.g.
                                          -S /usr/lib >/lib </home/user/lib
                                        /usr/local/lib results in
                                        /home/user/lib,/usr/local/lib,/usr/lib/
                                        ,/lib
                                        If nothing is found, the default
                                        dynamic library lookup is performed,
                                        see man ld.so(8) for details.
  -P [ --plugin ] arg                   List of plugin names to load in
                                        order,e.g. if the file is called
                                        'libfairmq-plugin-example.so', just list
                                        'example' or 'd:example' here.To load a
                                        prelinked plugin, list 'p:example'
                                        here.
```

## 7.2 Development

To develop a custom FairMQ plugin, one simply needs to inherit from the `fair::mq::Plugin` base class (`#include <fairmq/Plugin.h>`) and call the `REGISTER_FAIRMQ_PLUGIN` macro. It is possible to introduce new command line option together with a plugin.

The Plugin API includes:
  * `Take/Steal/ReleaseDeviceControl()`/`GetCurrent/ChangeDeviceState()`/`SubscribeTo/UnsubscribeFromDeviceStateChange()` APIs enable controlling the device state machine. Only one plugin is authorized to control at the same time. Which one is determined by which plugin calls `TakeDeviceControl()` first.
  * `Set/GetProperty()`/`GetPropertyKeys()`/`SubscribeTo/UnsubscribeFromPropertyChange()` APIs enable configuration of device properties.
See [`<fairmq/Plugin.h>`](/fairmq/Plugin.h) for the full API.

A more complete example which may serve as a start including example CMake code can be found here: [FairRootGroup/FairMQPlugin_example](https://github.com/FairRootGroup/FairMQPlugin_example).

## 7.3 Provided Plugins

### 7.3.1 PMIx

The [PMIx](https://pmix.org/) plugin enables launching a FairMQ topology with any PMIx capable launcher, e.g. the [Open Run-Time Environment (ORTE) of OpenMPI](https://www.open-mpi.org/doc/v4.0/man1/mpirun.1.php) or the [Slurm workload manager](https://slurm.schedmd.com/srun.html). This plugin is not (yet) very mature and serves as a proof of concept at the moment.

TODO example usage

← [Back](../README.md)
