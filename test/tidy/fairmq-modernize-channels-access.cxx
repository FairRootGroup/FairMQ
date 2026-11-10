#include <fairmq/Device.h>

#include <string>

struct Sampler : fair::mq::Device
{
    void InitTask() override
    {
        // CHECK-MESSAGES: :[[@LINE+1]]:20: warning: 'fChannels' is deprecated; use 'GetChannel("data", 0)' [fairmq-modernize-channels-access]
        auto& ch = fChannels.at("data").at(0);
        // CHECK-FIXES: auto& ch = GetChannel("data", 0);
        // CHECK-MESSAGES: :[[@LINE+1]]:29: warning: 'fChannels' is deprecated; use 'this->GetChannel(fName, fIndex)' [fairmq-modernize-channels-access]
        auto& other = this->fChannels.at(fName).at(fIndex);
        // CHECK-FIXES: auto& other = this->GetChannel(fName, fIndex);
        // CHECK-MESSAGES: :[[@LINE+1]]:30: warning: 'fChannels' is deprecated; use 'GetChannels()' [fairmq-modernize-channels-access]
        auto const n = this->fChannels.count("data");
        // CHECK-FIXES: auto const n = this->GetChannels().count("data");
        // CHECK-MESSAGES: :[[@LINE+1]]:31: warning: 'fChannels' is deprecated; use 'GetChannels()' [fairmq-modernize-channels-access]
        for (auto const& kv : fChannels) {
        // CHECK-FIXES: for (auto const& kv : GetChannels()) {
            static_cast<void>(kv);
        }
        // CHECK-MESSAGES: :[[@LINE+1]]:21: warning: 'fChannels' is deprecated; use 'GetChannels()' [fairmq-modernize-channels-access]
        auto& vec = fChannels.at("data");
        // CHECK-FIXES: auto& vec = GetChannels().at("data");
        static_cast<void>(ch);
        static_cast<void>(other);
        static_cast<void>(n);
        static_cast<void>(vec);
    }

    std::string fName{"data"};
    int fIndex{0};
};

void useDevice(fair::mq::Device& device)
{
    // CHECK-MESSAGES: :[[@LINE+1]]:23: warning: 'fChannels' is deprecated; use 'device.GetChannel("data", 1)' [fairmq-modernize-channels-access]
    auto& ch = device.fChannels.at("data").at(1);
    // CHECK-FIXES: auto& ch = device.GetChannel("data", 1);
    static_cast<void>(ch);
}

// The current API is not reported.
void useCurrentApi(fair::mq::Device& device)
{
    auto& ch = device.GetChannel("data", 0);
    auto const n = device.GetChannels().size();
    static_cast<void>(ch);
    static_cast<void>(n);
}
