#include <fairmq/Device.h>

#include <thread>

struct Endless : fair::mq::Device
{
    void Run() override
    {
        int n = 0;
        // CHECK-MESSAGES: :[[@LINE+1]]:9: warning: endless loop in 'Run()' does not check for a state change; the device does not stop on request; use 'while (!NewStatePending())' [fairmq-bugprone-run-loop-ignores-state]
        while (true) {
            ++n;
        }
        // CHECK-MESSAGES: :[[@LINE+1]]:9: warning: endless loop in 'Run()' does not check for a state change; the device does not stop on request; use 'while (!NewStatePending())' [fairmq-bugprone-run-loop-ignores-state]
        for (;;) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

// Not reported: the loops look at the state, transfer data or end.
struct Fine : fair::mq::Device
{
    void Run() override
    {
        while (!NewStatePending()) {
        }
        while (true) {
            if (NewStatePending()) {
                break;
            }
        }
        for (;;) {
            if (!WaitFor(std::chrono::seconds(1))) {
                return;
            }
        }
        while (1) {
            auto msg(NewMessage());
            Send(msg, "data");
        }
        for (int i = 0; i < 10; ++i) {
        }
    }
};

// Not reported: not Run() of a device.
struct Other
{
    void Run()
    {
        while (true) {
        }
    }
};
