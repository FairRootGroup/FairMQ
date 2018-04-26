/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Broadcaster.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLEMULTIPLECHANNELSBROADCASTER_H
#define FAIRMQEXAMPLEMULTIPLECHANNELSBROADCASTER_H

#include "FairMQDevice.h"

namespace example_multiple_channels
{

class Broadcaster : public FairMQDevice
{
  public:
    Broadcaster();
    virtual ~Broadcaster();

  protected:
    virtual bool ConditionalRun();
};

}

#endif /* FAIRMQEXAMPLEMULTIPLECHANNELSBROADCASTER_H */
