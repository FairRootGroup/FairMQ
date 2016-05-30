#include "dds_intercom.h"

#include <termios.h> // raw mode console input

#include <iostream>
#include <exception>
#include <sstream>
#include <thread>
#include <atomic>
#include <unistd.h>

using namespace std;
using namespace dds::intercom_api;

void PrintControlsHelp()
{
    cout << "Use keys to control the devices:" << endl;
    cout << "[c] check states, [h] help, [p] pause, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device" << endl;
    cout << "To quit press Ctrl+C" << endl;
}

int main(int argc, char* argv[])
{
    try
    {
        CCustomCmd ddsCustomCmd;

        // subscribe to receive messages from DDS
        ddsCustomCmd.subscribe([](const string& msg, const string& condition, uint64_t senderId)
        {
            cout << "Received: \"" << msg << "\"" << endl;
        });

        char c;

        // setup reading from cin (enable raw mode)
        struct termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag &= ~ICANON; // disable canonical input
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings

        if ( argc != 2 )
            PrintControlsHelp();
        else
            cin.putback(argv[1][0]);

        while (cin >> c)
        {
            int result = 0; // result of the dds send

            switch (c)
            {
                case 'c':
                    cout << " > checking state of the devices" << endl;
                    result = ddsCustomCmd.send("check-state", "");
                    break;
                case 'i':
                    cout << " > init devices" << endl;
                    result = ddsCustomCmd.send("INIT_DEVICE", "");
                    break;
                case 'j':
                    cout << " > init tasks" << endl;
                    result = ddsCustomCmd.send("INIT_TASK", "");
                    break;
                case 'p':
                    cout << " > pause devices" << endl;
                    result = ddsCustomCmd.send("PAUSE", "");
                    break;
                case 'r':
                    cout << " > run tasks" << endl;
                    result = ddsCustomCmd.send("RUN", "");
                    break;
                case 's':
                    cout << " > stop devices" << endl;
                    result = ddsCustomCmd.send("STOP", "");
                    break;
                case 't':
                    cout << " > reset tasks" << endl;
                    result = ddsCustomCmd.send("RESET_TASK", "");
                    break;
                case 'd':
                    cout << " > reset devices" << endl;
                    result = ddsCustomCmd.send("RESET_DEVICE", "");
                    break;
                case 'h':
                    cout << " > help" << endl;
                    PrintControlsHelp();
                    break;
                case 'q':
                    cout << " > end" << endl;
                    result = ddsCustomCmd.send("END", "");
                    break;
                default:
                    cout << "Invalid input: [" << c << "]" << endl;
                    PrintControlsHelp();
                    break;
            }

            if (result == 1)
            {
                cerr << "Error sending custom command" << endl;
            }
            if ( argc == 2 ) {
                usleep(50000);
                return EXIT_SUCCESS;
            }
        }

        // disable raw mode
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag |= ICANON; // re-enable canonical input
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
    }
    catch (exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
