// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'FairMQChannel.h' is deprecated; include <fairmq/Channel.h> [fairmq-modernize-deprecated-headers]
#include <FairMQChannel.h>
// CHECK-FIXES: #include <fairmq/Channel.h>
// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'fairmq/FairMQDevice.h' is deprecated; include <fairmq/Device.h> [fairmq-modernize-deprecated-headers]
#include <fairmq/FairMQDevice.h>
// CHECK-FIXES: #include <fairmq/Device.h>
// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'FairMQLogger.h' is deprecated; include <fairlogger/Logger.h> [fairmq-modernize-deprecated-headers]
#include <FairMQLogger.h>
// CHECK-FIXES: #include <fairlogger/Logger.h>
// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'FairMQMessage.h' is deprecated; include <fairmq/Message.h> [fairmq-modernize-deprecated-headers]
#include <FairMQMessage.h>
// CHECK-FIXES: #include <fairmq/Message.h>
// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'FairMQParts.h' is deprecated; include "fairmq/Parts.h" [fairmq-modernize-deprecated-headers]
#include "FairMQParts.h"
// CHECK-FIXES: #include "fairmq/Parts.h"
// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'FairMQPoller.h' is deprecated; include <fairmq/Poller.h> [fairmq-modernize-deprecated-headers]
#include <FairMQPoller.h>
// CHECK-FIXES: #include <fairmq/Poller.h>
// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'options/FairMQProgOptions.h' is deprecated; include <fairmq/ProgOptions.h> [fairmq-modernize-deprecated-headers]
#include <options/FairMQProgOptions.h>
// CHECK-FIXES: #include <fairmq/ProgOptions.h>
// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'FairMQSocket.h' is deprecated; include <fairmq/Socket.h> [fairmq-modernize-deprecated-headers]
#include <FairMQSocket.h>
// CHECK-FIXES: #include <fairmq/Socket.h>
// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'FairMQTransportFactory.h' is deprecated; include <fairmq/TransportFactory.h> [fairmq-modernize-deprecated-headers]
#include <FairMQTransportFactory.h>
// CHECK-FIXES: #include <fairmq/TransportFactory.h>
// CHECK-MESSAGES: :[[@LINE+1]]:10: warning: header 'FairMQUnmanagedRegion.h' is deprecated; include <fairmq/UnmanagedRegion.h> [fairmq-modernize-deprecated-headers]
#include <FairMQUnmanagedRegion.h>
// CHECK-FIXES: #include <fairmq/UnmanagedRegion.h>

// Current headers are not reported.
#include <fairmq/Device.h>
#include <fairmq/States.h>
