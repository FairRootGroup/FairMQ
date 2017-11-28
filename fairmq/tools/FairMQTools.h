#ifndef FAIRMQTOOLS_H_
#define FAIRMQTOOLS_H_

#warning "This header file is deprecated. Include <fairmq/Tools.h> instead. Note, that the namespace FairMQ::tools has been changed to fair::mq::tools in the new header."

#include <fairmq/tools/CppSTL.h>
#include <fairmq/tools/Network.h>
#include <fairmq/tools/Strings.h>
#include <fairmq/tools/Version.h>
#include <fairmq/tools/Unique.h>

namespace FairMQ
{
namespace tools
{

using fair::mq::tools::make_unique;
using fair::mq::tools::HashEnum;

using fair::mq::tools::getHostIPs;
using fair::mq::tools::getInterfaceIP;
using fair::mq::tools::getDefaultRouteNetworkInterface;

using fair::mq::tools::S;

using fair::mq::tools::Uuid;
using fair::mq::tools::UuidHash;

using fair::mq::tools::Version;

} // namespace tools
} // namespace FairMQ

#endif // FAIRMQTOOLS_H_
