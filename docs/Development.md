← [Back](../README.md)

# 4. Development

## 4.1 Testing

For unit testing it is often not feasible to boot up a full-blown distributed system with dozens of processes.

In some scenarios it is useful to not even instantiate a `fair::mq::Device` at all. Please see [this example](../test/protocols/_push_pull_multipart.cxx) for single and multi threaded unit test without a device instance. If you store your transport factories and channels on the heap, pls make sure, you destroy the channels before you destroy the related transport factory for proper shutdown. Channels provide all the `Send/Receive` and `New*Message/New*Poller` APIs provided by the device too.

## 4.2 Static Analysis

With `-DBUILD_TIDY_TOOL=ON` you can build the `fairmq-tidy` tool that implements static checks on your source code. To use it, enable the generation of a [compilation database](https://clang.llvm.org/docs/JSONCompilationDatabase.html) in your project via `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` (generates a file `<builddir>/compile_commands.json`):

```
fairmq-tidy -p <builddir> mysourcefile.cpp
```

If you find any issue (e.g. false positives) with this tool, please tell us by opening an issue on github.

### 4.2.1 CMake Integration

When discovering a FairMQ installation in your project, explicitely state, that you want one with the `fairmq-tidy` tool included:

```
find_package(FairMQ COMPONENTS tidy_tool)
```

Now the CMake module [`FairMQTidy.cmake`](../cmake/FairMQTidy.cmake) is available for inclusion:

```
include(FairMQTidy)
```

It provides the CMake function `fairmq_target_tidy()` which attaches appropriate `fairmq-tidy` build rules to each source file contained in the passed library or executable target, e.g. if you have an executable that uses FairMQ:

```
add_executable(myexe mysource1.cpp mysource2.cpp)
target_link_libraries(myexe PRIVATE FairMQ::FairMQ)
fairmq_target_tidy(TARGET myexe)
```

### 4.2.2 Extra Compiler Arguments

On most Linux distros you are likely to use GCC to compile your projects, so the resulting `compile_commands.json` contains the command-line tuned for GCC which might be missing options needed to successfully invoke the Clang frontend which is used internally by `fairmq-tidy`. In general, you can pass extra `clang` options via the following options:

```
  --extra-arg=<string>        - Additional argument to append to the compiler command line
  --extra-arg-before=<string> - Additional argument to prepend to the compiler command line
```

E.g. if standard headers are not found, you can hint the location like this:

```
fairmq-top -p <builddir> --extra-arg-before=-I$(clang -print-resource-dir)/include mysourcefile.cpp
```

← [Back](../README.md)
