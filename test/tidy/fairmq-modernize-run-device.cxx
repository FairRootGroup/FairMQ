// CHECK-FIXES: #include <fairmq/runDevice.h>
#include <runFairMQDevice.h>

#include <boost/program_options.hpp>

struct Sampler : fair::mq::Device
{
    explicit Sampler(int rate) : fRate(rate) {}
    int fRate;
};

struct Sink : fair::mq::Device
{};

void addCustomOptions(boost::program_options::options_description& options)
{
    options.add_options()("rate", boost::program_options::value<int>()->default_value(1), "rate");
}

// CHECK-MESSAGES: :[[@LINE+1]]:17: warning: 'runFairMQDevice.h' and a raw 'fair::mq::Device*' from 'getDevice()' are superseded; use 'fairmq/runDevice.h' and return a 'std::unique_ptr<fair::mq::Device>' [fairmq-modernize-run-device]
FairMQDevicePtr getDevice(const fair::mq::ProgOptions& config)
// CHECK-FIXES: std::unique_ptr<fair::mq::Device> getDevice(fair::mq::ProgOptions& config)
{
    if (config.Count("sink")) {
        return new Sink;
        // CHECK-FIXES: return std::make_unique<Sink>();
    }
    if (config.Count("none")) {
        return nullptr;
    }
    return new Sampler(config.GetProperty<int>("rate"));
    // CHECK-FIXES: return std::make_unique<Sampler>(config.GetProperty<int>("rate"));
}
