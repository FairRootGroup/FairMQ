/*
 * FairMQContext.h
 *
 *  Created on: Dec 5, 2012
 *      Author: dklein
 */

#ifndef FAIRMQCONTEXT_H_
#define FAIRMQCONTEXT_H_

#include <string>
#include <zmq.hpp>
#include "Rtypes.h"
#include "TString.h"


class FairMQContext
{
  private:
    TString fId;
    zmq::context_t* fContext;
  public:
    const static TString PAYLOAD, LOG, CONFIG, CONTROL;
    FairMQContext(TString deviceId, TString contextId, Int_t numIoThreads);
    virtual ~FairMQContext();
    TString GetId();
    zmq::context_t* GetContext();
    void Close();
};

#endif /* FAIRMQCONTEXT_H_ */
