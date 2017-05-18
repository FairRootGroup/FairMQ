#ifndef FAIRMQTOOLS_H_
#define FAIRMQTOOLS_H_

#warning "This header file is deprecated. Include <fairmq/Tools.h> instead. Note, that the namespace FairMQ::tools has been changed to fair::mq::tools in the new header."

#include <fairmq/tools/CppSTL.h>
#include <fairmq/tools/Network.h>

namespace FairMQ
{
namespace tools
{

using fair::mq::tools::make_unique;

using fair::mq::tools::getHostIPs;
using fair::mq::tools::getInterfaceIP;
using fair::mq::tools::getDefaultRouteNetworkInterface;

} // namespace tools
} // namespace FairMQ

#endif // FAIRMQTOOLS_H_
