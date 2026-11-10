#include <fairmq/Device.h>

struct Sampler : fair::mq::Device
{
    bool ConditionalRun() override
    {
        // CHECK-MESSAGES: :[[@LINE+1]]:20: warning: 'GetChannel()' looks up the channel by name in each iteration of 'ConditionalRun()'; look it up once, e.g. in 'InitTask()', and keep a reference [fairmq-performance-channel-lookup-in-loop]
        auto& ch = GetChannel("data");
        auto msg(ch.NewMessage());
        return ch.Send(msg) >= 0;
    }
};

struct Looper : fair::mq::Device
{
    void Run() override
    {
        auto& out = GetChannel("data");
        while (!NewStatePending()) {
            // CHECK-MESSAGES: :[[@LINE+1]]:24: warning: 'GetChannel()' looks up the channel by name in each iteration of a loop in 'Run()'; look it up once, e.g. in 'InitTask()', and keep a reference [fairmq-performance-channel-lookup-in-loop]
            auto& in = GetChannel("in", 0);
            auto msg(in.NewMessage());
            // CHECK-MESSAGES: :[[@LINE+1]]:17: warning: 'GetChannels()' looks up the channel by name in each iteration of a loop in 'Run()'; look it up once, e.g. in 'InitTask()', and keep a reference [fairmq-performance-channel-lookup-in-loop]
            if (GetChannels().count("monitor") == 0) {
                out.Send(msg);
            }
        }
        // Not reported: once per loop, or in a lambda.
        for (auto& sub : GetChannels().at("data")) {
            static_cast<void>(sub);
        }
        for (auto& ch = GetChannel("data"); !NewStatePending();) {
            auto msg(ch.NewMessage());
            auto send = [&] { return GetChannel("data").Send(msg); };
            send();
        }
    }
};
