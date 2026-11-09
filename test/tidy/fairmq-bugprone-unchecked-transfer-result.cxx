// TIDY-CONFIG: {CheckOptions: {fairmq-bugprone-unchecked-transfer-result.CheckSend: true}}
#include <fairmq/Device.h>

struct Processor : fair::mq::Device
{
    bool ConditionalRun() override
    {
        auto msg(NewMessage());
        // CHECK-MESSAGES: :[[@LINE+1]]:9: warning: result of 'Receive()' is not checked; it is negative on a timeout, an error or an interrupt [fairmq-bugprone-unchecked-transfer-result]
        Receive(msg, "in");
        // CHECK-MESSAGES: :[[@LINE+1]]:29: warning: result of 'Receive()' is not checked; it is negative on a timeout, an error or an interrupt [fairmq-bugprone-unchecked-transfer-result]
        GetChannel("in", 0).Receive(msg);
        for (int i = 0; i < 3; ++i)
            // CHECK-MESSAGES: :[[@LINE+1]]:13: warning: result of 'Send()' is not checked; it is negative on a timeout, an error or an interrupt [fairmq-bugprone-unchecked-transfer-result]
            Send(msg, "out");

        // Not reported: the result is used, or discarded on purpose.
        if (Receive(msg, "in") < 0) {
            return false;
        }
        auto const bytes = Send(msg, "out");
        static_cast<void>(Send(msg, "out"));
        (void)Receive(msg, "in");
        return bytes >= 0;
    }
};
