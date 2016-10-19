/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQCONFIGPLUGIN_H_
#define FAIRMQCONFIGPLUGIN_H_

class FairMQDevice;

extern "C"
struct FairMQConfigPlugin
{
    typedef void (*init_t)(FairMQDevice&);
    init_t initConfig;

    typedef void (*handleInitialConfig_t)(FairMQDevice&);
    handleInitialConfig_t handleInitialConfig;

    typedef void (*stop_t)();
    stop_t stopConfig;
};

#endif /* FAIRMQCONFIGPLUGIN_H_ */
