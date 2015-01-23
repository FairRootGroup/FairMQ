# FairMQ Tools

Contains common tools for use by FairMQ and/or users.

## FairMQ::tools::getHostIPs

Fills a map with the network interfaces and their IP addresses available on the current host.

#### Example usage

```c++
#include <map>
#include <string>
#include <iostream>

#include "FairMQTools.h"

void main()
{
    std::map<string,string> IPs;

    FairMQ::tools::getHostIPs(IPs);

    for (std::map<string,string>::iterator it = IPs.begin(); it != IPs.end(); ++it)
    {
        std::cout << it->first << ": " << it->second << std::endl;
    }
}
```
##### Example Output
```
eth0: 123.123.1.123
ib0: 123.123.2.123
lo: 127.0.0.1
```