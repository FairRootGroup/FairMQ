/********************************************************************************
 *    Copyright (C) 2017 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#include <fairmq/tools/Unique.h>

// We have to force boost::uuids to rely on /dev/*random instead of getrandom(2) or getentropy(3)
// otherwise on some systems we'd get boost::uuids::entropy_error
#define BOOST_UUID_RANDOM_PROVIDER_FORCE_POSIX

#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace std;

namespace fair::mq::tools
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

} // namespace fair::mq::tools
