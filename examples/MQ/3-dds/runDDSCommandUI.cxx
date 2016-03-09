// DDS
#include "CustomCmd.h"
// STD
#include <iostream>
#include <exception>
#include <sstream>
#include <thread>
#include <atomic>

using namespace std;
using namespace dds;
using namespace custom_cmd;

int main(int argc, char* argv[])
{
    try
    {
        CCustomCmd ddsCustomCmd;

        ddsCustomCmd.subscribeCmd([](const string& command, const string& condition, uint64_t senderId)
        {
            cout << "Received: \"" << command << "\"" << endl;
        });

        while (true)
        {
            int result = ddsCustomCmd.sendCmd("check-state", "");

            if (result == 1)
            {
                cerr << "Error sending custom command" << endl;
            }

            this_thread::sleep_for(chrono::seconds(1));
        }
    }
    catch (exception& _e)
    {
        cerr << "Error: " << _e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
