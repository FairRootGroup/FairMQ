/********************************************************************************
 *    Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include "Common.h"

#include <picosha2.h>

#include <unistd.h>

#include <sstream>
#include <string>

namespace fair::mq::shmem
{

std::string makeShmIdStr(const std::string& sessionId, const std::string& userId)
{
    std::string seed(userId + sessionId);
    // generate a 8-digit hex value out of sha256 hash
    std::vector<unsigned char> hash(4);
    picosha2::hash256(seed.begin(), seed.end(), hash.begin(), hash.end());

    return picosha2::bytes_to_hex_string(hash.begin(), hash.end());
}

std::string makeShmIdStr(const std::string& sessionId)
{
    return makeShmIdStr(sessionId, std::to_string(geteuid()));
}

uint64_t makeShmIdUint64(const std::string& sessionId)
{
    std::string shmId = makeShmIdStr(sessionId);
    uint64_t id = 0;
    std::stringstream ss;
    ss << std::hex << shmId;
    ss >> id;

    return id;
}


}   // namespace fair::mq::shmem
