/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/

#ifndef FAIRMQCONTROLPLUGIN_H_
#define FAIRMQCONTROLPLUGIN_H_

class FairMQDevice;

extern "C"
struct FairMQControlPlugin
{
    typedef void (*init_t)(FairMQDevice&);
    init_t initControl;

    typedef void (*handleStateChanges_t)(FairMQDevice&);
    handleStateChanges_t handleStateChanges;

    typedef void (*stop_t)();
    stop_t stopControl;
};

#endif /* FAIRMQCONTROLPLUGIN_H_ */
