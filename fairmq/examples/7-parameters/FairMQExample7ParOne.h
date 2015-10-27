/********************************************************************************
 *    Copyright (C) 2014 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH    *
 *                                                                              *
 *              This software is distributed under the terms of the             * 
 *         GNU Lesser General Public Licence version 3 (LGPL) version 3,        *  
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#ifndef FAIRMQEXAMPLE7PARONE_H_
#define FAIRMQEXAMPLE7PARONE_H_

#include "FairParGenericSet.h"

#include "TObject.h"

class FairParamList;

class FairMQExample7ParOne : public FairParGenericSet
{
  public:
    /** Standard constructor **/
    FairMQExample7ParOne(const char* name    = "FairMQExample7ParOne",
                         const char* title   = "FairMQ Example 7 Parameter One",
                         const char* context = "Default");

    /** Destructor **/
    virtual ~FairMQExample7ParOne();

    virtual void print();

    /** Reset all parameters **/
    virtual void clear();

    void putParams(FairParamList* list);
    Bool_t getParams(FairParamList* list);

    inline void SetValue(const Int_t& val) { fParameterValue = val; }

  private:
    Int_t fParameterValue; //

    FairMQExample7ParOne(const FairMQExample7ParOne&);
    FairMQExample7ParOne& operator=(const FairMQExample7ParOne&);

    ClassDef(FairMQExample7ParOne,1);
};

#endif // FAIRMQEXAMPLE7PARONE_H_
