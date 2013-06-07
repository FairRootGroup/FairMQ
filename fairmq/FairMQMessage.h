/*
 * FairMQMessage.h
 *
 *  Created on: Dec 5, 2012
 *      Author: dklein
 */

#ifndef FAIRMQMESSAGE_H_
#define FAIRMQMESSAGE_H_

#include <zmq.hpp>
#include "Rtypes.h"


class FairMQMessage
{
  private:
    zmq::message_t* fMessage;
  public:
    FairMQMessage();
    FairMQMessage(void* data_, size_t size_, zmq::free_fn* ffn_, void* hint_ = NULL);
    virtual ~FairMQMessage();
    zmq::message_t* GetMessage();
    Int_t Size();
    Bool_t Copy(FairMQMessage* msg);
};

#endif /* FAIRMQMESSAGE_H_ */
