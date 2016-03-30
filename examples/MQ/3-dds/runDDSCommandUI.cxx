#include "dds_intercom.h" // DDS

// STD
#include <iostream>
#include <exception>
#include <sstream>
#include <thread>
#include <atomic>

using namespace std;
using namespace dds::intercom_api;

int main(int argc, char* argv[])
{
    try
    {
        CCustomCmd ddsCustomCmd;

        ddsCustomCmd.subscribe([](const string& command, const string& condition, uint64_t senderId)
        {
            cout << "Received: \"" << command << "\"" << endl;
        });

        while (true)
        {
            int result = ddsCustomCmd.send("check-state", "");

            if (result == 1)
            {
                cerr << "Error sending custom command" << endl;
            }

            this_thread::sleep_for(chrono::seconds(1));
        }
    }
    catch (exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}