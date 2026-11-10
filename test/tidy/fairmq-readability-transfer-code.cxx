#include <fairmq/Device.h>

#include <cstdint>

struct Sink : fair::mq::Device
{
    bool ConditionalRun() override
    {
        auto msg(NewMessage());
        auto const result = Receive(msg, "data");
        // CHECK-MESSAGES: :[[@LINE+1]]:45: warning: magic number -2 for 'fair::mq::TransferCode::timeout' [fairmq-readability-transfer-code]
        if (Receive(msg, "data", 0, 100) == -2) {
        // CHECK-FIXES: if (Receive(msg, "data", 0, 100) == static_cast<int64_t>(fair::mq::TransferCode::timeout)) {
            return true;
        }
        // CHECK-MESSAGES: :[[@LINE+1]]:13: warning: magic number -3 for 'fair::mq::TransferCode::interrupted' [fairmq-readability-transfer-code]
        if (-3 != GetChannel("data").Send(msg)) {
        // CHECK-FIXES: if (static_cast<int64_t>(fair::mq::TransferCode::interrupted) != GetChannel("data").Send(msg)) {
            return true;
        }
        // CHECK-MESSAGES: :[[@LINE+1]]:38: warning: magic number -1 for 'fair::mq::TransferCode::error' [fairmq-readability-transfer-code]
        return Send(msg, "data") != (-1);
        // CHECK-FIXES: return Send(msg, "data") != (static_cast<int64_t>(fair::mq::TransferCode::error));

        // Not reported: other numbers, other operators, other functions.
        static_cast<void>(result < 0);
        static_cast<void>(Send(msg, "data") == -4);
        static_cast<void>(Send(msg, "data") < -1);
        static_cast<void>(msg->GetSize() == 0);
    }
};
