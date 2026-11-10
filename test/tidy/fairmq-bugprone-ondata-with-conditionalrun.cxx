#include <fairmq/Device.h>

struct Both : fair::mq::Device
{
    Both() { OnData("data", &Both::HandleData); }
    // CHECK-MESSAGES: :[[@LINE+1]]:10: warning: 'Both' overrides 'ConditionalRun()' and registers a data callback with 'OnData()'; 'ConditionalRun()' never runs [fairmq-bugprone-ondata-with-conditionalrun]
    bool ConditionalRun() override { return true; }
    bool HandleData(fair::mq::MessagePtr&, int) { return true; }
};

struct Lambda : fair::mq::Device
{
    void InitTask() override
    {
        OnData("data", [](fair::mq::MessagePtr&, int) { return true; });
    }
    bool ConditionalRun() override;
};
// CHECK-MESSAGES: :[[@LINE+1]]:14: warning: 'Lambda' overrides 'ConditionalRun()' and registers a data callback with 'OnData()'; 'ConditionalRun()' never runs [fairmq-bugprone-ondata-with-conditionalrun]
bool Lambda::ConditionalRun() { return true; }

// Not reported: only one of the two.
struct OnlyCallback : fair::mq::Device
{
    OnlyCallback() { OnData("data", &OnlyCallback::HandleData); }
    bool HandleData(fair::mq::MessagePtr&, int) { return true; }
};

struct OnlyConditionalRun : fair::mq::Device
{
    bool ConditionalRun() override { return true; }
};
