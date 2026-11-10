#include <fairmq/Channel.h>
#include <fairmq/Device.h>
#include <fairmq/Message.h>
#include <fairmq/Parts.h>

#include <memory>
#include <vector>

// CHECK-MESSAGES: :[[@LINE+1]]:18: warning: 'FairMQDevice' is deprecated; use 'fair::mq::Device' [fairmq-modernize-nonnamespaced-types]
struct Sampler : FairMQDevice
// CHECK-FIXES: struct Sampler : fair::mq::Device
{
    bool ConditionalRun() override
    {
        // CHECK-MESSAGES: :[[@LINE+1]]:9: warning: 'FairMQMessagePtr' is deprecated; use 'fair::mq::MessagePtr' [fairmq-modernize-nonnamespaced-types]
        FairMQMessagePtr msg(NewMessage(10));
        // CHECK-FIXES: fair::mq::MessagePtr msg(NewMessage(10));
        // CHECK-MESSAGES: :[[@LINE+1]]:11: warning: 'FairMQParts' is deprecated; use 'fair::mq::Parts' [fairmq-modernize-nonnamespaced-types]
        ::FairMQParts parts;
        // CHECK-FIXES: ::fair::mq::Parts parts;
        parts.AddPart(std::move(msg));
        return false;
    }
};

// CHECK-MESSAGES: :[[@LINE+1]]:23: warning: 'FairMQChannel' is deprecated; use 'fair::mq::Channel' [fairmq-modernize-nonnamespaced-types]
void useChannel(const FairMQChannel&);
// CHECK-FIXES: void useChannel(const fair::mq::Channel&);

// CHECK-MESSAGES: :[[@LINE+1]]:29: warning: 'FairMQMessage' is deprecated; use 'fair::mq::Message' [fairmq-modernize-nonnamespaced-types]
std::vector<std::unique_ptr<FairMQMessage>> messages;
// CHECK-FIXES: std::vector<std::unique_ptr<fair::mq::Message>> messages;

// Deprecated global aliases without the FairMQ prefix.
// CHECK-MESSAGES: :[[@LINE+1]]:1: warning: 'InputMsgCallback' is deprecated; use 'fair::mq::InputMsgCallback' [fairmq-modernize-nonnamespaced-types]
InputMsgCallback callback;
// CHECK-FIXES: fair::mq::InputMsgCallback callback;

// Namespaced types and aliases of the user are not reported.
using MyDevice = fair::mq::Device;
fair::mq::MessagePtr namespacedMsg;
MyDevice* device = nullptr;

// Spellings in macro expansions are not reported.
#define DEVICE_PTR FairMQDevice*
DEVICE_PTR macroDevice = nullptr;
