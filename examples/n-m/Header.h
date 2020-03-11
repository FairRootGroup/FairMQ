/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIR_MQ_EXAMPLE_N_M_HEADER_H
#define FAIR_MQ_EXAMPLE_N_M_HEADER_H

#include <cstdint>

namespace example_n_m
{

struct Header
{
    std::uint16_t id;
    int senderIndex;
};

} // namespace example_n_m

#endif /* FAIR_MQ_EXAMPLE_N_M_HEADER_H */
