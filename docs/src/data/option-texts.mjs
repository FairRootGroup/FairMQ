// Descriptions of the command-line options in ASD-STE100 style.
// Names, types and default values come from the code (scripts/options.mjs).
// The build fails if an entry here names an option that the code does not have.

export const deviceOptionTexts = {
  help: 'Print the help and stop.',
  version:
    'Print the versions of FairMQ and of the device, and stop. Known defect: the output is empty, because the device sets the console severity to <code>nolog</code> first.',
  severity:
    'Console log severity: trace, debug, info, state, warn, error, fatal or nolog. Empty: keep the default of FairLogger. The environment variable <code>FAIRMQ_SEVERITY</code> overrides this option.',
  'file-severity': 'Log severity for the log file of <code>--log-to-file</code>.',
  verbosity: 'Log verbosity: veryhigh, high, medium or low. It sets the fields in each log line.',
  color: 'Use colors in the console log.',
  'log-to-file': 'Write the log to this file. The console log is then off.',
  'print-options': 'Print all options in a machine-readable format (<code>option:value:type:description</code>) and stop.',
  id: 'Device ID. A JSON configuration uses it as the key of the device, unless you set <code>--config-key</code>.',
  'io-threads': 'Number of I/O threads of the ZeroMQ context. The zeromq and the shmem transports use it.',
  transport: 'Default transport of the device: <code>zeromq</code> or <code>shmem</code>. A channel can set its own transport.',
  'network-interface':
    'Network interface for a bind sub-channel without an address, for example <code>eth0</code>. The device binds to the IP of this interface on an automatic port. <code>default</code>: the interface of the default route.',
  'init-timeout':
    'Time in seconds to connect all channels in the state CONNECTING. If an address is not valid yet, the device tries again every 50 ms and reads the address property again. After this time, the device goes to the state ERROR.',
  'print-channels':
    'Print the channels that the device registers with <code>RegisterChannelEndpoint()</code>, as <code>name:min:max</code> (the minimum and maximum number of sub-channels), and stop. Known defect: the output is empty, because the device sets the console severity to <code>nolog</code> first.',
  'shm-segment-size': 'Size of the managed shared memory segment in bytes. The default is 2 GiB. Only the device that creates the segment sets its size.',
  'shm-allocation': 'Allocation algorithm of the segment: <code>rbtree_best_fit</code> or <code>simple_seq_fit</code>. A device that opens an existing segment uses the algorithm of that segment and logs a warning if it differs.',
  'shm-segment-id': 'Experimental. ID of the segment for new messages. Devices of one session can use several segments.',
  shmid: 'Experimental. Use this shmId instead of the one that FairMQ makes from the session name and the user ID. The value is a decimal number. The object names contain it as a hexadecimal number with at least 8 digits: <code>--shmid 1</code> gives <code>00000001</code>.',
  'shm-mlock-segment': 'Lock the segment in RAM after the device opens or creates it.',
  'shm-mlock-segment-on-creation': 'Lock the segment in RAM only when this device creates it.',
  'shm-zero-segment': 'Write zeros to the free memory of the segment after the device opens or creates it.',
  'shm-zero-segment-on-creation': 'Write zeros to the free memory of the segment only when this device creates it.',
  'shm-throw-bad-alloc':
    'Throw <code>fair::mq::MessageBadAlloc</code> when an allocation in the segment fails. <code>false</code>: try again every <code>--bad-alloc-attempt-interval</code> ms, until the allocation succeeds or a transition interrupts the transport. <code>false</code> overrides <code>--bad-alloc-max-attempts</code>. A message that is larger than the segment always throws.',
  'shm-metadata-msg-size':
    'Size in bytes of the ZeroMQ message that carries the metadata. The transport pads the metadata to this size. A value below the size of the metadata (40 bytes for one message) has no effect.',
  'bad-alloc-max-attempts':
    'Number of allocation attempts before <code>MessageBadAlloc</code>. -1: no limit. The device always makes one attempt, so 0 and 1 have the same effect.',
  'bad-alloc-attempt-interval': 'Time between two allocation attempts in ms.',
  'shm-monitor': 'Start <code>fairmq-shmmonitor</code> for the session, if no monitor runs for it. The device looks for the executable in <code>FAIRMQ_PATH</code> and <code>PATH</code>.',
  'shm-no-cleanup': 'Keep the shared memory when the last device of the session stops.',
  rate: 'Maximum rate of the <code>ConditionalRun()</code> loop in Hz. 0: no limit.',
  session: 'Session name. Devices of the same user with the same session use the same shared memory.',
  'config-key': 'Key of the device in the JSON file. Use it instead of the device ID.',
  'mq-config': 'JSON file with the configuration of the devices and their channels. If you also give <code>--channel-config</code>, the device uses only <code>--mq-config</code>.',
  'channel-config':
    'Channel configuration as a list of <code>key=value</code> pairs, separated by commas. Give one list for each channel. Each <code>address</code> in a list adds a sub-channel. Several lists can follow one <code>--channel-config</code>. A list can start with <code>&lt;name&gt;:</code> instead of <code>name=&lt;name&gt;</code>.',
  control:
    'Controller of the state machine: <code>static</code>, <code>dynamic</code> (also <code>interactive</code> and <code>external</code>), <code>gui</code> or <code>none</code>. An unknown value gives <code>static</code>, with an error in the log.',
  'catch-signals': 'Handle SIGINT and SIGTERM: 1 (yes) or 0 (no). The first signal starts the shutdown sequence. A second SIGINT aborts the process.',
  plugin:
    'Load these plugins, in this sequence. For the file <code>libfairmq-plugin-example.so</code>, the name is <code>example</code>. Put <code>p:</code> before the name of a prelinked plugin.',
  'plugin-search-path':
    'Directories to search for plugins, in this order. Put <code>&lt;</code> before a path to put it at the start of the list, or <code>&gt;</code> to put it at the end. If a plugin is not in these directories, the device tries the system lookup of libraries (system directories, RUNPATH, <code>LD_LIBRARY_PATH</code>).',
};
