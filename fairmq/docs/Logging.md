← [Back](../README.md)

# 5. Logging

The FairMQLogger header uses fair::Logger library for logging.

All log calls go through the provided LOG(severity) macro. Output through this macro is thread-safe. Logging is done to cout, file output and/or custom sinks.

## 5.1 Log severity

The log severity is controlled via:
```C++
fair::Logger::SetConsoleSeverity("<severity level>");
// and/or
fair::Logger::SetFileSeverity("<severity level>");
// and/or
fair::Logger::SetCustomSeverity("<customSinkName>", "<severity level>");
```

where severity level is one of the following:

```C++
"nolog",
"fatal",
"error",
"warn",
"state",
"info",
"debug",
"debug1",
"debug2",
"debug3",
"debug4",
"trace",
```

Logger will log the chosen severity and all above it (except "nolog", which deactivates logging for that sink completely). Fatal severity is always logged.

When running a FairMQ device, the log severity can be simply provided via `--severity <level>` cmd option.

## 5.2 Log verbosity

The log severity is controlled via:
```C++
fair::Logget::SetVerbosity("<verbosity level>");
```

it is same for all sinks, and is one of the follwing values: `low`, `medium`, `high`, `veryhigh`, which translates to following output:

```
low:      [severity] message
medium:   [HH:MM:SS][severity] message
high:     [process name][HH:MM:SS:µS][severity] message
veryhigh: [process name][HH:MM:SS:µS][severity][file:line:function] message
```

When running a FairMQ device, the log severity can be simply provided via `--verbosity <level>` cmd option.

## 5.3 Color

Colored output on console can be activated with:
```C++
Logger::SetConsoleColor(true);
```

## 5.4 File output

Output to file can be enabled via:
```C++
Logger::InitFileSink("<severity level>", "test_log", true);
```
which will add output to "test_log" filename (if third parameter is `true` it will add timestamp to the file name) with `<severity level>` severity.

## 5.5 Custom sinks

Custom sinks can be added via `Logger::AddCustomSink("sink name", "<severity>", callback)` method.

Here is an example adding a custom sink for all severities ("trace" and above). It has access to the log content and meta data. Custom log calls are also thread-safe.

```C++
    Logger::AddCustomSink("MyCustomSink", "trace", [](const string& content, const LogMetaData& metadata)
    {
        cout << "content: " << content << endl;

        cout << "available metadata: " << endl;
        cout << "std::time_t timestamp: " << metadata.timestamp << endl;
        cout << "std::chrono::microseconds us: " << metadata.us.count() << endl;
        cout << "std::string process_name: " << metadata.process_name << endl;
        cout << "std::string file: " << metadata.file << endl;
        cout << "std::string line: " << metadata.line << endl;
        cout << "std::string func: " << metadata.func << endl;
        cout << "std::string severity_name: " << metadata.severity_name << endl;
        cout << "fair::Severity severity: " << static_cast<int>(metadata.severity) << endl;
    });
```

If only output from custom sinks is desirable, console/file sinks must be deactivated by setting their severity to `"nolog"`.

← [Back](../README.md)
