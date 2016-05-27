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
    cout << "To control devices use one of the following arguments:" << endl;
    cout << "[c] check states, [h] help, [p] pause, [r] run, [s] stop, [t] reset task, [d] reset device, [q] end, [j] init task, [i] init device" << endl;
}

int main(int argc, char* argv[])
{
    try
    {
      if ( argc != 2 ) {
	PrintControlsHelp();
	return EXIT_FAILURE;
      }
      
      CCustomCmd ddsCustomCmd;
      
      // subscribe to receive messages from DDS
      ddsCustomCmd.subscribe([](const string& msg, const string& condition, uint64_t senderId)
			     {
			       cout << "Received: \"" << msg << "\"" << endl;
			     });
      
      char c = argv[1][0];
      
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
	  cout << "Error sending custom command" << endl;
	}
      usleep(50000);
    }
    catch (exception& e)
      {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
      }
    
    return EXIT_SUCCESS;
}
