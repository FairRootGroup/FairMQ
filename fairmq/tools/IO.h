/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TOOLS_IO_H
#define FAIR_MQ_TOOLS_IO_H

#include <termios.h>
#include <unistd.h>

namespace fair::mq::tools
{

/// Enables noncanonical mode for the input terminal for the lifetime of this object.
/// (input becomes available to the application without user having to type a line-delimiter)
/// Read `man termios` for more details
struct NonCanonicalInput
{
    NonCanonicalInput()
    {
        termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag &= ~ICANON; // disable canonical input
        t.c_lflag &= ~ECHO; // do not echo input chars
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
    }

    NonCanonicalInput(const NonCanonicalInput&) = delete;
    NonCanonicalInput(NonCanonicalInput&&) = delete;
    NonCanonicalInput& operator=(const NonCanonicalInput&) = delete;
    NonCanonicalInput& operator=(NonCanonicalInput&&) = delete;

    ~NonCanonicalInput()
    {
        termios t;
        tcgetattr(STDIN_FILENO, &t); // get the current terminal I/O structure
        t.c_lflag |= ICANON; // re-enable canonical input
        t.c_lflag |= ECHO; // echo input chars
        tcsetattr(STDIN_FILENO, TCSANOW, &t); // apply the new settings
    }
};

} // namespace fair::mq::tools

#endif /* FAIR_MQ_TOOLS_IO_H */
