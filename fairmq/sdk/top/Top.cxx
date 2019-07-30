/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Top.h"

#include <exception>
#include <iostream>

namespace fair {
namespace mq {
namespace sdk {

Top::Top(boost::asio::executor executor, DDSSession session)
    : fExecutor(std::move(executor))
    , fDDSSession(std::move(session))
{
    initscr();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
}

Top::~Top()
{
    endwin();
}

auto Top::AsyncRun() -> void
{
}

} /* namespace sdk */
} /* namespace mq */
} /* namespace fair */
