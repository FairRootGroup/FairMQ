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
    LOG(info) << "Received message with " << parts.Size() << " parts";

    Header header;
    header.stopFlag = (static_cast<Header*>(parts.At(0)->GetData()))->stopFlag;

    LOG(info) << "Received part 1 (header) with stopFlag: " << header.stopFlag;
    LOG(info) << "Received part 2 of size: " << parts.At(1)->GetSize();
    assert(parts.At(1)->GetSize() == 1000);
    LOG(info) << "Received part 3 of size: " << parts.At(2)->GetSize();
    assert(parts.At(2)->GetSize() == 500);
    LOG(info) << "Received part 4 of size: " << parts.At(3)->GetSize();
    assert(parts.At(3)->GetSize() == 600);
    LOG(info) << "Received part 5 of size: " << parts.At(4)->GetSize();
    assert(parts.At(4)->GetSize() == 700);
    LOG(info) << "Received part 6 of size: " << parts.At(5)->GetSize();
    assert(parts.At(5)->GetSize() == 0);
    LOG(info) << "Received part 7 of size: " << parts.At(6)->GetSize();
    assert(parts.At(6)->GetSize() == 100);

    if (header.stopFlag == 1) {
        LOG(info) << "stopFlag is 1, going IDLE";
        return false;
    }

    return true;
}

}
