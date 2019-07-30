/********************************************************************************
 *    Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_SDK_TOP_H
#define FAIR_MQ_SDK_TOP_H

#include <boost/asio/executor.hpp>
#include <cstddef>
#include <curses.h>
#include <fairmq/sdk/DDSSession.h>

namespace fair {
namespace mq {
namespace sdk {

/**
 * @class Top Top.h <fairmq/sdk/top/Top.h>
 * @brief fairmq-top ncurses application
 */
class Top
{
  public:
    Top(boost::asio::executor, DDSSession);
    ~Top();
    auto AsyncRun() -> void;

  private:
    boost::asio::executor fExecutor;
    DDSSession fDDSSession;
}; /* class Top */

} /* namespace sdk */
} /* namespace mq */
} /* namespace fair */

#endif /* FAIR_MQ_SDK_TOP_H */
