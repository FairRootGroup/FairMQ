#include <fairmq/Device.h>
#include <fairmq/States.h>
#include <fairmq/Transports.h> // zeromq/Message.h needs operator<<(Transport)
#include <fairmq/zeromq/Socket.h>

#include <string>

struct Sampler : fair::mq::Device
{
    void InitTask() override
    {
        // CHECK-MESSAGES: :[[@LINE+1]]:31: warning: 'fair::mq::Device::GetStateName' is deprecated; use 'fair::mq::GetStateName' [fairmq-modernize-deprecated-functions]
        std::string const s = GetStateName(fair::mq::State::Ready);
        // CHECK-FIXES: std::string const s = fair::mq::GetStateName(fair::mq::State::Ready);
        // CHECK-MESSAGES: :[[@LINE+1]]:31: warning: 'fair::mq::Device::GetTransitionName' is deprecated; use 'fair::mq::GetTransitionName' [fairmq-modernize-deprecated-functions]
        std::string const t = fair::mq::Device::GetTransitionName(fair::mq::Transition::Stop);
        // CHECK-FIXES: std::string const t = fair::mq::GetTransitionName(fair::mq::Transition::Stop);
        static_cast<void>(s);
        static_cast<void>(t);
    }
};

void useDevice(fair::mq::Device& device, fair::mq::Socket& socket)
{
    // CHECK-MESSAGES: :[[@LINE+1]]:23: warning: 'fair::mq::Device::GetStateName' is deprecated; use 'fair::mq::GetStateName' [fairmq-modernize-deprecated-functions]
    auto const name = device.GetStateName(device.GetCurrentState());
    // CHECK-FIXES: auto const name = fair::mq::GetStateName(device.GetCurrentState());
    // CHECK-MESSAGES: :[[@LINE+1]]:24: warning: 'fair::mq::zmq::Socket::GetConstant' is deprecated; use 'fair::mq::zmq::getConstant' [fairmq-modernize-deprecated-functions]
    int const linger = fair::mq::zmq::Socket::GetConstant("linger");
    // CHECK-FIXES: int const linger = fair::mq::zmq::getConstant("linger");
    // CHECK-MESSAGES: :[[@LINE+1]]:12: warning: 'Socket::Close()' is deprecated; the socket closes when it is destroyed [fairmq-modernize-deprecated-functions]
    socket.Close();
    static_cast<void>(name);
    static_cast<void>(linger);
}

// The successors are not reported.
std::string const current = fair::mq::GetStateName(fair::mq::State::Idle);
