/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tools/Unique.h>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/functional/hash.hpp>

using namespace std;

namespace fair
{
namespace mq
{
namespace tools
{

// generates UUID string
string Uuid()
{
    boost::uuids::random_generator gen;
    boost::uuids::uuid u = gen();
    return boost::uuids::to_string(u);
}

// generates UUID and returns its hash
size_t UuidHash()
{
    boost::uuids::random_generator gen;
    boost::hash<boost::uuids::uuid> uuid_hasher;
    boost::uuids::uuid u = gen();
    return uuid_hasher(u);
}

} /* namespace tools */
} /* namespace mq */
} /* namespace fair */
