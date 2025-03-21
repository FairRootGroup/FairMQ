/********************************************************************************
 * Copyright (C) 2014-2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_TRANSPORTENUMS_H
#define FAIR_MQ_TRANSPORTENUMS_H

namespace fair::mq {

enum class Transport {
    DEFAULT,
    ZMQ,
    SHM
};

}

#endif // FAIR_MQ_TRANSPORTENUMS_H
