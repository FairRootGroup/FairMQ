/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * Sink.cxx
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#include "Sink.h"
#include "Header.h"

using namespace std;

namespace example_multipart
{

bool Sink::HandleData(FairMQParts& parts, int /*index*/)
{
    Header header;
    header.stopFlag = (static_cast<Header*>(parts.At(0)->GetData()))->stopFlag;

    LOG(info) << "Received message with " << parts.Size() << " parts";

    LOG(info) << "Received header with stopFlag: " << header.stopFlag;
    LOG(info) << "Received body of size: " << parts.At(1)->GetSize();

    if (header.stopFlag == 1) {
        LOG(info) << "stopFlag is 1, going IDLE";
        return false;
    }

    return true;
}

}
