#include <fairmq/Device.h>

#include <cstring>

struct Sampler : fair::mq::Device
{
    bool ConditionalRun() override
    {
        auto msg(NewMessage(16));
        Send(msg, "data");
        // CHECK-MESSAGES: :[[@LINE+1]]:21: warning: 'msg' is used after it was sent; a sent message is empty (zeromq) or shared with the receiver (shmem) [fairmq-bugprone-use-after-send]
        std::memset(msg->GetData(), 0, 16);
        return true;
    }

    void Run() override
    {
        fair::mq::MessagePtr msg(NewMessage(16));
        while (!NewStatePending()) {
            // CHECK-MESSAGES: :[[@LINE+1]]:18: warning: 'msg' is used after it was sent; a sent message is empty (zeromq) or shared with the receiver (shmem) [fairmq-bugprone-use-after-send]
            Send(msg, "data");
        }
    }

    void Branch(bool flag)
    {
        auto msg(NewMessage(16));
        GetChannel("data").Send(msg);
        if (flag) {
            // CHECK-MESSAGES: :[[@LINE+1]]:25: warning: 'msg' is used after it was sent; a sent message is empty (zeromq) or shared with the receiver (shmem) [fairmq-bugprone-use-after-send]
            auto size = msg->GetSize();
            static_cast<void>(size);
        }
    }

    // Not reported below.
    void NewEachIteration()
    {
        while (!NewStatePending()) {
            auto msg(NewMessage(16));
            Send(msg, "data");
        }
    }

    void Reinit()
    {
        auto msg(NewMessage(16));
        Send(msg, "data");
        msg = NewMessage(8);
        Send(msg, "data");
        msg->Rebuild(4);
        Send(msg, "data");
        Receive(msg, "data");
        static_cast<void>(msg->GetSize());
        Send(msg, "data");
        msg.reset();
    }

    void Checked()
    {
        auto msg(NewMessage(16));
        if (Send(msg, "data") < 0) {
            static_cast<void>(msg->GetSize());
        }
    }

    void Parts()
    {
        fair::mq::Parts parts;
        Send(parts, "data");
        static_cast<void>(parts.Size());
    }
};
