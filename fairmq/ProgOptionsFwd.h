/********************************************************************************
 * Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIR_MQ_PROGOPTIONSFWD_H
#define FAIR_MQ_PROGOPTIONSFWD_H

namespace fair::mq {
class ProgOptions;
}

// using FairMQProgOptions [[deprecated("Use fair::mq::ProgOptions")]] = fair::mq::ProgOptions;
using FairMQProgOptions = fair::mq::ProgOptions;

#endif /* FAIR_MQ_PROGOPTIONSFWD_H */
