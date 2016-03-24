/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
/**
 * FairMQExample8Sink.h
 *
 * @since 2014-10-10
 * @author A. Rybalchenko
 */

#ifndef FAIRMQEXAMPLE8SINK_H_
#define FAIRMQEXAMPLE8SINK_H_

#include "FairMQDevice.h"


struct Ex8Header {
  int32_t stopFlag;
};


struct MyPoint
{
        /* data */
    double x=1;
    double y=1;
    double z=1;
};


struct MyPointSerializer
{
    void Serialize(std::unique_ptr<FairMQMessage>& msg, MyPoint* input)
    {
        int DataSize = sizeof(MyPoint);
        msg->Rebuild(DataSize);
        MyPoint* digiptr = reinterpret_cast<MyPoint*>(msg->GetData());
        digiptr->x=input->x;
        digiptr->y=input->y;
        digiptr->z=input->z;
    }

    void Deserialize(std::unique_ptr<FairMQMessage>& msg, MyPoint* input)
    {
        MyPoint* digiptr = static_cast<MyPoint*>(msg->GetData());
        input->x=digiptr->x;
        input->y=digiptr->y;
        input->z=digiptr->z;
    }

    void Deserialize(FairMQMessage& msg, MyPoint* input)
    {
        MyPoint* digiptr = static_cast<MyPoint*>(msg.GetData());
        input->x=digiptr->x;
        input->y=digiptr->y;
        input->z=digiptr->z;
    }

};

class FairMQExample8Sink : public FairMQDevice
{
  public:
    FairMQExample8Sink();
    virtual ~FairMQExample8Sink();

  protected:
    virtual void Run();
};

#endif /* FAIRMQEXAMPLE8SINK_H_ */
